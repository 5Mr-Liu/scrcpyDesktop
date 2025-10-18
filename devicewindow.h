#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "adbprocess.h"
#include "scrcpyoptions.h" // <--- 包含你的 ScrcpyOptions 头文件

#include "controlsender.h" // 包含 ControlSender 头文件
#include <QKeyEvent> // 包含 QKeyEvent

QT_BEGIN_NAMESPACE
namespace Ui { class DeviceWindow; }
QT_END_NAMESPACE

class VideoDecoderThread;

class DeviceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeviceWindow(const QString &serial,const ScrcpyOptions &options, QWidget *parent = nullptr);
    ~DeviceWindow();
    QString getSerial() const;

signals:
    void windowClosed(const QString &serial);
    void statusUpdated(const QString &serial, const QString &deviceName, const QSize &frameSize);

protected:
    void closeEvent(QCloseEvent *event) override;
    // --- 新增：重写事件处理器 ---
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void startStreaming();
    void pushServer();
    void onPushServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void forwardPort();
    void onForwardPortFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void startServer();

    // --- 连接与解码相关的槽函数 ---
    void connectToSocketWithRetry(); // 【保留】带重试的连接槽函数
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSocketReadyRead();
    void onFrameDecoded(const QImage &frame);
    void onDecodingFinished(const QString &message);

    // --- 新增：工具栏按钮的槽函数 ---
    void on_action_power_triggered();
    void on_action_volumeUp_triggered();
    void on_action_volumeDown_triggered();
    void on_action_rotateDevice_triggered();
    void on_action_home_triggered();
    void on_action_back_triggered();
    void on_action_appSwitch_triggered();
    void on_action_menu_triggered();
    void on_action_expandNotifications_triggered();
    void on_action_collapseNotifications_triggered();
    void on_action_screenshot_triggered();

private:
    void stopAll();
    // --- 新增：辅助函数 ---
    void setupToolbarActions();
    QPoint mapMousePosition(const QPoint &pos);
    int qtKeyToAndroidKey(int qtKey);
    int qtModifiersToAndroidMetaState(Qt::KeyboardModifiers modifiers);
    Ui::DeviceWindow *ui;
    QString mSerial;
    quint16 mLocalPort;
    AdbProcess *mServerProcess;
    QTcpSocket *mVideoSocket;
    VideoDecoderThread *mDecoder;
    QString mDeviceName;

    // --- 新增 ---
    ControlSender *mControlSender; // 控制发送器实例
    ScrcpyOptions mOptions;
    int mConnectionRetries;
    QSize mCurrentFrameSize;
    // --- 新增：用于跟踪鼠标状态 ---
    bool mIsMousePressed = false;
};

#endif // DEVICEWINDOW_H
