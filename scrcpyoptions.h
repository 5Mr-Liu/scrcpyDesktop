#ifndef SCRCPYOPTIONS_H
#define SCRCPYOPTIONS_H

#include <QString>
#include <QStringList>

class ScrcpyOptions
{
public:
    ScrcpyOptions();

    // 将所有选项转换为 adb shell 参数列表
    QStringList toAdbShellArgs() const;

    // --- 核心与通用参数 ---
    QString version;
    QString logLevel;
    bool tunnel_forward;

    // --- 视频参数 ---
    bool video;
    QString video_source; // display, camera
    quint32 video_bit_rate;
    QString video_codec;
    quint16 max_size;
    quint16 max_fps;
    quint8 display_id;
    QString crop;
    // ... 其他视频参数可以按需添加

    // --- 音频参数 ---
    bool audio;
    quint32 audio_bit_rate;
    QString audio_codec;
    QString audio_source;
    quint16 audio_buffer;
    bool audio_dup;
    bool require_audio;
    // ... 其他音频参数

    // --- 摄像头参数 (仅当 video_source == "camera" 时有效) ---
    QString camera_id;
    QString camera_facing; // front, back, external
    QString camera_size; // "width:height"
    quint8 camera_fps;
    bool camera_high_speed;

    // --- 控制与交互参数 ---
    bool control;
    bool show_touches;
    bool clipboard_autosync;
    QString keyboard_mode; // sdk, uhid, aoa, disabled
    QString mouse_mode; // sdk, uhid, aoa, disabled
    bool otg;


    // --- 设备与电源管理参数 ---
    bool stay_awake;
    bool power_off_on_close;

    // --- 窗口与录制参数 ---
    // 下面这些参数是PC客户端用的，不会传递给 scrcpy-server
    // 我们在 DeviceWindow 中处理它们，但为了统一管理，也放在这里
    bool fullscreen;
    bool always_on_top;
    bool window_borderless;
    QString window_title;
    // 下面这些参数会传递给 scrcpy-server
    QString record_file;
    QString record_format; // mp4, mkv, ...
    bool no_playback; // -N, 录制但不显示/播放
    bool no_video_playback;
};

#endif // SCRCPYOPTIONS_H
