# SpotifyVol

A lightweight, cross-platform Spotify controller with a custom volume OSD and audio ducking. It runs in your system tray and uses global media keys to control Spotify's playback volume, not your system volume.



## Features

*   **Global Hotkey Control**: Intercepts system media keys (Volume Up/Down) to control Spotify's application volume directly. Use Shift for fine-grained adjustments.
*   **Customizable OSD**: A modern, non-intrusive On-Screen Display shows track information, album art, and the current volume level without stealing focus.
*   **Audio Ducker (Windows only)**: Automatically lowers Spotify's volume when other applications (e.g., Discord, games) are making noise. The volume is restored after a configurable period of silence.
*   **System Tray Integration**: Control playback and access settings from a convenient tray icon.
*   **Cross-Platform**: A single codebase supporting Windows, macOS, and Linux (X11).
*   **No-Focus Overlay**: The OSD appears over your active window (including full-screen games) without interrupting your workflow.

## Setup & Configuration

This application uses the Spotify Web API. You must register your own "app" with Spotify to get the necessary credentials.

### 1. Create a Spotify App

1.  Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard/applications) and log in.
2.  Click `CREATE AN APP`.
3.  Give it a name (e.g., "My Volume Control") and a description.
4.  Once created, you will see your **Client ID** and **Client Secret**. You will need these in the next step.
5.  Go to `Edit Settings`. In the `Redirect URIs` field, add `http://localhost:8888/callback`. This must match the port in your `.env` file.
6.  Click `Save`.

### 2. Configure Credentials

The project uses a `.env` file to manage API credentials at build time. This file is not tracked by git.

1.  In the project's root directory, copy the example file:
    ```bash
    cp .env.example .env
    ```
2.  Open the new `.env` file in a text editor.
3.  Paste your Client ID and Client Secret from the Spotify Developer Dashboard.

    ```dotenv
    # .env
    SPOTIFY_CLIENT_ID="YOUR_CLIENT_ID_HERE"
    SPOTIFY_CLIENT_SECRET="YOUR_CLIENT_SECRET_HERE"
    POLL_INTERVAL_MS=2000
    APP_REDIRECT_PORT=8888
    ```

The app will now be compiled with your credentials. If you change them, you must rebuild the project.

## Building from Source

This project is built using CMake. Ensure you have Qt 6 (with Widgets, Network, Gui modules), CMake, and a C++ compiler installed.

### Windows (MinGW)

These instructions assume a MinGW-based Qt installation.

```powershell
# 1. Add MinGW to your PATH for this session
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path

# 2. Configure the project
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe

# 3. Build the executable
cmake --build build

# 4. Deploy Qt runtime DLLs
C:/Qt/6.10.2/mingw_64/bin/windeployqt.exe --release --force --dir build build/SpotifyVol.exe
```

The final executable will be at `build/SpotifyVol.exe`.

### macOS

The app builds into a standard `.app` bundle.

```bash
# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The final bundle will be at `build/SpotifyVol.app`. On first run, macOS will prompt for "Input Monitoring" and "Accessibility" permissions, which are required for global hotkey interception.

### Linux (X11)

Ensure you have Qt 6 development packages (`Widgets`, `Gui`, `Network`) and X11 development headers (`libx11-dev`) installed.

```bash
# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The final executable will be at `build/SpotifyVol`. Global key interception is supported on X11 sessions but is not available on Wayland.

## Usage

1.  Run the executable (`SpotifyVol.exe`, `open build/SpotifyVol.app`, or `./build/SpotifyVol`).
2.  On the first launch, you will be prompted to authorize the app with your Spotify account in your web browser.
3.  Once authorized, the app will run in the system tray.
4.  Use your keyboard's media keys (Volume Up/Down) to control Spotify's volume. The OSD will appear.
5.  Right-click the tray icon to access settings, re-link your account, or quit the application.

## Audio Ducker (Windows)

The Audio Ducker is a powerful feature for gamers or anyone who uses voice chat. It monitors system audio and automatically reduces Spotify's volume when another application is making sound, then restores it afterward.

**Configuration:**

*   **Mode**:
    *   **Monitor Process**: Ducks Spotify only when a specific application (e.g., `Discord.exe`, `CSGO.exe`) is making sound.
    *   **Monitor Entire Output**: Ducks Spotify whenever *any* sound is played on a specific audio device.
*   **Settings**: All parameters, including ducked volume, thresholds, and timings, are configurable in the app's settings dialog.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.