# SpotifyVol

A lightweight, cross-platform Spotify controller with a custom volume OSD and audio ducking. It runs in your system tray and uses global media keys to control Spotify's playback volume, not your system volume.



## Features

*   **Global Hotkey Control**: Intercepts system media keys (Volume Up/Down) to control Spotify's application volume directly. Use Shift for fine-grained adjustments.
*   **Customizable OSD**: A modern, non-intrusive On-Screen Display shows track information, album art, and the current volume level without stealing focus.
*   **Audio Ducker (Windows only)**: Automatically lowers Spotify's volume when other applications (e.g., Discord, games) are making noise. The volume is restored after a configurable period of silence.
*   **System Tray Integration**: Control playback and access settings from a convenient tray icon.
*   **Cross-Platform**: A single codebase supporting Windows, macOS, and Linux (X11).
*   **No-Focus Overlay**: The OSD appears over your active window (including full-screen games) without interrupting your workflow.

## Setup

No Spotify API keys, developer app, or `.env` file are required. SpotifyVol authenticates using Spotify's OAuth2 **device flow** with the built-in Spotify desktop client id.

On first launch — or any time from **Settings → Connect Spotify** — your browser opens a Spotify authorization page. Approve it and the app connects. Your login is remembered afterwards (a refresh token is stored locally in your OS settings store), so you won't need to re-authorize on later launches.

The app never handles your password or any cookie. Once connected it talks only to Spotify's realtime Connect backend (the same protocol the official clients use), streaming playback state over a WebSocket rather than polling the public Web API — so it isn't subject to Web-API rate limits.

## Building from Source

This project is built using CMake. You need Qt 6 (Widgets, Network, Gui, WebSockets), Protobuf (`protoc` plus libprotobuf), zlib, CMake, a C++17 compiler, and — on Linux — X11 development headers.

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

Ensure you have Qt 6 development packages (`Widgets`, `Gui`, `Network`, `WebSockets`), Protobuf (`protobuf-compiler`, `libprotobuf-dev`), zlib (`zlib1g-dev`), and X11 development headers (`libx11-dev`) installed.

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