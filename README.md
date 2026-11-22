<p align="center">
  <h1 align="center">TachyonDesk</h1>
  <p align="center">
    High-performance, single-port Windows remote desktop with GPU video streaming, single-window control, remote audio, and modern file transfer.
  </p>
  <p align="center">
    <a href="README_CN.md">简体中文</a> ·
    <a href="BUILDING.md">Build Guide</a> ·
    <a href="SECURITY.md">Security</a> ·
    <a href="PORTABLE_MODE.md">Portable &amp; Service Modes</a> ·
    <a href="docs/design.md">Architecture Design (CN)</a>
  </p>
</p>

---

## Overview

**TachyonDesk** is an open-source Windows remote desktop suite (Server, Viewer, and Repeater) forked from [UltraVNC](https://github.com/ultravnc/UltraVNC) (baseline `7bd294a`) and licensed under **GPL-3.0-or-later**.

While preserving full compatibility with the standard RFB (VNC) protocol and single-TCP-port deployment, TachyonDesk adds a negotiated **multiplexed extension channel** (`rfbExtMsg`) with priority scheduling (Audio > Control > Video > Bulk) to deliver modern remote desktop capabilities over a single port—without breaking interoperability with standard VNC clients or servers.

## Key Features

### Core Components
- **TachyonDesk Server (`TachyonServer.exe`)**: Windows desktop & service host (`TachyonDeskService`), supporting hybrid, portable (`tachyon.portable`), and system-service configuration modes.
- **TachyonDesk Viewer (`TachyonViewer.exe`)**: Native Win32 remote client with Simplified/Traditional Chinese & English UI support for all extended features.
- **TachyonDesk Repeater (`TachyonRepeater.exe`)**: Standalone relay server supporting Mode I/II reverse connections and automatic server-side reconnection (`[auto_reverse]`).

### Extended Capabilities (Single TCP Port)
| Feature | Description | Status |
|---|---|---|
| **Multiplexed Extension Channel** | Capability-negotiated `rfbExtMsg` channel with 64 KB chunking, 4-level priority queues, and RTT measurement. | Stable |
| **Bidirectional Lazy File Clipboard** | `Ctrl+C` / `Ctrl+V` files & folders between local and remote Explorer via OLE virtual files (`IStream`), streamed on demand with native progress & cancellation. | Stable |
| **GPU Video Mode (H.264 / HEVC / AV1)** | DXGI Desktop Duplication zero-copy capture + NVIDIA NVENC hardware encoding (with Media Foundation H.264 fallback for non-NVIDIA GPUs), AIMD adaptive bitrate control, remote cursor shape sync, and D3D11VA hardware decoding. | Stable (H.264/HEVC) / Experimental (AV1 & MF H.264) |
| **Single-Window Remote Control** | Capture and control a single remote window (WGC / `PrintWindow` + popup menu/dialog compositing + input filtering) in lossless zstd tile mode or NVENC video mode. | Stable |
| **Remote Audio** | WASAPI loopback capture (with optional remote local mute) + Media Foundation AAC-LC encoding + adaptive jitter-buffered playback. | Stable |
| **Modern File Manager & File-Only Mode** | Dual-pane local/remote file manager, transfer queue, resume interrupted transfers (`~tachyon-*.part`), drag remote items directly to local Explorer, and `/filemode` (no framebuffer traffic). | Stable (Core) / Experimental (Resume & Explorer DnD) |
| **Media Settings & Auto-Restore** | Configurable max bitrate, FPS (30/60/120), codec preference, audio bitrate, and automatic video/audio restoration after reconnect. | Experimental |
| **Auto Reverse Repeater** | Server-side automatic reverse connection & reconnect loop to `TachyonRepeater` via `tachyon.ini` (`[auto_reverse]`) or CLI flags. | Stable |

## Quick Start

### 1. Portable Mode
Create an empty file named `tachyon.portable` next to `TachyonServer.exe` to store `tachyon.ini` directly in the application folder. See [PORTABLE_MODE.md](PORTABLE_MODE.md) for hybrid and service-mode details.

### 2. File-Transfer-Only Mode
Launch `TachyonViewer.exe /filemode` (or check **File transfer only** in the connection dialog) to connect directly to the dual-pane file manager without requesting screen updates or sending keyboard/mouse input.

### 3. Automatic Reverse Connection (Repeater)
Add the following section to `tachyon.ini` on the server:
```ini
[auto_reverse]
AutoReverseEnabled=1
AutoReverseRepeaterHost=your-repeater-host
AutoReverseRepeaterPort=5500
AutoReverseId=ID:123456
AutoReverseReconnectInterval=5
```
Or pass CLI flags in application mode:
```text
TachyonServer.exe -autoreverse -repeater your-repeater-host:5500 -id:123456 -reconnect 5 -run
```

## Building from Source

TachyonDesk targets **Windows 10 / 11 (x64)** with **Visual Studio 2022 or newer** (C++ desktop workload).

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Build outputs are placed in `build\bin\x64\Release\`. For full requirements, toolset options, and vcpkg notes, see [BUILDING.md](BUILDING.md).

## Roadmap

- [ ] **AI Agent Remote Computer Control (High Priority)** — Expose a structured, permission-scoped interface (e.g., MCP / local API) allowing AI agents to inspect the remote screen or target window, perform mouse/keyboard actions, and transfer files safely with human-in-the-loop oversight
- [ ] **Complete non-NVIDIA hardware encoding** (AMD AMF / Intel QSV / Media Foundation H.264 & HEVC stabilization)
- [ ] **Optional UDP video transport** (same port number as TCP, with FEC / lower latency on lossy networks)
- [ ] **Microphone redirection** (Viewer -> Server audio channel)
- [ ] **Multi-monitor switching in Video Mode** & smoother DPI/HDR handling
- [ ] **CI/CD & Automated Release Packaging** (GitHub Actions + signed/portable x64 releases)
- [ ] **Cross-platform Viewer exploration** (Linux / macOS client supporting the `rfbExtMsg` protocol)

## Security

If you expose TachyonDesk over the public internet, **always enable transport encryption** (TLS / RSA-AES / DSM plugin) or tunnel through a VPN. Please read [SECURITY.md](SECURITY.md) before deploying in production or service mode.

## License & Acknowledgments

TachyonDesk is free software distributed under the **GNU General Public License v3.0 or later** ([LICENSE](LICENSE)).

TachyonDesk is derived from [UltraVNC](https://github.com/ultravnc/UltraVNC) (Copyright © 2002–2025 UltraVNC Team Members, and 1999–2002 Vdacc-VNC & eSVNC Projects). All upstream copyright notices and GPL headers are preserved in the source tree.
