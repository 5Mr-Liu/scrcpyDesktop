#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QList>

/**
 * @file devicemanager.h
 * @brief Defines the DeviceManager class for detecting and managing Android devices via ADB.
 */

/**
 * @struct DeviceInfo
 * @brief A simple structure to hold information about a single detected Android device.
 *
 * This struct cleanly encapsulates the data parsed from the 'adb devices' command output.
 */
struct DeviceInfo {
    QString serial; // The unique serial number of the device.
    QString status; // The connection status, e.g., "device", "offline", "unauthorized".
};

/**
 * @class DeviceManager
 * @brief Manages the discovery of Android devices connected to the system.
 *
 * This class uses QProcess to run the 'adb devices' command asynchronously.
 * It parses the output of the command and emits a signal containing a list
 * of all found devices. It also provides logging signals to communicate its
 * status to the user interface.
 */
class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

public slots:
    /**
     * @brief Initiates a scan for connected Android devices.
     *
     * This public slot can be triggered, for instance, by a button click in the UI.
     * It starts the 'adb devices' command asynchronously.
     */
    void refreshDevices();

signals:
    /**
     * @brief Emitted when the device scan is complete and the list of devices has been updated.
     * @param devices A list of DeviceInfo structs representing the found devices.
     */
    void devicesUpdated(const QList<DeviceInfo> &devices);

    /**
     * @brief Emitted to provide status updates or error messages to the UI for logging.
     * @param message The log message to be displayed.
     */
    void logMessage(const QString &message);

private slots:
    /**
     * @brief An internal slot that is called when the adb QProcess finishes.
     *
     * This slot handles the result of the 'adb devices' command, parsing its output
     * or reporting any errors that occurred.
     * @param exitCode The exit code of the process.
     * @param exitStatus The exit status (e.g., normal exit or crash).
     */
    void onAdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    // The QProcess instance used to execute adb commands.
    QProcess *mAdbProcess;
};

#endif // DEVICEMANAGER_H
