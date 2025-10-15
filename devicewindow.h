#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "adbprocess.h"
#include "scrcpyoptions.h" // <--- 包含你的 ScrcpyOptions 头文件


QT_BEGIN_NAMESPACE
namespace Ui { class DeviceWindow; }
QT_END_NAMESPACE

class VideoDecoderThread;

class DeviceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeviceWindow(const QString &serial, QWidget *parent = nullptr);
    ~DeviceWindow();
    QString getSerial() const;

signals:
    void windowClosed(const QString &serial);

protected:
    void closeEvent(QCloseEvent *event) override;

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

private:
    void stopAll();

    Ui::DeviceWindow *ui;
    QString mSerial;
    quint16 mLocalPort;

    AdbProcess *mServerProcess;
    QTcpSocket *mVideoSocket;
    VideoDecoderThread *mDecoder;

    // --- 使用你的 ScrcpyOptions ---
    ScrcpyOptions mOptions; // <--- 使用你的选项类

    // 【保留】用于连接重试的计数器
    int mConnectionRetries = 0;

    QSize mCurrentFrameSize;
};

#endif // DEVICEWINDOW_H
