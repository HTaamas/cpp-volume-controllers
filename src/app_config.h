#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QString>

namespace AppConfig {
    // These values are generated at build time from .env
    // IDE placeholder - actual values come from build/include/app_config.h
    const QString SPOTIFY_CLIENT_ID = "";
    const QString SPOTIFY_CLIENT_SECRET = "";
    constexpr int POLL_INTERVAL_MS = 2000;
    constexpr int PAUSED_POLL_INTERVAL_MS = 10000;
    constexpr int NO_DEVICE_POLL_INTERVAL_STARTING_MS = 2000;
    constexpr int NO_DEVICE_POLL_INTERVAL_INCREMENT_MS = 10000;
    constexpr int NO_DEVICE_POLL_INTERVAL_MAX_MS = 60000;
    constexpr int REDIRECT_PORT = 8888;
}

#endif // APP_CONFIG_H
