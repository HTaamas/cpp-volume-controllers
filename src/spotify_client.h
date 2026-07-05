#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QTcpServer>
#include <QTcpSocket>
#include <QElapsedTimer>
#include <functional>
#include <app_config.h>

class SpotifyClient : public QObject {
    Q_OBJECT
public:
    explicit SpotifyClient(QObject *parent = nullptr);
    void startAuth();
    bool setVolume(int volume);
    void togglePlayPause();
    void nextTrack();
    void prevTrack();
    void pollPlayback();
    int getPollIntervalMs();
    bool hasCredentialsConfigured() const;
    bool hasStoredSession() const;
    QString authRedirectUri() const;
    bool isRateLimited() const;
    int rateLimitRemainingMs() const;


    QTimer *pollTimer = nullptr;

signals:
    void trackChanged(int volume, const QString &track, const QString &artist, const QString &trackId, const QString &albumArtUrl, int progressMs, int durationMs, bool isPlaying, bool volumeControlSupported);
    void stateSynced(int volume, int progressMs, bool isPlaying, bool volumeControlSupported);
    void rateLimitChanged(int retryAfterMs);
    void authComplete();

private slots:
    void handleTokenResponse(QNetworkReply *reply);
    void handlePlaybackResponse(QNetworkReply *reply);
    void handleVolumeResponse(QNetworkReply *reply);
    void onNewConnection();

private:
    QString redirectUri() const;
    QNetworkRequest authorizedRequest(const QUrl &url) const;
    void updatePollTimerInterval();
    void exchangeAuthorizationCode(const QString &code);
    void postTokenRequest(const QUrlQuery &body, const std::function<void(QNetworkReply *)> &handler);
    void setVolumeControlSupported(bool supported);
    void emitPlaybackState();
    void emitRateLimitState(int retryAfterMs);
    void enterRateLimitCooldown(int retryAfterMs);
    int parseRetryAfterMs(QNetworkReply *reply) const;
    void refreshAccessToken();
    void saveTokens();
    void loadTokens();

    int localNoDevicePollIntervalMs = AppConfig::NO_DEVICE_POLL_INTERVAL_STARTING_MS;
    QNetworkAccessManager *network;
    const QString clientId = AppConfig::SPOTIFY_CLIENT_ID;
    const QString clientSecret = AppConfig::SPOTIFY_CLIENT_SECRET;
    QString accessToken;
    QString refreshToken;
    QString lastTrackId;
    bool volumeControlSupported = true;
    int currentVolume = 50;
    int lastProgressMs = 0;
    bool lastIsPlaying = false;
    int pendingVolume = -1;
    QElapsedTimer pendingVolumeTimer;
    QTimer *rateLimitTimer = nullptr;
    bool rateLimited = false;
    int rateLimitRetryAfterMs = 0;

    QTcpServer *authServer = nullptr;
    const int redirectPort = AppConfig::REDIRECT_PORT;
};

#endif // SPOTIFY_CLIENT_H
