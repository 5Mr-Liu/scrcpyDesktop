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
    quint32 video_bit_rate;
    QString video_codec;
    quint16 max_size;
    quint16 max_fps;
    QString crop;
    // ... 其他视频参数可以按需添加

    // --- 音频参数 ---
    bool audio;
    quint32 audio_bit_rate;
    QString audio_codec;
    // ... 其他音频参数

    // --- 控制与交互参数 ---
    bool control;
    bool show_touches;
    bool clipboard_autosync;

    // --- 设备与电源管理参数 ---
    bool stay_awake;
    bool power_off_on_close;
};

#endif // SCRCPYOPTIONS_H
