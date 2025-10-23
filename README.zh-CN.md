# Scrcpy Desktop - 多设备 Scrcpy 桌面客户端

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-blue)]()
[![Qt Version](https://img.shields.io/badge/Qt-6.x%2B-green.svg)]()
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

**[English](./README.md) | 中文**

<img  alt="Image" src="https://private-user-images.githubusercontent.com/98318399/504505816-99de7346-5355-4d77-a280-7299680513a1.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjExODk0ODMsIm5iZiI6MTc2MTE4OTE4MywicGF0aCI6Ii85ODMxODM5OS81MDQ1MDU4MTYtOTlkZTczNDYtNTM1NS00ZDc3LWEyODAtNzI5OTY4MDUxM2ExLnBuZz9YLUFtei1BbGdvcml0aG09QVdTNC1ITUFDLVNIQTI1NiZYLUFtei1DcmVkZW50aWFsPUFLSUFWQ09EWUxTQTUzUFFLNFpBJTJGMjAyNTEwMjMlMkZ1cy1lYXN0LTElMkZzMyUyRmF3czRfcmVxdWVzdCZYLUFtei1EYXRlPTIwMjUxMDIzVDAzMTMwM1omWC1BbXotRXhwaXJlcz0zMDAmWC1BbXotU2lnbmF0dXJlPTQxYzkzOWY0YWM1MGNlYjI4NzM5ZDRkN2NkMTg0YzIxOGFlYTVlZjIzYjY0MTQ5Nzk0OWI5NjI4Y2MzYWIwOWEmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0In0.SMYFG2p4NPXH3lalnGexfUptyzGXJHL8ADUf0lqa9QM"/>

**Scrcpy Desktop** 是一个基于 [scrcpy](https://github.com/Genymobile/scrcpy) 构建的功能强大的桌面图形界面客户端。它使用 C++ 和 Qt 框架开发，旨在提供一个直观、高效的方式来同时管理和控制多个安卓设备。

本项目是一个独立的图形界面客户端，其核心镜像与控制功能完全依赖于杰出的开源项目 [**scrcpy**](https://github.com/Genymobile/scrcpy)，由 [Genymobile](https://www.genymobile.com/) 开发。我们对 Genymobile 团队创造出如此优秀和高性能的工具表示由衷的感谢！


## 📑 目录

- [✨ 主要功能](#-主要功能)
- [📦 安装与依赖](#-安装与依赖)
  - [下载预编译版本 (推荐)](#下载预编译版本-推荐)
  - [从源码编译](#从源码编译)
    - [1. 依赖项](#1-依赖项)
    - [2. Scrcpy-Server](#2-scrcpy-server)
    - [3. 编译步骤](#3-编译步骤)
- [🚀 如何使用](#-如何使用)
- [🏗️ 项目架构](#️-项目架构)
- [📄 许可证](#-许可证)
- [🙏 致谢](#-致谢)

## ✨ 主要功能

- **多设备管理**: 在一个界面中同时查看和管理所有通过 USB 或 Wi-Fi 连接的设备。
- **高性能镜像**: 利用 FFmpeg 进行硬件加速解码，提供低延迟、高帧率的屏幕镜像。
- **实时设备控制**: 通过鼠标和键盘无缝控制你的设备，支持点击、滚动、文本输入等。
- **丰富的配置选项**: 图形化界面让你轻松配置 scrcpy 的各项参数，无需记忆复杂的命令行。
  - **视频**: 分辨率、比特率、帧率、视频源（屏幕/摄像头）、编码器 (H.264/H.265/AV1)。
  - **音频**: 音频转发（需要 Android 11+）、音频源、比特率、编码器。
  - **控制**: 触摸显示、保持唤醒、关闭时息屏等。
  - **录制**: 一键录制屏幕为 MP4 或 MKV 文件。
- **设备操作工具栏**: 在每个设备窗口中都提供了便捷的工具栏，用于执行常用操作（电源、音量、旋转、Home、返回、截屏等）。
- **无线连接助手**: 简化了通过 Wi-Fi 连接设备的流程，包括一键开启 TCP/IP 模式。
- **状态监控**: 在表格视图中实时监控所有已连接设备的状态（分辨率、连接方式等）。
- **跨平台支持**: 可在 Windows, macOS, 和 Linux 上编译和运行。
- **配置文件**: 保存和加载你的常用配置，方便在不同场景间快速切换。

## 📦 安装与依赖

### 下载预编译版本 (推荐)

对于大多数用户，我们推荐直接从 **[Releases](https://github.com/your-username/your-repo-name/releases)** 页面下载适用于您操作系统的最新版本。

### 从源码编译

如果您希望自行编译，请确保以下依赖项已正确安装并配置。

#### 1. 依赖项

- **ADB (Android Debug Bridge)**: 必须安装并将其路径添加到系统的 `PATH` 环境变量中。
  - 从 [Android SDK Platform Tools](https://developer.android.com/studio/releases/platform-tools) 下载。
  - 验证安装：在终端或命令提示符中运行 `adb --version`。
- **Qt 框架 (6.x)**: 本项目使用 Qt 进行开发。
  - 下载 [Qt Online Installer](https://www.qt.io/download-qt-installer

-   在安装过程中，请确保选择与你的编译器匹配的 Qt 版本（例如，在 Windows 上选择 MinGW 或 MSVC，在 macOS 上选择 Clang，在 Linux 上选择 GCC）。
-   **FFmpeg 库 (dev/shared)**: 用于解码 scrcpy 传输的视频流。
    -   **Windows**: 从 [FFmpeg for Windows](https://www.gyan.dev/ffmpeg/builds/) 下载 `full_build-shared.7z` 版本。将 `include` 和 `lib` 目录的内容复制到你的项目或编译器路径中，并将 `bin` 目录下的 `.dll` 文件（如 `avcodec-*.dll`, `avutil-*.dll`等）复制到本程序最终生成的可执行文件（`.exe`）所在的目录。
    -   **macOS**: 使用 Homebrew 安装: `brew install ffmpeg`
    -   **Linux (Ubuntu/Debian)**: 使用 apt 安装: `sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev libavutil-dev`

#### 2. Scrcpy-Server

本项目需要 `scrcpy-server` 文件来在安卓设备上运行服务。
1.  前往 [Scrcpy Releases](https://github.com/Genymobile/scrcpy/releases) 页面。
2.  下载与代码兼容的 `scrcpy-server` 文件（请检查 `scrcpyoptions.h` 或相关配置文件中定义的版本，例如 **v3.3.3**）。
3.  将下载的文件**重命名**为 `scrcpy-server` 并放置在**本程序生成的可执行文件**旁边。

#### 3. 编译步骤

##### 使用 Qt Creator (推荐)

1.  克隆本仓库：`git clone https://github.com/your-username/your-repo-name.git`
2.  使用 Qt Creator 打开项目根目录下的 `.pro` 文件。
3.  配置项目以使用你已安装的 Qt Kit。
4.  点击左下角的 "构建" (Build) 按钮，然后点击 "运行" (Run) 按钮。

**注意**: 请确保在运行前已将 `scrcpy-server` 文件和 FFmpeg 的 `.dll` 文件（仅限 Windows）放置在可执行文件旁边。

## 🚀 如何使用

1.  **启动程序**: 运行已编译的可执行文件。
2.  **连接设备**:
    -   **USB 连接**:
        1.  在你的安卓设备上启用 "开发者选项" 和 "USB 调试"。
        2.  通过 USB 数据线将设备连接到电脑。
        3.  在程序左侧面板点击 "刷新 USB 设备"。
        4.  你的设备应该会出现在列表中，状态为 `device`。如果显示为 `unauthorized`，请检查你的设备屏幕并授权 USB 调试。
        5.  选中你的设备，然后点击 "连接 USB 设备" 按钮。
    -   **Wi-Fi 连接 (无线)**:
        1.  首先，通过 USB 将设备连接到电脑。
        2.  在 "USB 设备" 列表中选中该设备，然后点击 "启用 TCP/IP 模式"。
        3.  断开 USB 连接。
        4.  在手机的 "设置" -> "关于手机" -> "状态信息" 中找到设备的 IP 地址。
        5.  在程序的 "Wi-Fi 连接" 选项卡中输入该 IP 地址，然后点击 "通过 Wi-Fi 连接"。
3.  **配置选项**:
    -   在连接设备**之前**，你可以在程序右侧的 "Scrcpy 参数配置" 面板中设置所有参数。
    -   展开 "视频设置", "音频设置" 等区域来调整你需要的选项。
4.  **设备窗口**:
    -   连接成功后，会弹出一个新的窗口，显示设备的实时屏幕。
    -   使用鼠标点击、拖动和滚轮来模拟触摸操作。
    -   使用电脑键盘进行文本输入。
    -   使用窗口顶部的工具栏来执行快速操作（Home, Back, 锁屏等）。
5.  **保存/加载配置**:
    -   在 "文件" 菜单中，你可以将当前的所有参数配置保存为一个 `.ini` 文件。
    -   之后，你可以通过 "加载配置" 快速恢复之前的设置。

## 🏗️ 项目架构

本项目采用模块化设计，将不同功能分离到各自的类中，以提高代码的可读性和可维护性。

-   `MainWindow`: 应用程序的主窗口，负责管理整体 UI、用户交互和启动设备连接。
-   `DeviceManager`: 负责通过 `adb devices` 命令异步发现和更新连接的设备列表。
-   `DeviceWindow`: 每个设备连接的核心。它管理单个设备的整个生命周期，包括推送服务、建立连接、显示视频和处理用户输入。
-   `AdbProcess`: `QProcess` 的一个封装类，简化了执行 `adb` 命令的过程。
-   `ScrcpyOptions`: 一个数据结构类，用于收集 UI 上的所有配置，并能生成启动 scrcpy-server 所需的命令行参数。
-   `VideoDecoderThread`: 一个专用的 `QThread`，使用 FFmpeg 库来高效地解码从设备接收到的视频流，确保 UI 的流畅性。
-   `ControlSender`: 负责将鼠标和键盘的输入事件序列化为 scrcpy 协议格式，并通过一个独立的 TCP 套接字发送到设备。
-   `UiStateManager`: 管理主窗口 UI 控件之间的联动逻辑（例如，选中 "禁用视频" 时，自动禁用所有视频相关选项）。

## 📄 许可证

本项目基于 [MIT License](LICENSE) 授权。详情请见 `LICENSE` 文件。

## 🙏 致谢

-   **[scrcpy](https://github.com/Genymobile/scrcpy)**: 本项目得以实现的核心。感谢 Genymobile 团队的杰出工作。
-   **[Qt Framework](https://www.qt.io/)**: 提供了强大的跨平台 GUI 开发工具。
-   **[FFmpeg](https://ffmpeg.org/)**: 提供了无与伦比的音视频处理能力。
-   **所有贡献者和用户**: 感谢你们的支持和反馈！

