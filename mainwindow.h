#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "devicemanager.h"
#include "devicewindow.h"
#include <QMap>
#include "scrcpyoptions.h"
#include "uistatemanager.h"

namespace Ui {
class MainWindow;
}

/**
 * @class MainWindow
 * @brief The main application window and central controller.
 *
 * This class is responsible for the main user interface, managing device discovery,
 * collecting user-configured scrcpy options, and launching DeviceWindow instances
 * for each connected device. It also handles logging and UI state management.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- Core Logic Slots (Not directly tied to UI interaction) ---
    /**
     * @brief Slot to handle the updated list of devices from DeviceManager.
     * @param devices The new list of detected devices.
     */
    void onDevicesUpdated(const QList<DeviceInfo> &devices);

    /**
     * @brief Slot to receive and display log messages in the UI.
     * @param message The message to log.
     */
    void onLogMessage(const QString &message);

    /**
     * @brief Slot to handle cleanup when a DeviceWindow is closed.
     * @param serial The serial number of the device whose window was closed.
     */
    void onDeviceWindowClosed(const QString &serial);

    // --- Button Click Handler Slots (Manually connected in the constructor) ---
    void handleConnectUsbClick();
    void handleEnableTcpIpClick();
    void handleConnectWifiClick();
    void handleRemoveWifiClick();
    void handleConnectSerialClick();
    void handleDisconnectAllClick();
    void handleKillAdbClick();
    void handleSaveRecordAsClick();
    void handleClearLogsClick();

    // --- Menu Bar Action Slots ---

    // File Menu
    void handleSaveProfileAction();
    void handleLoadProfileAction();
    void handleSettingsAction();
    void handleExitAction();

    // Device Menu
    void handleConnectAllUsbAction();

    // View Menu
    void handleToggleLeftPanel(bool checked);
    void handleToggleBottomPanel(bool checked);
    void handleAppAlwaysOnTop(bool checked);
    void handleGridLayoutAction();

    // Help Menu
    void handleAboutAction();
    void handleShortcutsAction();
    void handleCheckUpdatesAction();

private:
    /**
     * @brief Centralizes all menu action signal/slot connections.
     */
    void setupMenuConnections();

    /**
     * @brief Saves the current UI settings to an INI file.
     * @param filePath The path of the file to save to.
     */
    void saveUiToSettings(const QString& filePath);

    /**
     * @brief Loads UI settings from an INI file.
     * @param filePath The path of the file to load from.
     */
    void loadUiFromSettings(const QString& filePath);

    /**

     * @brief Gathers all scrcpy options from the UI controls.
     * @return A ScrcpyOptions struct populated with the current settings.
     */
    ScrcpyOptions gatherScrcpyOptions() const;

    /**
     * @brief Validates the collected ScrcpyOptions before starting a session.
     * @param options The options to validate.
     * @return True if options are valid, false otherwise.
     */
    bool validateOptions(const ScrcpyOptions &options);

    /**
     * @brief Creates and shows a new DeviceWindow for a specified device.
     * @param serial The serial number of the device to connect to.
     */
    void startDeviceWindow(const QString &serial);

    Ui::MainWindow *ui;
    DeviceManager *mDeviceManager;
    // A map to keep track of active device windows, using the serial number as the key.
    QMap<QString, DeviceWindow*> mDeviceWindows;
    UiStateManager *mUiStateManager;
};

#endif // MAINWINDOW_H
