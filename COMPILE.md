# Compile Guide (Windows + Qt MinGW)

This project is configured to build with CMake and Ninja.
Your Qt installation is MinGW-based, so use the MinGW compiler toolchain.

## Prerequisites

- Qt MinGW installed at: `C:\Qt\6.10.2\mingw_64`
- MinGW toolchain installed at: `C:\Qt\Tools\mingw1310_64\bin`
- CMake and Ninja available in PATH

## Build Commands (PowerShell)

Run these from the project root.

```powershell
# 1) Ensure MinGW runtime tools are available in this shell session
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path

# 2) Configure a MinGW build directory
cmake -S . -B build-mingw -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe

# 3) Build
cmake --build build-mingw -j

# 4) Deploy Qt runtime DLLs next to the exe
C:/Qt/6.10.2/mingw_64/bin/windeployqt.exe --release --force --dir build-mingw build-mingw/SpotifyVol.exe
```

Output executable:

- `build-mingw/SpotifyVol.exe`

## Rebuild After Changes

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path
cmake --build build-mingw -j
C:/Qt/6.10.2/mingw_64/bin/windeployqt.exe --release --force --dir build-mingw build-mingw/SpotifyVol.exe
```

## Run

```powershell
./build-mingw/SpotifyVol.exe
```

## Clean Reconfigure

If CMake cache/toolchain gets mixed up, delete and reconfigure:

```powershell
Remove-Item -Recurse -Force build-mingw
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path
cmake -S . -B build-mingw -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe
cmake --build build-mingw -j
```

## Common Pitfalls

- Do not run `make` for this setup.
  - Ninja generator does not produce a Makefile.
  - Use `cmake --build build-mingw` instead.

- If you see `Qt6Gui.dll`, `Qt6Core.dll`, `Qt6Network.dll`, or `Qt6Widgets.dll` missing:
  - Run `windeployqt` (step 4 above) to copy Qt runtime DLLs and plugins.

- If compiler checks fail unexpectedly, make sure MinGW is on PATH in the same terminal session:

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;" + $env:Path
```

# macOS Build

On macOS, the app now builds as a proper `.app` bundle instead of a terminal-style executable.

## Build Commands (macOS)

Run these from the project root.

```bash
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac -j
```

Bundle output:

- `build-mac/SpotifyVol.app`

Run it with:

```bash
open build-mac/SpotifyVol.app
```

# Linux Build

Linux can build the Qt app already, but the current codebase does not yet implement native global volume-key interception there. In practice, that means the Spotify UI, OSD, tray icon, auth flow, and playback polling can work on Linux, but media-key suppression/hijacking is still a future platform-specific task.

## Build Commands (Linux)

Run these from the project root.

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j
```

Output executable:

- `build-linux/SpotifyVol`

Run it with:

```bash
./build-linux/SpotifyVol
```

## Linux Notes

- Install Qt 6 development packages for `Widgets`, `Gui`, and `Network`.
- Install X11 development headers/libraries so the global volume-key hook can build.
- A system tray is required for the tray icon to appear normally.
- Global volume-key interception is implemented for X11 sessions.
- Wayland sessions usually do not allow global key suppression, so the Linux hook will log that limitation and remain disabled there.
