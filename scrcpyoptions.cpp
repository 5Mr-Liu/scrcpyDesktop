#include "scrcpyoptions.h"

// A constant for the temporary filename used for recordings on the device.
const QString DEVICE_RECORD_FILENAME = "temp_record_file";

ScrcpyOptions::ScrcpyOptions()
{
    // --- Set default values for all options ---
    // IMPORTANT: This version MUST match the version of the scrcpy-server.jar file you are using.
    version = "3.3.3"; // Example: updated to a recent scrcpy version.

    // Core
    logLevel = "info";
    tunnel_forward = true;

    // Video
    video = true;
    video_source = "display";
    video_codec = "h264";
    max_size = 0; // 0 means no limit.
    max_fps = 0;  // 0 means no limit.
    display_id = 0;
    video_bit_rate = 8000000; // 8 Mbps.

    // Audio
    audio = true;
    audio_source = "output";
    audio_bit_rate = 128000; // 128 kbps.
    audio_codec = "opus";
    audio_buffer = 50; // 50 ms.
    audio_dup = false;
    require_audio = false;

    // Camera (defaults)
    camera_facing = "any";
    camera_fps = 0;
    camera_high_speed = false;

    // Control
    control = true;
    show_touches = false;
    clipboard_autosync = true; // Clipboard sync is generally safe to enable by default.
    keyboard_mode = "sdk";
    mouse_mode = "sdk";
    otg = false;

    // Power Management
    stay_awake = false;
    power_off_on_close = false;

    // Client-side window options
    fullscreen = false;
    always_on_top = false;
    window_borderless = false;

    // Recording
    record_format = "auto";
    no_playback = false;
    no_video_playback = false;
}

QStringList ScrcpyOptions::toAdbShellArgs() const
{
    QStringList args;
    // Base command to execute the scrcpy server on the device.
    args << "shell"
         << "CLASSPATH=/data/local/tmp/scrcpy-server.jar"
         << "app_process"
         << "/"
         << "com.genymobile.scrcpy.Server"
         << version;

    // Append core parameters.
    args << QString("log_level=%1").arg(logLevel);

    // Append video parameters only if video is enabled.
    args << QString("video=%1").arg(video ? "true" : "false");
    if (video) {
        if (video_source != "display") args << QString("video_source=%1").arg(video_source);
        if (max_size > 0) args << QString("max_size=%1").arg(max_size);
        if (video_bit_rate != 8000000) args << QString("video_bit_rate=%1").arg(video_bit_rate);
        if (max_fps > 0) args << QString("max_fps=%1").arg(max_fps);
        if (video_codec != "h264") args << QString("video_codec=%1").arg(video_codec);
        if (display_id != 0) args << QString("display_id=%1").arg(display_id);
        if (no_video_playback) args << "video_playback=false";
        if (!crop.isEmpty()) args << QString("crop=%1").arg(crop);
        // Append camera-specific parameters if the camera is the source.
        if (video_source == "camera") {
            if (!camera_id.isEmpty()) args << QString("camera_id=%1").arg(camera_id);
            if (camera_facing != "any") args << QString("camera_facing=%1").arg(camera_facing);
            if (!camera_size.isEmpty() && camera_size.toLower() != "auto") args << QString("camera_size=%1").arg(camera_size);
            if (camera_fps > 0) args << QString("camera_fps=%1").arg(camera_fps);
            if (camera_high_speed) args << "camera_high_speed=true";
        }
    }

    // Append audio parameters only if audio is enabled.
    args << QString("audio=%1").arg(audio ? "true" : "false");
    if (audio) {
        if (audio_source != "output") args << QString("audio_source=%1").arg(audio_source);
        if (audio_bit_rate != 128000) args << QString("audio_bit_rate=%1").arg(audio_bit_rate);
        if (audio_codec != "opus") args << QString("audio_codec=%1").arg(audio_codec);
        if (audio_buffer != 50) args << QString("audio_buffer=%1").arg(audio_buffer);
        if (audio_dup) args << "audio_dup=true";
        if (require_audio) args << "require_audio=true";
    }

    // Append control and behavior parameters.
    args << QString("control=%1").arg(control ? "true" : "false");
    if (stay_awake) args << "stay_awake=true";
    if (power_off_on_close) args << "power_off_on_close=true";
    if (show_touches) args << "show_touches=true";
    if (!clipboard_autosync) args << "clipboard_autosync=false";
    if (keyboard_mode != "sdk") args << QString("keyboard=%1").arg(keyboard_mode);
    if (mouse_mode != "sdk") args << QString("mouse=%1").arg(mouse_mode);
    if (otg) args << "otg=true";

    // Append recording parameters.
    if (!record_file.isEmpty()) {
        // The PC path is for the client, but we must tell the server to record to a temporary file on the device.
        QString format = (record_format == "auto" ? "mkv" : record_format);
        QString device_path = QString("/sdcard/%1.%2").arg(DEVICE_RECORD_FILENAME).arg(format);

        args << QString("record_file=%1").arg(device_path);

        if (record_format != "auto") {
            args << QString("record_format=%1").arg(record_format);
        }
    }
    if (no_playback) args << "no_playback=true";

    // Append fixed parameters required for this client's operation.
    if (tunnel_forward) args << "tunnel_forward=true";
    args << "send_dummy_byte=true"; // Helps in detecting the connection start.

    return args;
}


