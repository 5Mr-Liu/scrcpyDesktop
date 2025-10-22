#ifndef SCRCPYOPTIONS_H
#define SCRCPYOPTIONS_H

#include <QString>
#include <QStringList>

/**
 * @file scrcpyoptions.h
 * @brief Defines the ScrcpyOptions class, which encapsulates all configuration parameters for a scrcpy session.
 */

/**
 * @class ScrcpyOptions
 * @brief Holds all possible command-line options for the scrcpy server.
 *
 * This class serves as a data structure to store the configuration for a scrcpy session.
 * It provides a convenient way to pass settings from the UI to the process that starts
 * the scrcpy server on the Android device. It also includes a method to convert these
 * options into the format required by the 'adb shell' command.
 */
class ScrcpyOptions
{
public:
    ScrcpyOptions();

    /**
     * @brief Converts all stored options into a list of arguments for the 'adb shell' command.
     * @return A QStringList containing the formatted arguments to start the scrcpy server.
     */
    QStringList toAdbShellArgs() const;

    // --- Core & General Parameters ---
    QString version;          // The version of the scrcpy server to be executed.
    QString logLevel;         // Log level for the server (e.g., "info", "debug").
    bool tunnel_forward;      // Whether to use a forward tunnel for the connection.

    // --- Video Parameters ---
    bool video;               // Enable/disable video streaming.
    QString video_source;     // Source of the video ("display" or "camera").
    quint32 video_bit_rate;   // Video bitrate in bits per second.
    QString video_codec;      // Video codec to use ("h264", "h265", "av1").
    quint16 max_size;         // Maximum video dimension (width or height). 0 for unlimited.
    quint16 max_fps;          // Maximum frames per second. 0 for unlimited.
    quint8 display_id;        // The ID of the display to mirror.
    QString crop;             // Crop the video to a specific dimension ("width:height:x:y").

    // --- Audio Parameters ---
    bool audio;               // Enable/disable audio forwarding.
    quint32 audio_bit_rate;   // Audio bitrate in bits per second.
    QString audio_codec;      // Audio codec to use ("opus", "aac", "flac", "raw").
    QString audio_source;     // Source of the audio ("output", "mic").
    quint16 audio_buffer;     // Audio buffer size in milliseconds.
    bool audio_dup;           // Duplicate audio output to the device's speakers.
    bool require_audio;       // Abort if audio capture fails.

    // --- Camera Parameters (Effective only when video_source == "camera") ---
    QString camera_id;        // The specific ID of the camera to use.
    QString camera_facing;    // Camera facing direction ("front", "back", "external").
    QString camera_size;      // Camera resolution, e.g., "1920:1080".
    quint8 camera_fps;        // Camera frames per second.
    bool camera_high_speed;   // Enable high-speed camera mode if available.

    // --- Control & Interaction Parameters ---
    bool control;             // Enable/disable device control (input events).
    bool show_touches;        // Show physical touches on the device's screen.
    bool clipboard_autosync;  // Enable automatic clipboard synchronization.
    QString keyboard_mode;    // Keyboard injection mode ("sdk", "uhid", "aoa", "disabled").
    QString mouse_mode;       // Mouse injection mode ("sdk", "uhid", "aoa", "disabled").
    bool otg;                 // Run in OTG mode (keyboard/mouse as physical devices).

    // --- Device & Power Management Parameters ---
    bool stay_awake;          // Prevent the device from sleeping.
    bool power_off_on_close;  // Turn the device screen off when the client closes.

    // --- Client-Side Window & Recording Parameters ---
    // These options are used by the client (DeviceWindow) and are NOT passed to the scrcpy server,
    // but are kept here for centralized management.
    bool fullscreen;
    bool always_on_top;
    bool window_borderless;
    QString window_title;

    // These recording parameters ARE passed to the scrcpy server.
    QString record_file;      // PC path to save the recording. The server will use a temp path.
    QString record_format;    // Recording container format ("mp4", "mkv").
    bool no_playback;         // Record but do not display the stream on the client.
    bool no_video_playback;   // For audio-only mirroring, disable the black video window.
};

#endif // SCRCPYOPTIONS_H
