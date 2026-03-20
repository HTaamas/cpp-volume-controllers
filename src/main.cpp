#include <QApplication>
#include "spotify_client.h"
#include "osd_window.h"
#include "tray_manager.h"
#include "volume_handler.h"
#include <QMessageBox>
#include <QMetaObject>
#include <atomic>
#include <exception>
#include <QDebug>
#include <QElapsedTimer>

namespace {
void appMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg) {
    static std::atomic_bool dialogQueued{false};

    if (type == QtCriticalMsg || type == QtFatalMsg) {
        if (QApplication::instance() && !dialogQueued.exchange(true)) {
            const QString errorText = msg;
            QMetaObject::invokeMethod(QApplication::instance(), [errorText]() {
                QMessageBox::critical(nullptr, "SpotifyVol Error", errorText);
                dialogQueued.store(false);
            }, Qt::QueuedConnection);
        }
    }

    const QByteArray local = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", local.constData());

    if (type == QtFatalMsg) {
        abort();
    }
}
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    qInstallMessageHandler(appMessageHandler);

    SpotifyClient spotify;
    OSDWindow osd;
    TrayManager tray;
    VolumeHandler volHandler;

    int currentVolume = 50;
    QString currentTrack = "Loading...";
    QString currentArtist = "Spotify";
    QString currentArtUrl = "";
    int currentProgress = 0;
    int currentDuration = 0;
    bool currentIsPlaying = false;
    QElapsedTimer progressClock;

    auto updateProgressBaseline = [&](int progressMs, bool isPlaying) {
        currentProgress = progressMs;
        currentIsPlaying = isPlaying;

        if (currentIsPlaying) {
            if (progressClock.isValid()) {
                progressClock.restart();
            } else {
                progressClock.start();
            }
        }
    };

    auto estimatedProgressNow = [&]() {
        if (!progressClock.isValid() || !currentIsPlaying) {
            return currentProgress;
        }
        int estimated = currentProgress + static_cast<int>(progressClock.elapsed());
        if (currentDuration > 0) {
            estimated = qBound(0, estimated, currentDuration);
        }
        return estimated;
    };

    QObject::connect(&spotify, &SpotifyClient::trackChanged, [&](int volume, const QString &track, const QString &artist, const QString &trackId, const QString &albumArtUrl, int progressMs, int durationMs, bool isPlaying) {
        currentVolume = volume;
        currentTrack = track;
        currentArtist = artist;
        currentArtUrl = albumArtUrl;
        updateProgressBaseline(progressMs, isPlaying);
        currentDuration = durationMs;
        osd.showVolume(volume, track, artist, albumArtUrl, progressMs, durationMs, isPlaying);
        tray.updateTrackInfo(track, artist);
    });

    QObject::connect(&spotify, &SpotifyClient::stateSynced, [&](int volume, int progressMs, bool isPlaying) {
        const bool playbackStateChanged = (isPlaying != currentIsPlaying);
        currentVolume = volume;
        updateProgressBaseline(progressMs, isPlaying);
        osd.syncProgress(progressMs, isPlaying);

        if (playbackStateChanged) {
            osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, progressMs, currentDuration, currentIsPlaying);
        }
    });

    QObject::connect(&volHandler, &VolumeHandler::volumeChanged, [&](int delta) {
        currentVolume = qBound(0, currentVolume + delta, 100);
        spotify.setVolume(currentVolume);
        osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, estimatedProgressNow(), currentDuration, currentIsPlaying);
    });

    // Check if we need to auth
    QTimer::singleShot(1000, [&spotify]() {
        spotify.pollPlayback();
        // If we don't have a token, start auth flow
        // In this simple rewrite, we might want to check the token state properly.
        // spotify.startAuth();
    });

    try {
        return app.exec();
    } catch (const std::exception &ex) {
        QMessageBox::critical(nullptr, "SpotifyVol Fatal Error", QString("Unhandled exception: %1").arg(ex.what()));
    } catch (...) {
        QMessageBox::critical(nullptr, "SpotifyVol Fatal Error", "Unhandled unknown exception.");
    }

    return 1;
}
