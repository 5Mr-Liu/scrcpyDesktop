#include "scrcpyoptions.h"

ScrcpyOptions::ScrcpyOptions()
{
    // --- 设置默认值 ---
    // 核心
    version = "3.3.3"; // !! 重要：请根据你的 scrcpy-server.jar 版本修改
    logLevel = "info";
    tunnel_forward = true;

    // 视频
    video = true;
    video_bit_rate = 8000000; // 8 Mbps
    video_codec = "h264";
    max_size = 0; // 0 表示不限制
    max_fps = 0;  // 0 表示不限制

    // 音频 (默认关闭，因为我们只做了视频解码)
    audio = false;
    audio_bit_rate = 128000;
    audio_codec = "opus";

    // 控制 (暂时关闭，因为我们还没实现控制)
    control = false;
    show_touches = false;
    clipboard_autosync = true; // 剪贴板可以先开着

    // 电源
    stay_awake = false;
    power_off_on_close = false;
}

QStringList ScrcpyOptions::toAdbShellArgs() const
{
    QStringList args;
    args << "shell"
         << "CLASSPATH=/data/local/tmp/scrcpy-server.jar"
         << "app_process"
         << "/"
         << "com.genymobile.scrcpy.Server"
         << version;

    // --- 动态添加参数 ---

    // 核心参数
    if (!logLevel.isEmpty()) {
        args << QString("log_level=%1").arg(logLevel);
    }
    if (tunnel_forward) {
        args << "tunnel_forward=true";
    }

    args << "send_dummy_byte=true";

    // 视频参数
    if (!video) { // 只有禁用时才发送 "video=false"
        args << "video=false";
    }
    if (video_bit_rate > 0) {
        args << QString("video_bit_rate=%1").arg(video_bit_rate);
    }
    if (!video_codec.isEmpty() && video_codec != "h264") { // h264 是默认值，可以不发
        args << QString("video_codec=%1").arg(video_codec);
    }
    if (max_size > 0) {
        args << QString("max_size=%1").arg(max_size);
    }
    if (max_fps > 0) {
        args << QString("max_fps=%1").arg(max_fps);
    }
    if (!crop.isEmpty()) {
        args << QString("crop=%1").arg(crop);
    }

    // 音频参数
    if (!audio) {
        args << "audio=false";
    }
    // ... 其他音频参数逻辑 ...

    // 控制参数
    if (!control) {
        args << "control=false";
    }
    if (show_touches) {
        args << "show_touches=true";
    }
    if (!clipboard_autosync) {
        args << "clipboard_autosync=false";
    }

    // 电源参数
    if (stay_awake) {
        args << "stay_awake=true";
    }
    if (power_off_on_close) {
        args << "power_off_on_close=true";
    }

    return args;
}
