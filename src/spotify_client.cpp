#include "spotify_client.h"
#include <app_config.h>
#include <QSettings>
#include <QDebug>
#include <QProcessEnvironment>
#include <QFile>
#include <QCoreApplication>

namespace {
constexpr int kPendingVolumeGracePeriodMs = 2500;
constexpr auto kSpotifyAuthScope = "user-modify-playback-state user-read-playback-state";
const QUrl kAuthorizeUrl(QStringLiteral("https://accounts.spotify.com/authorize"));
const QUrl kTokenUrl(QStringLiteral("https://accounts.spotify.com/api/token"));
const QUrl kPlaybackUrl(QStringLiteral("https://api.spotify.com/v1/me/player"));
const QUrl kVolumeUrl(QStringLiteral("https://api.spotify.com/v1/me/player/volume"));
}

SpotifyClient::SpotifyClient(QObject *parent) : QObject(parent), network(new QNetworkAccessManager(this)) {
    loadTokens();

    QTimer *pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SpotifyClient::pollPlayback);
    pollTimer->start(AppConfig::POLL_INTERVAL_MS);
}

QString SpotifyClient::redirectUri() const {
    return QString("http://127.0.0.1:%1/callback").arg(redirectPort);
}

QNetworkRequest SpotifyClient::authorizedRequest(const QUrl &url) const {
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    return request;
}

void SpotifyClient::postTokenRequest(const QUrlQuery &body, const std::function<void(QNetworkReply *)> &handler) {
    QNetworkRequest request(kTokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = network->post(request, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply, handler]() {
        handler(reply);
    });
}

void SpotifyClient::setVolumeControlSupported(bool supported) {
    volumeControlSupported = supported;
    if (!supported) {
        pendingVolume = -1;
    }
}

void SpotifyClient::emitPlaybackState() {
    emit stateSynced(currentVolume, lastProgressMs, lastIsPlaying, volumeControlSupported);
}

void SpotifyClient::startAuth() {
    if (authServer) {
        authServer->close();
        authServer->deleteLater();
        authServer = nullptr;
    }

    authServer = new QTcpServer(this);
    if (!authServer->listen(QHostAddress::LocalHost, redirectPort)) {
        qDebug() << "Failed to start auth server";
        authServer->deleteLater();
        authServer = nullptr;
        return;
    }
    connect(authServer, &QTcpServer::newConnection, this, &SpotifyClient::onNewConnection);

    QUrl authUrl(kAuthorizeUrl);
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("response_type", "code");
    query.addQueryItem("redirect_uri", redirectUri());
    query.addQueryItem("scope", kSpotifyAuthScope);
    authUrl.setQuery(query);
    QDesktopServices::openUrl(authUrl);
}

void SpotifyClient::exchangeAuthorizationCode(const QString &code) {
    QUrlQuery body;
    body.addQueryItem("grant_type", "authorization_code");
    body.addQueryItem("code", code);
    body.addQueryItem("redirect_uri", redirectUri());
    body.addQueryItem("client_id", clientId);
    body.addQueryItem("client_secret", clientSecret);

    postTokenRequest(body, [this](QNetworkReply *reply) {
        handleTokenResponse(reply);
    });
}

void SpotifyClient::onNewConnection() {
    if (!authServer) {
        return;
    }

    QTcpSocket *socket = authServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, [this, socket]() {
        const QString requestData = QString::fromUtf8(socket->readAll());
        const int requestLineEnd = requestData.indexOf("\r\n");
        const QString requestLine = requestLineEnd >= 0 ? requestData.left(requestLineEnd) : requestData;
        const QStringList parts = requestLine.split(' ', Qt::SkipEmptyParts);

        if (parts.size() >= 2 && parts[0] == "GET") {
            const QUrl callbackUrl(QString("http://127.0.0.1:%1%2").arg(redirectPort).arg(parts[1]));
            const QString code = QUrlQuery(callbackUrl).queryItemValue("code");

            socket->write("HTTP/1.1 200 OK\r\n\r\nAuth Complete! You can close this window.");
            socket->disconnectFromHost();

            if (authServer) {
                authServer->close();
                authServer->deleteLater();
                authServer = nullptr;
            }

            if (code.isEmpty()) {
                qDebug() << "Spotify auth callback arrived without an authorization code.";
                return;
            }

            exchangeAuthorizationCode(code);
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
    
    QUrl url(kVolumeUrl);
    QUrlQuery query;
    query.addQueryItem("volume_percent", QString::number(currentVolume));
    url.setQuery(query);

    QNetworkReply *reply = network->put(authorizedRequest(url), QByteArray());
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
    
    QNetworkReply *reply = network->get(authorizedRequest(kPlaybackUrl));
    connect(reply, &QNetworkReply::finished, [this, reply]() { handlePlaybackResponse(reply); });
}

void SpotifyClient::handlePlaybackResponse(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            setVolumeControlSupported(false);
            emitPlaybackState();
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
            setVolumeControlSupported(false);
            lastIsPlaying = obj["is_playing"].toBool();
            lastProgressMs = obj["progress_ms"].toInt(lastProgressMs);
            emitPlaybackState();
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
        setVolumeControlSupported(device.value("supports_volume").toBool(true));
        int vol = device["volume_percent"].toInt();
        int progressMs = obj["progress_ms"].toInt();
        int durationMs = item["duration_ms"].toInt();
        bool isPlaying = obj["is_playing"].toBool();
        lastProgressMs = progressMs;
        lastIsPlaying = isPlaying;

        // Keep local volume changes stable for a short window until Spotify catches up.
        int effectiveVolume = vol;
        if (pendingVolume >= 0) {
            if (qAbs(vol - pendingVolume) <= 1) {
                pendingVolume = -1;
            } else if (pendingVolumeTimer.isValid() && pendingVolumeTimer.elapsed() < kPendingVolumeGracePeriodMs) {
                effectiveVolume = pendingVolume;
            } else {
                pendingVolume = -1;
            }
        }

        currentVolume = effectiveVolume;
        emitPlaybackState();

        if (trackId != lastTrackId) {
            lastTrackId = trackId;
            emit trackChanged(currentVolume, trackName, artistName, trackId, albumArtUrl, progressMs, durationMs, isPlaying, volumeControlSupported);
        }
    } else {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 401) {
            refreshAccessToken();
        } else if (statusCode == 403 || statusCode == 404) {
            setVolumeControlSupported(false);
            emitPlaybackState();
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
            setVolumeControlSupported(false);
            emitPlaybackState();
        }
        pendingVolume = -1;
    }
    reply->deleteLater();
}

void SpotifyClient::refreshAccessToken() {
    if (refreshToken.isEmpty()) return;

    QUrlQuery body;
    body.addQueryItem("grant_type", "refresh_token");
    body.addQueryItem("refresh_token", refreshToken);
    body.addQueryItem("client_id", clientId);
    body.addQueryItem("client_secret", clientSecret);

    postTokenRequest(body, [this](QNetworkReply *reply) {
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
