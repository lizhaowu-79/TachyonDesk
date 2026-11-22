<p align="center">
  <h1 align="center">TachyonDesk（快子桌面）</h1>
  <p align="center">
    单端口高性能 Windows 远程桌面套件：GPU 硬件视频流、单窗口远控、远程声音与现代文件传输。
  </p>
  <p align="center">
    <a href="README.md">English</a> ·
    <a href="BUILDING.md">编译指南</a> ·
    <a href="SECURITY.md">安全须知</a> ·
    <a href="PORTABLE_MODE.md">便携与服务模式</a> ·
    <a href="docs/design.md">架构设计文档</a>
  </p>
</p>

---

## 项目简介

**TachyonDesk** 是一款面向 Windows 10/11（x64）的开源远程桌面套件（包含服务端、查看器与中继器），基于 [UltraVNC](https://github.com/ultravnc/UltraVNC)（上游基线 `7bd294a`）深度二次开发，采用 **GPL-3.0-or-later** 协议开源。

项目坚持 **单 TCP 端口** 与 **标准 RFB 协议向后兼容** 的设计原则：通过能力协商的双向扩展消息通道（`rfbExtMsg`）及四级优先级调度（音频 > 控制 > 视频 > 批量文件），在同一条加密连接中承载现代远控所需的全部媒体与文件能力；连接原版 VNC 客户端或服务端时自动回退为标准模式。

## 核心特性

### 三大组件
- **服务端（`TachyonServer.exe`）**：支持应用模式、便携模式（`tachyon.portable`）与 Windows 系统服务模式（`TachyonDeskService`），配置与原版 UltraVNC 完全隔离。
- **查看器（`TachyonViewer.exe`）**：原生 Win32 客户端，所有新增功能均内置简体中文、繁体中文与英文界面支持。
- **中继器（`TachyonRepeater.exe`）**：独立中继服务，配合服务端 `[auto_reverse]` 实现内网穿透与断线自动重连。

### 单端口扩展功能一览
| 功能模块 | 说明 | 当前状态 |
|---|---|---|
| **多路复用扩展通道** | 能力协商、64 KB 分片、四级优先级队列（音频/控制/视频/文件）、Ping/Pong 实时 RTT 测量。 | 稳定 |
| **双向文件复制粘贴** | 本地与远程资源管理器之间直接 `Ctrl+C` / `Ctrl+V` 文件和文件夹，基于 OLE 虚拟文件（`IStream`）粘贴时按需拉取，支持系统原生进度条与取消。 | 稳定 |
| **显卡视频模式（H.264 / HEVC / AV1）** | DXGI Desktop Duplication 零拷贝抓屏 + NVIDIA NVENC 硬件编码（并支持 Media Foundation H.264 编码），AIMD 自适应码率控制、远程光标形状同步、D3D11VA 硬件解码。 | H.264/HEVC 稳定；AV1 与 MF H.264 实验性 |
| **单窗口远控** | 仅捕获并控制远程单个窗口（WGC / `PrintWindow` + 弹出菜单/对话框合成 + 输入过滤），支持无损分块（zstd）与 NVENC 视频流两种传输模式。 | 稳定 |
| **远程声音** | WASAPI loopback 采集（支持远程本机静音）+ Media Foundation AAC-LC 编解码 + 自适应抖动缓冲播放。 | 稳定 |
| **现代文件管理器与纯文件模式** | 左右双栏本机/远程浏览、传输队列、断点续传（`~tachyon-*.part`）、直接从远程拖放虚拟文件到 Windows 资源管理器；支持 `/filemode` 仅文件连接模式（不传画面、不发键鼠输入）。 | 核心稳定；断点续传与拖放至资源管理器实验性 |
| **媒体设置与重连恢复** | 支持配置最大码率、帧率（30/60/120）、首选编码格式、声音码率，以及断线重连后自动恢复视频模式与远程声音。 | 实验性 |
| **自动反向中继** | 服务端支持通过 `tachyon.ini`（`[auto_reverse]`）或命令行自动连接 `TachyonRepeater` 并在断线后按间隔重连。 | 稳定 |

## 快速上手

### 1. 便携模式
在 `TachyonServer.exe` 同目录下新建空文件 `tachyon.portable`，服务端即可直接读写当前目录的 `tachyon.ini`。混合模式与系统服务模式的详细说明见 [PORTABLE_MODE.md](PORTABLE_MODE.md)。

### 2. 仅文件传输模式
使用命令行 `TachyonViewer.exe /filemode`，或在连接对话框勾选 **仅文件传输 / File transfer only**，连接后直接打开双栏文件传输窗口，不请求远程桌面画面。

### 3. 自动反向连接中继器
在服务端的 `tachyon.ini` 中加入：
```ini
[auto_reverse]
AutoReverseEnabled=1
AutoReverseRepeaterHost=your-repeater-host
AutoReverseRepeaterPort=5500
AutoReverseId=ID:123456
AutoReverseReconnectInterval=5
```
或以应用模式命令行启动：
```text
TachyonServer.exe -autoreverse -repeater your-repeater-host:5500 -id:123456 -reconnect 5 -run
```

## 从源码编译

需要 **Windows 10 / 11（x64）** 与 **Visual Studio 2022 或更新版本**（安装 C++ 桌面开发工作负载）：

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

默认输出至 `build\bin\x64\Release\`，完整说明请参考 [BUILDING.md](BUILDING.md)。

## 未来规划（Roadmap）

- [ ] **AI Agent 远程计算机操控（高优先级）**：提供结构化、可权限控制的接口（如 MCP / 本地 API），支持 AI Agent 观察远程桌面或指定单窗口画面、执行键鼠操作与文件交互，并配合人工确认与安全边界机制
- [ ] **完善非 NVIDIA 显卡硬件编码**：稳定 Media Foundation H.264 / HEVC，探索 AMD AMF / Intel QSV 支持
- [ ] **可选 UDP 视频通道**：复用同一端口号提供可选 UDP 传输与抗丢包优化，提升弱网游戏/视频体验
- [ ] **麦克风回传**：支持查看器 → 服务端的语音采集与播放
- [ ] **视频模式多显示器切换** 与高 DPI / HDR 体验优化
- [ ] **CI/CD 与自动打包发布**：配置 GitHub Actions 自动编译校验及 x64 便携包/安装包发布
- [ ] **跨平台查看器探索**：面向 Linux / macOS 的 `rfbExtMsg` 协议客户端

## 安全提示

如需在公网环境使用，**务必启用传输层加密**（TLS / RSA-AES / DSM 插件）或通过 VPN 访问。部署前请务必阅读 [SECURITY.md](SECURITY.md)。

## 开源协议与致谢

本项目基于 **GNU General Public License v3.0 or later** 协议开源（见 [LICENSE](LICENSE)）。

TachyonDesk 基于 [UltraVNC](https://github.com/ultravnc/UltraVNC) 二次开发（Copyright © 2002–2025 UltraVNC Team Members，以及 1999–2002 Vdacc-VNC & eSVNC Projects），源码中完整保留了上游版权与协议声明。
