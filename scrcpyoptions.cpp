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
    video_source = "display";
    video_codec = "h264";
    max_size = 0; // 0 表示不限制
    max_fps = 0;  // 0 表示不限制
    display_id = 0;
    video_bit_rate = 8000000; // 8 Mbps

    // 音频 (默认关闭，因为我们只做了视频解码)
    audio = true;
    audio_bit_rate = 128000;
    audio_codec = "opus";
    audio_buffer = 50;
    audio_dup = false;
    require_audio = false;

    camera_facing = "any";
    camera_fps = 0;
    camera_high_speed = false;

    // 控制 (暂时关闭，因为我们还没实现控制)
    control = true;
    audio_source = "output";
    show_touches = false;
    clipboard_autosync = true; // 剪贴板可以先开着

    // 电源
    stay_awake = false;
    power_off_on_close = false;
    keyboard_mode = "sdk";
    mouse_mode = "sdk";
    otg = false;

    fullscreen = false;
    always_on_top = false;
    window_borderless = false;

    record_format = "auto";
    no_playback = false;
    no_video_playback = false;
}

// in scrcpyoptions.cpp
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

    // ======================== 这是关键的修复 ========================
    // 视频参数
    if (video) {
        args << "video=true"; // 必须明确告诉服务器启动视频

        if (video_source != "display") {
            args << QString("video_source=%1").arg(video_source);
        }
        if (max_size > 0) {
            args << QString("max_size=%1").arg(max_size);
        }
        if (max_fps > 0) {
            args << QString("max_fps=%1").arg(max_fps);
        }
        // 只有当码率不是默认值时才发送，作为一种优化
        if (video_bit_rate != 8000000) {
            args << QString("video_bit_rate=%1").arg(video_bit_rate);
        }
        if (!video_codec.isEmpty() && video_codec != "h264") {
            args << QString("video_codec=%1").arg(video_codec);
        }
        if (display_id > 0) {
            args << QString("display_id=%1").arg(display_id);
        }
        if (no_video_playback) {
            args << "video_playback=false";
        }
        if (!crop.isEmpty()) {
            args << QString("crop=%1").arg(crop);
        }

        // 摄像头参数 (只有在 video_source='camera' 时才相关)
        if (video_source == "camera") {
            if (!camera_id.isEmpty()) {
                args << QString("camera_id=%1").arg(camera_id);
            }
            if (camera_facing != "any") {
                args << QString("camera_facing=%1").arg(camera_facing);
            }
            if (!camera_size.isEmpty()) {
                args << QString("camera_size=%1").arg(camera_size);
            }
            if (camera_fps > 0) {
                args << QString("camera_fps=%1").arg(camera_fps);
            }
            if (camera_high_speed) {
                args << "camera_high_speed=true";
            }
        }
    } else {
        args << "video=false";
    }
    // =============================================================

    // 音频参数
    if (audio) {
        args << "audio=true"; // 同样，明确告诉服务器
        if (audio_source != "output") {
            args << QString("audio_source=%1").arg(audio_source);
        }
        if (audio_bit_rate != 128000) {
            args << QString("audio_bit_rate=%1").arg(audio_bit_rate);
        }
        if (audio_codec != "opus") {
            args << QString("audio_codec=%1").arg(audio_codec);
        }
        if (audio_buffer != 50) {
            args << QString("audio_buffer=%1").arg(audio_buffer);
        }
        if (audio_dup) {
            args << "audio_dup=true";
        }
        if (require_audio) {
            args << "require_audio=true";
        }
    } else {
        args << "audio=false";
    }


    // 控制参数
    if (control) {
        args << "control=true";
    } else {
        args << "control=false";
    }
    if (show_touches) {
        args << "show_touches=true";
    }
    // 注意：scrcpy 2.0+ 的默认值是 true，所以只有当它为 false 时才需要发送
    // 但为了兼容性和明确性，可以始终发送
    args << QString("clipboard_autosync=%1").arg(clipboard_autosync ? "true" : "false");

    if (keyboard_mode != "sdk") {
        args << QString("keyboard=%1").arg(keyboard_mode);
    }
    if (mouse_mode != "sdk") {
        args << QString("mouse=%1").arg(mouse_mode);
    }
    if (otg) {
        args << "otg=true";
    }

    // 电源参数
    if (stay_awake) {
        args << "stay_awake=true";
    }
    if (power_off_on_close) {
        args << "power_off_on_close=true";
    }

    // 录制
    if (!record_file.isEmpty()) {
        // scrcpy 2.0+ 录制参数改名为 record_file
        args << QString("record_file=%1").arg(record_file);
        if (record_format != "auto") {
            args << QString("record_format=%1").arg(record_format);
        }
    }
    if (no_playback) {
        args << "no_playback=true";
    }

    return args;
}
