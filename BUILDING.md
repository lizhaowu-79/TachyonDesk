# Building TachyonDesk

TachyonDesk targets **Windows 10 / 11 (x64)**.

## Prerequisites

- **Visual Studio 2022** (toolset `v143`) or **Visual Studio 2026 / v18** (toolset `v145`) with the **Desktop development with C++** workload and **C++ MFC** (for resource header `afxres.h` / `winres.h`).
- **Windows 10 / 11 SDK** (10.0.22000+ recommended for Windows.Graphics.Capture borderless capture support).
- **Python 3** (optional, only needed if you modify viewer `Tr()`/`TrF()` UI strings and want to regenerate `viewer/UiTextTable.inc`).

## Quick Build (Release | x64)

Clone the repository **with submodules** (`3rdparty/noVNC` is embedded by `TachyonServer.exe` resources):

```powershell
git clone --recursive https://github.com/lizhaowu-79/TachyonDesk.git
cd TachyonDesk
```

The repository bundles prebuilt x64 Release third-party dependencies under `3rdparty/` (`zlib`, `zstd`, `libjpeg-turbo`, `liblzma`, `libsodium`, `minizip-ng`, `libyuv`, `ffnvcodec`), so `Release | x64` builds out of the box without running vcpkg:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

### Build Options
- **Clean Rebuild**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\build.ps1 -Rebuild
  ```
- **Custom Output Directory** (e.g. RAM disk):
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\build.ps1 -BuildRoot Z:\TachyonDesk-build
  ```
- **Explicit Platform Toolset**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\build.ps1 -PlatformToolset v143
  ```

### Build Outputs
Compiled binaries and runtime DLLs are written to `<BuildRoot>\bin\x64\Release\`:
- `TachyonViewer.exe`
- `TachyonServer.exe`
- `TachyonRepeater.exe`
- Required runtime DLLs (`libsodium.dll`, `zlib1.dll`, `zstd.dll`, `liblzma.dll`, `jpeg62.dll`, `bz2.dll`)

## Building Individual Solutions in Visual Studio

You can also open any of the three solutions directly in Visual Studio:
- `viewer/TachyonViewer.sln`
- `server/TachyonServer.sln`
- `repeater/TachyonRepeater.sln`

Build paths are configured via [Directory.Build.props](Directory.Build.props) and [BuildOutput.targets](BuildOutput.targets).

## Updating Viewer UI Translations

After editing any `Tr(L"...")` or `TrF(L"...")` string in `viewer/`:
```powershell
python viewer\uitext\gen.py
```
The script verifies that all keys have Simplified/Traditional Chinese translations and matching format placeholders, then regenerates `viewer/UiTextTable.inc`.
