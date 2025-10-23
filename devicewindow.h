#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QPointer>
#include "adbprocess.h"
#include "scrcpyoptions.h"
#include "controlsender.h"
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class DeviceWindow; }
QT_END_NAMESPACE

class VideoDecoderThread;

/**
 * @class DeviceWindow
 * @brief Manages the display and interaction for a single Android device.
 *
 * This class orchestrates the entire process for a device connection:
 * 1. Pushing the scrcpy server to the device.
 * 2. Setting up a reverse TCP port forward.
 * 3. Starting the server on the device.
 * 4. Establishing video and control socket connections.
 * 5. Decoding the video stream and displaying it with optimized rendering.
 * 6. Forwarding user input (mouse, keyboard) to the device.
 * 7. Handling cleanup and teardown of all resources.
 *
 * OPTIMIZATIONS:
 * - Uses QLabel's built-in scaling instead of manual per-frame scaling
 * - Caches coordinate transformations for mouse events
 * - Defers window resizing to prevent blocking
 * - Thread-safe UI updates with Qt::QueuedConnection
 */
class DeviceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeviceWindow(const QString &serial, const ScrcpyOptions &options, QWidget *parent = nullptr);
    ~DeviceWindow();

    QString getSerial() const;

signals:
    void windowClosed(const QString &serial);
    void statusUpdated(const QString &serial, const QString &deviceName, const QSize &frameSize);

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;


private slots:
    // Connection workflow
    void startStreaming();
    void pushServer();
    void onPushServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void forwardPort();
    void onForwardPortFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void startServer();

    // Socket and decoding
    void connectToSocketWithRetry();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSocketReadyRead();
    void onFrameDecoded(const QImage &frame);
    void onDecodingFinished(const QString &message);

    // Toolbar actions
    void on_action_power_triggered();
    void on_action_volumeUp_triggered();
    void on_action_volumeDown_triggered();
    void on_action_home_triggered();
    void on_action_back_triggered();
    void on_action_appSwitch_triggered();
    void on_action_menu_triggered();
    void on_action_expandNotifications_triggered();
    void on_action_collapseNotifications_triggered();
    void on_action_screenshot_triggered();

private:
    // Configuration constants
    struct DisplayConfig {
        static constexpr int BASE_HEIGHT_PORTRAIT = 800;
        static constexpr int BASE_WIDTH_LANDSCAPE = 960;
        static constexpr int MAX_CONNECTION_RETRIES = 20;
        static constexpr int RETRY_DELAY_MS = 200;
        static constexpr int DECODER_STOP_TIMEOUT_MS = 2000;
        static constexpr int SERVER_PROCESS_TIMEOUT_MS = 1000;
    };

    /**
     * @struct CoordinateTransform
     * @brief Caches mouse coordinate transformation calculations
     *
     * Instead of recalculating margins and scaling factors on every mouse event,
     * we cache them and only update when the frame size changes.
     */
    struct CoordinateTransform {
        double scaleX = 1.0;
        double scaleY = 1.0;
        int marginX = 0;
        int marginY = 0;
        QSize videoSize;
        bool isValid = false;
    };

    void stopAll();
    void setupToolbarActions();
    void showError(const QString &title, const QString &message, bool fatal = false);

    // Optimized coordinate mapping
    QPoint mapMousePosition(const QPoint &pos);
    void updateCoordinateTransform();

    // Key mapping helpers
    int qtKeyToAndroidKey(int qtKey);
    int qtModifiersToAndroidMetaState(Qt::KeyboardModifiers modifiers);

    void optimizeSocketForLowLatency(QTcpSocket* socket);

    // UI and core members
    Ui::DeviceWindow *ui;
    QString mSerial;
    quint16 mLocalPort;
    QPointer<AdbProcess> mServerProcess;  // Changed to QPointer for safety
    QPointer<QTcpSocket> mVideoSocket;    // Changed to QPointer for safety
    QPointer<VideoDecoderThread> mDecoder; // Changed to QPointer for safety
    QString mDeviceName;

    // Control and state
    QPointer<ControlSender> mControlSender; // Changed to QPointer for safety
    ScrcpyOptions mOptions;
    int mConnectionRetries;
    QSize mCurrentFrameSize;
    bool mIsMousePressed = false;

    // Performance optimizations
    CoordinateTransform mTransform;
    bool mFirstFrame = true; // Track first frame to set scaling mode once


    // FPS monitoring (debug builds only)
#ifdef QT_DEBUG
    qint64 mFrameCount = 0;
    qint64 mLastFpsTime = 0;
#endif
};

#endif // DEVICEWINDOW_H
