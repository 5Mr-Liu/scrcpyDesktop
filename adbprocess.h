#ifndef ADBPROCESS_H
#define ADBPROCESS_H

#include <QProcess>

class AdbProcess : public QProcess
{
    Q_OBJECT
public:
    explicit AdbProcess(QObject *parent = nullptr);
    ~AdbProcess();

    // 启动一个 adb 命令
    void execute(const QString &serial, const QStringList &args);

    // 阻塞式执行，等待命令完成（带超时）
    bool executeBlocking(const QString &serial, const QStringList &args, int timeoutMs = 30000);

    // 获取进程的输出
    QString getOutput();

private:
    QString mSerial;
};

#endif // ADBPROCESS_H
