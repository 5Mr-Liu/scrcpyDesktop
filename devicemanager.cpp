#include "devicemanager.h"
#include <QDebug>
#include <QRegularExpression>

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    mAdbProcess = new QProcess(this);

    // 连接 QProcess 的 finished 信号，以便在命令执行完后得到通知
    connect(mAdbProcess, &QProcess::finished, this, &DeviceManager::onAdbProcessFinished);
}

DeviceManager::~DeviceManager()
{
    // 确保进程在对象销毁时被清理
    if (mAdbProcess->state() == QProcess::Running) {
        mAdbProcess->kill();
        mAdbProcess->waitForFinished();
    }
}

void DeviceManager::refreshDevices()
{
    // 如果上次的扫描还在进行中，就不要开始新的扫描
    if (mAdbProcess->state() == QProcess::Running) {
        emit logMessage("正在扫描设备，请稍候...");
        return;
    }

    emit logMessage("开始扫描 USB 设备 (adb devices)...");
    // 启动 adb devices 命令
    QString adbPath = "scrcpy-win64-v3.3.3/adb.exe";
    mAdbProcess->start(adbPath, QStringList() << "devices");
}

void DeviceManager::onAdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        emit logMessage("错误：adb 进程崩溃。");
        return;
    }
    if (exitCode != 0) {
        emit logMessage("错误：adb 命令执行失败。请确保 adb 环境配置正确。");
        QString errorOutput = mAdbProcess->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            emit logMessage("ADB Error: " + errorOutput);
        }
        return;
    }
    QString output = mAdbProcess->readAllStandardOutput();

    QList<DeviceInfo> devices;
    // 按行分割输出
    // VVVV--- 关键修改 2 ---VVVV
    QStringList lines = output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith("List of devices")) {
            continue;
        }
        // VVVV--- 关键修改 3 ---VVVV
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() == 2) {
            DeviceInfo device;
            device.serial = parts[0];
            device.status = parts[1];
            devices.append(device);
        }
    }
    emit logMessage(QString("扫描完成，发现 %1 个设备。").arg(devices.size()));
    emit devicesUpdated(devices);
}
