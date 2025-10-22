#include "adbprocess.h"
#include <QDebug>

AdbProcess::AdbProcess(QObject *parent) : QProcess(parent)
{
    // The constructor initializes the base QProcess class.
}

AdbProcess::~AdbProcess()
{
    // Ensure the process is terminated when the object is destructed.
    // This is a crucial cleanup step to prevent orphaned adb processes.
    if (state() == QProcess::Running) {
        terminate(); // Request termination
        waitForFinished(1000); // Wait up to 1 second for it to finish gracefully
    }
}

void AdbProcess::execute(const QString &serial, const QStringList &args)
{
    mSerial = serial; // Store the serial for reference if needed.

    // Prepare the full list of arguments for the adb executable.
    QStringList allArgs;

    // If a specific device serial is provided, prepend the "-s <serial>" argument,
    // which is the standard way to target a device with adb.
    if (!serial.isEmpty()) {
        allArgs << "-s" << serial;
    }
    // Append the user-provided command arguments.
    allArgs.append(args);

    // Log the command being executed for debugging purposes.
    qDebug() << "Executing adb command:" << "adb" << allArgs;

    // Start the adb process with the constructed arguments.
    // Note: This assumes 'adb' is available in the system's PATH environment variable.
    // If it's not, you must provide the full, absolute path to the executable.
    // For example: start("C:/android/sdk/platform-tools/adb.exe", allArgs);
    start("adb", allArgs);
}

bool AdbProcess::executeBlocking(const QString &serial, const QStringList &args, int timeoutMs)
{
    // First, start the command in a non-blocking way.
    execute(serial, args);
    // Then, block the current thread and wait for the process to complete,
    // respecting the specified timeout.
    return waitForFinished(timeoutMs);
}

QString AdbProcess::getOutput()
{
    // Read all data from both standard output and standard error streams.
    // Combining them is often useful as command-line tools can write
    // important information (like errors) to the standard error stream.
    // Convert the resulting QByteArray from the local 8-bit encoding to a QString.
    return QString::fromLocal8Bit(readAllStandardOutput() + readAllStandardError());
}
