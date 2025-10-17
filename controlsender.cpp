#include "controlsender.h"
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>
#include <QtEndian>

// 辅助函数，用于将数据写入 QByteArray (大端字节序)
template<typename T>
void write_be(QByteArray &arr, T value) {
    T big_endian_value = qToBigEndian(value);
    arr.append(reinterpret_cast<const char*>(&big_endian_value), sizeof(T));
}

ControlSender::ControlSender(QObject *parent) : QObject(parent)
{
    mControlSocket = new QTcpSocket(this);
    connect(mControlSocket, &QTcpSocket::connected, this, &ControlSender::controlSocketConnected);
    connect(mControlSocket, &QTcpSocket::disconnected, this, &ControlSender::controlSocketDisconnected);
}

ControlSender::~ControlSender()
{
}

void ControlSender::connectToServer(const QString &host, quint16 port)
{
    if (mControlSocket->state() == QAbstractSocket::UnconnectedState) {
        qDebug() << "[Control] Connecting to" << host << ":" << port;
        mControlSocket->connectToHost(host, port);
    }
}

void ControlSender::disconnectFromServer()
{
    if (mControlSocket->state() != QAbstractSocket::UnconnectedState) {
        mControlSocket->disconnectFromHost();
    }
}

qint64 ControlSender::send(const QByteArray &data)
{
    if (mControlSocket->state() == QAbstractSocket::ConnectedState) {
        return mControlSocket->write(data);
    }
    return -1;
}

void ControlSender::postInjectTouch(AndroidMotionEventAction action, QPoint pos, QSize screenSize)
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT);
    buffer.append(static_cast<char>(action)); // action
    write_be<qint64>(buffer, -1); // pointerId, -1 for virtual finger
    write_be<quint32>(buffer, pos.x());
    write_be<quint32>(buffer, pos.y());
    write_be<quint16>(buffer, screenSize.width());
    write_be<quint16>(buffer, screenSize.height());
    write_be<quint16>(buffer, 0xFFFF); // pressure
    write_be<quint32>(buffer, 1); // action button (AMOTION_EVENT_BUTTON_PRIMARY)
    write_be<quint32>(buffer, 1); // buttons state (AMOTION_EVENT_BUTTON_PRIMARY)

    send(buffer);
}

void ControlSender::postInjectKeycode(AndroidKeyEventAction action, int keyCode, int metaState)
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_KEYCODE);
    buffer.append(static_cast<char>(action));
    write_be<quint32>(buffer, keyCode);
    write_be<quint32>(buffer, 0); // repeat
    write_be<quint32>(buffer, metaState);

    send(buffer);
}

void ControlSender::postInjectText(const QString &text)
{
    QByteArray textBytes = text.toUtf8();
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_INJECT_TEXT);
    write_be<quint32>(buffer, textBytes.length());
    buffer.append(textBytes);

    send(buffer);
}

void ControlSender::postBackOrScreenOn(AndroidKeyEventAction action)
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON);
    buffer.append(static_cast<char>(action));
    send(buffer);
}

void ControlSender::postSetScreenPowerMode(ScreenPowerMode mode)
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE);
    buffer.append(static_cast<char>(mode));

    send(buffer);
}

void ControlSender::postRotateDevice()
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_ROTATE_DEVICE);
    send(buffer);
}

void ControlSender::postExpandNotificationPanel()
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL);
    send(buffer);
}

void ControlSender::postCollapseNotificationPanel()
{
    QByteArray buffer;
    buffer.append(CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL);
    send(buffer);
}
