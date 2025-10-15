#include "adbprocess.h"
#include <QDebug>

AdbProcess::AdbProcess(QObject *parent) : QProcess(parent)
{
}

AdbProcess::~AdbProcess()
{
    // 确保进程在对象析构时被终止
    if (state() == QProcess::Running) {
        terminate();
        waitForFinished(1000);
    }
}

void AdbProcess::execute(const QString &serial, const QStringList &args)
{
    mSerial = serial;
    QStringList allArgs;
    if (!serial.isEmpty()) {
        allArgs << "-s" << serial;
    }
    allArgs.append(args);

    qDebug() << "Executing adb command:" << "adb" << allArgs;

    // 设置 adb.exe 的路径
    // 注意：这里假设 adb 在系统 PATH 中，如果不在，需要提供完整路径
    // 例如：start("C:/path/to/adb.exe", allArgs);
    start("adb", allArgs);
}

bool AdbProcess::executeBlocking(const QString &serial, const QStringList &args, int timeoutMs)
{
    execute(serial, args);
    return waitForFinished(timeoutMs);
}

QString AdbProcess::getOutput()
{
    return QString::fromLocal8Bit(readAllStandardOutput() + readAllStandardError());
}
