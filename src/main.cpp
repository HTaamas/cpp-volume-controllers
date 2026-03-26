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

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

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

#ifdef _WIN32
bool isProcessElevated() {
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    BOOL isMember = FALSE;

    if (!AllocateAndInitializeSid(
            &ntAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &adminGroup)) {
        return false;
    }

    const BOOL checkResult = CheckTokenMembership(nullptr, adminGroup, &isMember);
    FreeSid(adminGroup);

    return checkResult && isMember;
}

bool ensureAdminPrivileges() {
    if (isProcessElevated()) {
        return true;
    }

    wchar_t exePath[MAX_PATH] = {0};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        const HINSTANCE elevateResult = ShellExecuteW(
            nullptr,
            L"runas",
            exePath,
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );
        if (reinterpret_cast<INT_PTR>(elevateResult) > 32) {
            // Relaunch succeeded; current non-elevated process should exit.
            return false;
        }
    }

    MessageBoxW(
        nullptr,
        L"SpotifyVol requires administrator privileges and could not auto-elevate.",
        L"SpotifyVol",
        MB_OK | MB_ICONERROR
    );

    return false;
}
#endif
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    if (!ensureAdminPrivileges()) {
        return 0;
    }
#endif

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
    bool currentVolumeControlSupported = true;
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

    QObject::connect(&spotify, &SpotifyClient::trackChanged, [&](int volume, const QString &track, const QString &artist, const QString &trackId, const QString &albumArtUrl, int progressMs, int durationMs, bool isPlaying, bool volumeControlSupported) {
        currentVolume = volume;
        currentTrack = track;
        currentArtist = artist;
        currentArtUrl = albumArtUrl;
        currentVolumeControlSupported = volumeControlSupported;
        updateProgressBaseline(progressMs, isPlaying);
        currentDuration = durationMs;
        osd.showVolume(volume, track, artist, albumArtUrl, progressMs, durationMs, isPlaying, volumeControlSupported);
        tray.updateTrackInfo(track, artist);
    });

    QObject::connect(&spotify, &SpotifyClient::stateSynced, [&](int volume, int progressMs, bool isPlaying, bool volumeControlSupported) {
        const bool playbackStateChanged = (isPlaying != currentIsPlaying);
        currentVolume = volume;
        currentVolumeControlSupported = volumeControlSupported;
        updateProgressBaseline(progressMs, isPlaying);
        osd.syncProgress(progressMs, isPlaying, volumeControlSupported);

        if (playbackStateChanged) {
            osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, progressMs, currentDuration, currentIsPlaying, currentVolumeControlSupported);
        }
    });

    QObject::connect(&volHandler, &VolumeHandler::volumeChanged, [&](int delta) {
        const int requestedVolume = qBound(0, currentVolume + delta, 100);
        if (!spotify.setVolume(requestedVolume)) {
            if (!currentVolumeControlSupported) {
                osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, estimatedProgressNow(), currentDuration, currentIsPlaying, false);
            }
            return;
        }

        currentVolume = requestedVolume;
        osd.showVolume(currentVolume, currentTrack, currentArtist, currentArtUrl, estimatedProgressNow(), currentDuration, currentIsPlaying, currentVolumeControlSupported);
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
