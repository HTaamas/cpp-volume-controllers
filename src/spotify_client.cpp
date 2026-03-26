#include "spotify_client.h"
#include <app_config.h>
#include <QSettings>
#include <QDebug>
#include <QProcessEnvironment>
#include <QFile>
#include <QCoreApplication>

SpotifyClient::SpotifyClient(QObject *parent) : QObject(parent), network(new QNetworkAccessManager(this)) {

    loadTokens();
    
    QTimer *pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SpotifyClient::pollPlayback);
    pollTimer->start(AppConfig::POLL_INTERVAL_MS);
}

void SpotifyClient::startAuth() {
    authServer = new QTcpServer(this);
    if (!authServer->listen(QHostAddress::LocalHost, redirectPort)) {
        qDebug() << "Failed to start auth server";
        return;
    }
    connect(authServer, &QTcpServer::newConnection, this, &SpotifyClient::onNewConnection);

    QUrl authUrl("https://accounts.spotify.com/authorize");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("redirect_uri", "http://127.0.0.1:8888/callback");
    query.addQueryItem("scope", "user-modify-playback-state user-read-playback-state");
    authUrl.setQuery(query);
    QDesktopServices::openUrl(authUrl);
}

void SpotifyClient::onNewConnection() {
    QTcpSocket *socket = authServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        QString data = socket->readAll();
        if (data.contains("GET /callback")) {
            int codeStart = data.indexOf("code=") + 5;
            int codeEnd = data.indexOf(" ", codeStart);
            QString code = data.mid(codeStart, codeEnd - codeStart);
            
            socket->write("HTTP/1.1 200 OK\r\n\r\nAuth Complete! You can close this window.");
            socket->disconnectFromHost();
            authServer->close();

            // Exchange code for token
            QNetworkRequest request(QUrl("https://accounts.spotify.com/api/token"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            
            QUrlQuery body;
            body.addQueryItem("grant_type", "authorization_code");
            body.addQueryItem("code", code);
            body.addQueryItem("redirect_uri", "http://127.0.0.1:8888/callback");
            body.addQueryItem("client_id", clientId);
            body.addQueryItem("client_secret", clientSecret);
            
            QNetworkReply *reply = network->post(request, body.toString(QUrl::FullyEncoded).toUtf8());
            connect(reply, &QNetworkReply::finished, [this, reply]() { handleTokenResponse(reply); });
        }
    });
}

void SpotifyClient::handleTokenResponse(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        accessToken = doc.object()["access_token"].toString();
        refreshToken = doc.object()["refresh_token"].toString();
        saveTokens();
        emit authComplete();
    } else {
        qDebug() << "Token Error:" << reply->errorString();
    }
    reply->deleteLater();
}

bool SpotifyClient::setVolume(int volume) {
    if (accessToken.isEmpty() || !volumeControlSupported) {
        return false;
    }

    currentVolume = qBound(0, volume, 100);
    pendingVolume = currentVolume;
    pendingVolumeTimer.restart();
    
    QUrl url("https://api.spotify.com/v1/me/player/volume");
    QUrlQuery query;
    query.addQueryItem("volume_percent", QString::number(currentVolume));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    
    QNetworkReply *reply = network->put(request, QByteArray());
    connect(reply, &QNetworkReply::finished, [this, reply]() { handleVolumeResponse(reply); });
    return true;
}

void SpotifyClient::pollPlayback() {
    if (accessToken.isEmpty()) {
        if (!refreshToken.isEmpty()) {
            refreshAccessToken();
        } else {
            startAuth();
        }
        return;
    }
    
    QNetworkRequest request(QUrl("https://api.spotify.com/v1/me/player"));
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    
    QNetworkReply *reply = network->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { handlePlaybackResponse(reply); });
}

