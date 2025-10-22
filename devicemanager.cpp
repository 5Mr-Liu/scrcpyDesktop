#include "devicemanager.h"
#include <QDebug>
#include <QRegularExpression>

/**
 * @file devicemanager.cpp
 * @brief Implementation of the DeviceManager class.
 */

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    mAdbProcess = new QProcess(this);

    // Connect the QProcess::finished signal to our handler slot.
    // This is the core of the asynchronous design: when the adb command completes,
    // our onAdbProcessFinished method will be automatically invoked.
    connect(mAdbProcess, &QProcess::finished, this, &DeviceManager::onAdbProcessFinished);
}

DeviceManager::~DeviceManager()
{
    // Ensure the adb process is cleaned up when the DeviceManager object is destroyed.
    // This prevents orphaned adb processes if the application closes while a scan is running.
    if (mAdbProcess->state() == QProcess::Running) {
        mAdbProcess->kill();
        mAdbProcess->waitForFinished();
    }
}

void DeviceManager::refreshDevices()
{
    // Prevent starting a new scan if one is already in progress.
    if (mAdbProcess->state() == QProcess::Running) {
        emit logMessage("Device scan is already in progress, please wait...");
        return;
    }

    emit logMessage("Starting USB device scan (adb devices)...");

    // Start the 'adb devices' command.
    // Note: This assumes 'adb' is in the system's PATH. If not, a full path must be provided.
    // For example: "C:/path/to/platform-tools/adb.exe"
    // The previous hardcoded path "scrcpy-win64-v3.3.3/adb.exe" is not portable.
    mAdbProcess->start("adb", QStringList() << "devices");
}

void DeviceManager::onAdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // First, check if the process crashed.
    if (exitStatus == QProcess::CrashExit) {
        emit logMessage("Error: The adb process crashed.");
        return;
    }

    // Check if the command executed successfully. A non-zero exit code indicates an error.
    if (exitCode != 0) {
        emit logMessage("Error: 'adb devices' command failed to execute. Please ensure adb is correctly configured in your system's PATH.");
        // Read and display the standard error output from adb for more detailed diagnostics.
        QString errorOutput = mAdbProcess->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            emit logMessage("ADB Error: " + errorOutput);
        }
        return;
    }

    // Read the standard output from the successfully executed command.
    QString output = mAdbProcess->readAllStandardOutput();

    QList<DeviceInfo> devices;

    // Split the output into individual lines.
    // Using QRegularExpression("[\r\n]") handles both Windows (CRLF) and Unix (LF) line endings.
    QStringList lines = output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);

    // Iterate over each line to parse device information.
    for (const QString &line : lines) {
        // The first line of the output is "List of devices attached", which should be ignored.
        if (line.startsWith("List of devices")) {
            continue;
        }

        // Split the line by one or more whitespace characters to separate the serial and status.
        // Example line: "emulator-5554   device"
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        // A valid device line consists of exactly two parts: [serial, status].
        if (parts.size() == 2) {
            DeviceInfo device;
            device.serial = parts[0];
            device.status = parts[1];
            devices.append(device);
        }
    }

    // Report the results and emit the signal with the updated device list.
    emit logMessage(QString("Scan complete. Found %1 device(s).").arg(devices.size()));
    emit devicesUpdated(devices);
}
