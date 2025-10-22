#ifndef UISTATEMANAGER_H
#define UISTATEMANAGER_H

#include <QObject>
#include <QMap>
#include <QSize> // Include QSize for the frame size parameter

// Forward declarations to avoid circular header dependencies.
namespace Ui {
class MainWindow;
}
class DeviceWindow;

/**
 * @file uistatemanager.h
 * @brief Defines the UiStateManager class, responsible for managing the state of UI controls.
 */

/**
 * @class UiStateManager
 * @brief Manages the dynamic states of the main window's UI elements.
 *
 * This class centralizes all the logic for enabling, disabling, and updating
 * UI controls based on user selections and application state. For example, it
 * handles disabling video settings when the "No Video" checkbox is ticked,
 * or updating the status bar with the number of connected devices.
 * This separates UI logic from the main window's core application logic,
 * leading to cleaner and more maintainable code.
 */
class UiStateManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor for UiStateManager.
     * @param ui A pointer to the MainWindow's UI object.
     * @param parent The parent QObject.
     */
    explicit UiStateManager(Ui::MainWindow *ui, QObject *parent = nullptr);

    /**
     * @brief Performs initial setup of the UI states and table headers.
     */
    void initializeStates();

    /**
     * @brief Sets the pointer to the map of active device windows.
     * @param deviceWindows A const pointer to the QMap instance in MainWindow.
     */
    void setDeviceWindowsMap(const QMap<QString, DeviceWindow*> *deviceWindows);

public slots:
    // --- Public Slots for UI Control Signals ---

    /**
     * @brief A master slot that calls all individual state update methods.
     *        Connected to various UI controls that trigger state changes.
     */
    void updateAllControlStates();

    // --- Public Slots for Device Status Updates ---

    /**
     * @brief Updates the general status indicators, like the status bar
     *        and the visibility of the device status tab.
     */
    void updateConnectedDeviceStatus();

    /**
     * @brief Adds a new row to the device status table for a new connection.
     * @param serial The serial number of the device being connected.
     */
    void addDeviceToStatusTable(const QString &serial);

    /**
     * @brief Removes the corresponding row from the device status table when a device disconnects.
     * @param serial The serial number of the disconnected device.
     */
    void removeDeviceFromStatusTable(const QString &serial);

    /**
     * @brief Updates the information for a specific device in the status table.
     * @param serial The serial number of the device to update.
     * @param deviceName The discovered name of the device.
     * @param frameSize The current resolution of the device's video stream.
     */
    void updateDeviceStatusInfo(const QString &serial, const QString &deviceName, const QSize &frameSize);

private:
    // --- Private Helper Methods for Specific UI Areas ---
    void updateVideoControlsState();
    void updateAudioControlsState();
    void updateControlControlsState();
    void updateRecordingControlsState();

    Ui::MainWindow *m_ui; // A pointer to the main window's UI elements.
    // A const pointer to the map of device windows in MainWindow, used for data retrieval.
    const QMap<QString, DeviceWindow*> *m_deviceWindows = nullptr;
};

#endif // UISTATEMANAGER_H
