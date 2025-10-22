#include "controlsender.h"
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>
#include <QtEndian>

/**
 * @file controlsender.cpp
 * @brief Implementation of the ControlSender class.
 *
 * This file contains the logic for serializing control messages into byte arrays
 * according to the scrcpy protocol and sending them over a TCP socket.
 */

/**
 * @brief A template helper function to write a value to a QByteArray in big-endian byte order.
 *
 * The scrcpy protocol requires multi-byte numerical values to be sent in big-endian format.
 * This function handles the conversion and appending of the value to the byte array.
 *
 * @tparam T The data type of the value (e.g., quint32, qint64).
 * @param arr The QByteArray to append the data to.
 * @param value The value to be written.
 */
template<typename T>
void write_be(QByteArray &arr, T value) {
    T big_endian_value = qToBigEndian(value);
    arr.append(reinterpret_cast<const char*>(&big_endian_value), sizeof(T));
}

ControlSender::ControlSender(QObject *parent) : QObject(parent)
{
    // Initialize the TCP socket for control messages.
    mControlSocket = new QTcpSocket(this);

    mControlSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    mControlSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 8192);

    // Connect socket signals to the corresponding slots/signals of this class.
    connect(mControlSocket, &QTcpSocket::connected, this, &ControlSender::controlSocketConnected);
    connect(mControlSocket, &QTcpSocket::disconnected, this, &ControlSender::controlSocketDisconnected);
}

ControlSender::~ControlSender()
{
    // The QTcpSocket is a child of this object, so it will be automatically
    // deleted by Qt's parent-child memory management system.
}

void ControlSender::connectToServer(const QString &host, quint16 port)
{
    // Only attempt to connect if we are not already connected or connecting.
    if (mControlSocket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << "[Control] Connecting to" << host << ":" << port;
        mControlSocket->connectToHost(host, port);
    }
}

void ControlSender::disconnectFromServer()
{
    // Disconnect if the socket is in any state other than unconnected.
    if (mControlSocket->state() != QAbstractSocket::UnconnectedState) {
        mControlSocket->disconnectFromHost();
    }
}

qint64 ControlSender::send(const QByteArray &data)
{
    // Ensure the socket is connected before attempting to write data.
    if (mControlSocket->state() == QAbstractSocket::ConnectedState) {
        return mControlSocket->write(data);
    }
    // Return -1 to indicate that nothing was sent.
    return -1;
}

void ControlSender::postInjectTouch(AndroidMotionEventAction action, QPoint pos, QSize screenSize)
{
    // Message structure for INJECT_TOUCH_EVENT:
    // [type: 1 byte]
    // [action: 1 byte]
    // [pointerId: 8 bytes]
    // [position.x: 4 bytes]
    // [position.y: 4 bytes]
    // [screenSize.width: 2 bytes]
    // [screenSize.height: 2 bytes]
    // [pressure: 2 bytes]
    // [action_button: 4 bytes]
    // [buttons: 4 bytes]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT);
    buffer.append(static_cast<char>(action));
    write_be<qint64>(buffer, -1); // pointerId, -1 represents a virtual finger.
    write_be<quint32>(buffer, pos.x());
    write_be<quint32>(buffer, pos.y());
    write_be<quint16>(buffer, screenSize.width());
    write_be<quint16>(buffer, screenSize.height());
    write_be<quint16>(buffer, 0xFFFF); // pressure, 0xFFFF represents max pressure.
    write_be<quint32>(buffer, 1);      // action button (AMOTION_EVENT_BUTTON_PRIMARY).
    write_be<quint32>(buffer, 1);      // buttons state (AMOTION_EVENT_BUTTON_PRIMARY).

    send(buffer);
}

void ControlSender::postInjectKeycode(AndroidKeyEventAction action, int keyCode, int metaState)
{
    // Message structure for INJECT_KEYCODE:
    // [type: 1 byte]
    // [action: 1 byte]
    // [keycode: 4 bytes]
    // [repeat: 4 bytes]
    // [metaState: 4 bytes]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_KEYCODE);
    buffer.append(static_cast<char>(action));
    write_be<quint32>(buffer, keyCode);
    write_be<quint32>(buffer, 0); // repeat count, 0 for a single event.
    write_be<quint32>(buffer, metaState);

    send(buffer);
}

void ControlSender::postInjectText(const QString &text)
{
    // Message structure for INJECT_TEXT:
    // [type: 1 byte]
    // [length: 4 bytes]
    // [text: 'length' bytes]
    QByteArray textBytes = text.toUtf8();
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_TEXT);
    write_be<quint32>(buffer, textBytes.length());
    buffer.append(textBytes);

    send(buffer);
}

void ControlSender::postBackOrScreenOn(AndroidKeyEventAction action)
{
    // Message structure for BACK_OR_SCREEN_ON:
    // [type: 1 byte]
    // [action: 1 byte] (for the key event part)
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON);
    buffer.append(static_cast<char>(action));
    send(buffer);
}

void ControlSender::postSetScreenPowerMode(ScreenPowerMode mode)
{
    // Message structure for SET_SCREEN_POWER_MODE:
    // [type: 1 byte]
    // [mode: 1 byte]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE);
    buffer.append(static_cast<char>(mode));

    send(buffer);
}

void ControlSender::postRotateDevice()
{
    // Message structure for ROTATE_DEVICE:
    // [type: 1 byte]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_ROTATE_DEVICE);
    send(buffer);
}

void ControlSender::postExpandNotificationPanel()
{
    // Message structure for EXPAND_NOTIFICATION_PANEL:
    // [type: 1 byte]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL);
    send(buffer);
}

void ControlSender::postCollapseNotificationPanel()
{
    // Message structure for COLLAPSE_NOTIFICATION_PANEL:
    // [type: 1 byte]
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL);
    send(buffer);
}
