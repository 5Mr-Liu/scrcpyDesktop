#ifndef CONTROLSENDER_H
#define CONTROLSENDER_H

#include <QObject>
#include <QPoint>
#include <QSize>

class QTcpSocket;

/**
 * @file controlsender.h
 * @brief Defines the ControlSender class and related enums for sending control messages to a scrcpy server.
 *
 * This file contains the necessary enumerations for message types and event actions,
 * which are structured according to the scrcpy control protocol.
 */

/**
 * @enum ControlMsgType
 * @brief Defines the types of control messages that can be sent to the scrcpy server.
 *
 * Each value corresponds to a specific action to be performed on the Android device.
 */
enum ControlMsgType {
    CONTROL_MSG_TYPE_INJECT_KEYCODE,          // Injects a key event (press or release).
    CONTROL_MSG_TYPE_INJECT_TEXT,             // Injects a string of text.
    CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT,      // Injects a touch event (down, up, move).
    CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT,     // Injects a scroll event.
    CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON,       // Simulates a BACK key press or turns the screen on.
    CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL, // Expands the notification panel.
    CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL,// Collapses the notification panel.
    CONTROL_MSG_TYPE_GET_CLIPBOARD,           // Requests the device's clipboard content.
    CONTROL_MSG_TYPE_SET_CLIPBOARD,           // Sets the device's clipboard content.
    CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE,   // Sets the screen power mode (e.g., off, on).
    CONTROL_MSG_TYPE_ROTATE_DEVICE,           // Rotates the device screen.
};

/**
 * @enum AndroidKeyEventAction
 * @brief Defines the actions for a key event.
 */
enum AndroidKeyEventAction {
    AKEY_EVENT_ACTION_DOWN = 0, // The key is being pressed down.
    AKEY_EVENT_ACTION_UP = 1,   // The key is being released.
};

/**
 * @enum AndroidMotionEventAction
 * @brief Defines the actions for a touch (motion) event.
 */
enum AndroidMotionEventAction {
    AMOTION_EVENT_ACTION_DOWN = 0, // A finger is touching the screen.
    AMOTION_EVENT_ACTION_UP = 1,   // A finger is being lifted from the screen.
    AMOTION_EVENT_ACTION_MOVE = 2, // A finger is moving across the screen.
};

/**
 * @enum ScreenPowerMode
 * @brief Defines the desired power mode for the device's screen.
 */
enum ScreenPowerMode {
    SCREEN_POWER_MODE_OFF = 0,    // Turn the screen off.
    SCREEN_POWER_MODE_DOZE = 1,   // Put the screen in a low-power (doze/ambient) mode.
    SCREEN_POWER_MODE_NORMAL = 2, // Turn the screen on (normal brightness).
};


/**
 * @class ControlSender
 * @brief Manages the network connection and serialization of control messages for a scrcpy server.
 *
 * This class handles connecting to the scrcpy server's control socket and provides a high-level
 * API for sending various commands like touch events, key presses, and text input.
 * It serializes these commands into the specific byte format required by the scrcpy protocol.
 */
class ControlSender : public QObject
{
    Q_OBJECT
public:
    explicit ControlSender(QObject *parent = nullptr);
    ~ControlSender();

    /**
     * @brief Initiates a connection to the scrcpy server's control socket.
     * @param host The hostname or IP address of the server.
     * @param port The port number of the control socket.
     */
    void connectToServer(const QString &host, quint16 port);

    /**
     * @brief Disconnects from the server.
     */
    void disconnectFromServer();

    // -- Public API for sending various control commands --

    /**
     * @brief Sends a touch event to the device.
     * @param action The type of touch action (DOWN, UP, or MOVE).
     * @param pos The coordinates of the touch event.
     * @param screenSize The current screen dimensions, required by the protocol.
     */
    void postInjectTouch(AndroidMotionEventAction action, QPoint pos, QSize screenSize);

    /**
     * @brief Sends a keycode event to the device.
     * @param action The type of key action (DOWN or UP).
     * @param keyCode The Android keycode to send (from AndroidKeycodes.h).
     * @param metaState The state of meta keys like Shift, Ctrl, Alt (from AndroidKeycodes.h).
     */
    void postInjectKeycode(AndroidKeyEventAction action, int keyCode, int metaState = 0);

    /**
     * @brief Sends a string of text to be typed on the device.
     * @param text The text to inject.
     */
    void postInjectText(const QString &text);

    /**
     * @brief Sends a BACK key press or a command to turn the screen on.
     * @param action The key action (typically DOWN followed by UP).
     */
    void postBackOrScreenOn(AndroidKeyEventAction action);

    /**
     * @brief Sets the power mode of the device's screen.
     * @param mode The desired power mode.
     */
    void postSetScreenPowerMode(ScreenPowerMode mode);

    /**
     * @brief Sends a command to rotate the device's screen.
     */
    void postRotateDevice();

    /**
     * @brief Sends a command to expand the notification panel.
     */
    void postExpandNotificationPanel();

    /**
     * @brief Sends a command to collapse the notification panel.
     */
    void postCollapseNotificationPanel();
    // ... Other commands can be added here as needed, following the scrcpy protocol.

signals:
    /**
     * @brief Emitted when the control socket successfully connects to the server.
     */
    void controlSocketConnected();

    /**
     * @brief Emitted when the control socket disconnects from the server.
     */
    void controlSocketDisconnected();

private:
    /**
     * @brief Private helper function to write raw data to the socket.
     * @param data The QByteArray containing the serialized message.
     * @return The number of bytes written, or -1 on error.
     */
    qint64 send(const QByteArray &data);

    // The TCP socket used for the control connection.
    QTcpSocket *mControlSocket;
};

#endif // CONTROLSENDER_H
