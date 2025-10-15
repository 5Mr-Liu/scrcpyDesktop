#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QList>

// 用一个结构体来清晰地表示一个设备的信息
struct DeviceInfo {
    QString serial;
    QString status; // "device", "offline", "unauthorized" 等
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

public slots:
    // 这个槽函数将由 MainWindow 的按钮触发
    void refreshDevices();

signals:
    // 当设备列表更新后，发射这个信号，将新的设备列表传递出去
    void devicesUpdated(const QList<DeviceInfo> &devices);
    void logMessage(const QString &message); // 用于向主窗口发送日志

private slots:
    // 当 adb 进程执行完毕时，这个内部槽会被调用
    void onAdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *mAdbProcess;
};

#endif // DEVICEMANAGER_H