void SpotifyClient::handlePlaybackResponse(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            volumeControlSupported = false;
            pendingVolume = -1;
            emit stateSynced(currentVolume, lastProgressMs, lastIsPlaying, volumeControlSupported);
            reply->deleteLater();
            return; // 204 No Content / no active playback payload
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qDebug() << "Playback Error: invalid JSON payload";
            reply->deleteLater();
            return;
        }

        QJsonObject obj = doc.object();
        QJsonObject item = obj["item"].toObject();
        QJsonObject device = obj["device"].toObject();
        if (item.isEmpty() || device.isEmpty()) {
            volumeControlSupported = false;
            pendingVolume = -1;
            lastIsPlaying = obj["is_playing"].toBool();
            lastProgressMs = obj["progress_ms"].toInt(lastProgressMs);
            emit stateSynced(currentVolume, lastProgressMs, lastIsPlaying, volumeControlSupported);
            reply->deleteLater();
            return;
        }
        
        QString trackId = item["id"].toString();
        QString trackName = item["name"].toString();
        
        QString artistName;
        QJsonArray artistsArray = item["artists"].toArray();
        for (int i = 0; i < artistsArray.size(); ++i) {
            if (i > 0) artistName += ", ";
            artistName += artistsArray[i].toObject()["name"].toString();
        }

        const QJsonArray images = item["album"].toObject()["images"].toArray();
        QString albumArtUrl;
        if (!images.isEmpty()) {
            albumArtUrl = images.first().toObject()["url"].toString();
        }
        volumeControlSupported = device.value("supports_volume").toBool(true);
        int vol = device["volume_percent"].toInt();
        int progressMs = obj["progress_ms"].toInt();
        int durationMs = item["duration_ms"].toInt();
        bool isPlaying = obj["is_playing"].toBool();
        lastProgressMs = progressMs;
        lastIsPlaying = isPlaying;

        if (!volumeControlSupported) {
            pendingVolume = -1;
        }

        // Keep local volume changes stable for a short window until Spotify catches up.
        int effectiveVolume = vol;
        if (pendingVolume >= 0) {
            if (qAbs(vol - pendingVolume) <= 1) {
                pendingVolume = -1;
            } else if (pendingVolumeTimer.isValid() && pendingVolumeTimer.elapsed() < 2500) {
                effectiveVolume = pendingVolume;
            } else {
                pendingVolume = -1;
            }
        }

        currentVolume = effectiveVolume;
        emit stateSynced(currentVolume, progressMs, isPlaying, volumeControlSupported);

        if (trackId != lastTrackId) {
            lastTrackId = trackId;
            emit trackChanged(currentVolume, trackName, artistName, trackId, albumArtUrl, progressMs, durationMs, isPlaying, volumeControlSupported);
        }
    } else {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            refreshAccessToken();
        } else if (statusCode == 403 || statusCode == 404) {
            volumeControlSupported = false;
            pendingVolume = -1;
            emit stateSynced(currentVolume, lastProgressMs, lastIsPlaying, volumeControlSupported);
        } else {
            qDebug() << "Playback Error:" << reply->errorString() << "status" << statusCode;
        }
    }
    reply->deleteLater();
}

void SpotifyClient::handleVolumeResponse(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Volume Error:" << reply->errorString() << "status" << statusCode;
        if (statusCode == 403 || statusCode == 404) {
            volumeControlSupported = false;
            emit stateSynced(currentVolume, lastProgressMs, lastIsPlaying, volumeControlSupported);
        }
        pendingVolume = -1;
    }
    reply->deleteLater();
}

void SpotifyClient::refreshAccessToken() {
    if (refreshToken.isEmpty()) return;
    
    QNetworkRequest request(QUrl("https://accounts.spotify.com/api/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QUrlQuery body;
    body.addQueryItem("grant_type", "refresh_token");
    body.addQueryItem("refresh_token", refreshToken);
    body.addQueryItem("client_id", clientId);
    body.addQueryItem("client_secret", clientSecret);
    
    QNetworkReply *reply = network->post(request, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            accessToken = doc.object()["access_token"].toString();
            saveTokens();
        }
        reply->deleteLater();
    });
}

void SpotifyClient::saveTokens() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    settings.setValue("accessToken", accessToken);
    settings.setValue("refreshToken", refreshToken);
}

void SpotifyClient::loadTokens() {
    QSettings settings("SpotifyVol", "SpotifyVol");
    accessToken = settings.value("accessToken").toString();
    refreshToken = settings.value("refreshToken").toString();
}
