#ifndef ADBPROCESS_H
#define ADBPROCESS_H

#include <QProcess>

/**
 * @class AdbProcess
 * @brief A QProcess subclass specialized for executing Android Debug Bridge (adb) commands.
 *
 * This class simplifies the process of running adb commands by handling
 * the boilerplate of adding the device serial number and managing the process lifecycle.
 */
class AdbProcess : public QProcess
{
    Q_OBJECT
public:
    /**
     * @brief Constructs an AdbProcess object.
     * @param parent The parent QObject, optional.
     */
    explicit AdbProcess(QObject *parent = nullptr);

    /**
     * @brief Destructor.
     *
     * Ensures any running adb process is terminated when the object is destroyed.
     */
    ~AdbProcess();

    /**
     * @brief Asynchronously executes an adb command for a specific device.
     *
     * This method starts the adb command and returns immediately.
     * You can connect to signals like finished() or readyReadStandardOutput()
     * to handle the process completion and output.
     *
     * @param serial The serial number of the target device. Can be empty if only one device is connected.
     * @param args A list of arguments to pass to the adb command (e.g., {"shell", "ls"}).
     */
    void execute(const QString &serial, const QStringList &args);

    /**
     * @brief Synchronously executes an adb command and waits for it to finish.
     *
     * This method blocks the calling thread until the adb command has finished
     * or the specified timeout is reached.
     *
     * @param serial The serial number of the target device.
     * @param args A list of arguments for the adb command.
     * @param timeoutMs The maximum time to wait for the command to finish, in milliseconds. Default is 30000 (30 seconds).
     * @return True if the process finished successfully within the timeout; otherwise, false.
     */
    bool executeBlocking(const QString &serial, const QStringList &args, int timeoutMs = 30000);

    /**
     * @brief Retrieves the combined standard output and standard error from the last executed command.
     *
     * This should be called after the process has finished.
     *
     * @return A QString containing all output from the process.
     */
    QString getOutput();

private:
    // Stores the serial number of the device for the current command.
    QString mSerial;
};

#endif // ADBPROCESS_H
