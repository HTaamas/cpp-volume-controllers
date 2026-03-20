#include <QApplication>
#include "spotify_client.h"
#include "osd_window.h"
#include "tray_manager.h"
#include "volume_handler_factory.h"
#include <QDebug>
#include <QElapsedTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    SpotifyClient spotify;
    OSDWindow osd;
    TrayManager tray;
    VolumeHandlerImpl volHandler;

    int currentVolume = 50;
    QString currentTrack = "Loading...";
    QString currentArtist = "Spotify";
    QString currentArtUrl = "";
    int currentProgress = 0;
    int currentDuration = 0;
    QElapsedTimer progressClock;

    auto updateProgressBaseline = [&](int progressMs) {
        currentProgress = progressMs;
        if (progressClock.isValid()) {
            progressClock.restart();
        } else {
            progressClock.start();
        }
    };

    auto estimatedProgressNow = [&]() {
        if (!progressClock.isValid()) {
            return currentProgress;
        }
        int estimated = currentProgress + static_cast<int>(progressClock.elapsed());
        if (currentDuration > 0) {
            estimated = qBound(0, estimated, currentDuration);
        }
        return estimated;
    };

    QObject::connect(&spotify, &SpotifyClient::trackChanged, [&](int volume, const QString &track, const QString &artist, const QString &trackId, const QString &albumArtUrl, int progressMs, int durationMs) {
        currentVolume = volume;
        currentTrack = track;
        currentArtist = artist;
        currentArtUrl = albumArtUrl;
        updateProgressBaseline(progressMs);
        currentDuration = durationMs;
        osd.showVolume(volume, track, artist, albumArtUrl, progressMs, durationMs);
        tray.updateTrackInfo(track, artist);
    });

    QObject::connect(&spotify, &SpotifyClient::stateSynced, [&](int volume, int progressMs) {
        currentVolume = volume;
        updateProgressBaseline(progressMs);
        osd.syncProgress(progressMs);
    });

    QObject::connect(&volHandler, &VolumeHandler::volumeChanged, [&](int delta) {
        currentVolume = qBound(0, currentVolume + delta, 100);
        spotify.setVolume(currentVolume);
        osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, estimatedProgressNow(), currentDuration);
    });

    // Check if we need to auth
    QTimer::singleShot(1000, [&spotify]() {
        spotify.pollPlayback();
        // If we don't have a token, start auth flow
        // In this simple rewrite, we might want to check the token state properly.
        // spotify.startAuth();
    });

    return app.exec();
}
