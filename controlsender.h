#ifndef CONTROLSENDER_H
#define CONTROLSENDER_H

#include <QObject>
#include <QPoint>
#include <QSize>

class QTcpSocket;

// Scrcpy 控制消息类型定义
enum ControlMsgType {
    CONTROL_MSG_TYPE_INJECT_KEYCODE,
    CONTROL_MSG_TYPE_INJECT_TEXT,
    CONTROL_MSG_TYPE_INJECT_TOUCH_EVENT,
    CONTROL_MSG_TYPE_INJECT_SCROLL_EVENT,
    CONTROL_MSG_TYPE_BACK_OR_SCREEN_ON,
    CONTROL_MSG_TYPE_EXPAND_NOTIFICATION_PANEL,
    CONTROL_MSG_TYPE_COLLAPSE_NOTIFICATION_PANEL,
    CONTROL_MSG_TYPE_GET_CLIPBOARD,
    CONTROL_MSG_TYPE_SET_CLIPBOARD,
    CONTROL_MSG_TYPE_SET_SCREEN_POWER_MODE,
    CONTROL_MSG_TYPE_ROTATE_DEVICE,
};

// Android 按键动作
enum AndroidKeyEventAction {
    AKEY_EVENT_ACTION_DOWN = 0,
    AKEY_EVENT_ACTION_UP = 1,
};

// Android 触摸动作
enum AndroidMotionEventAction {
    AMOTION_EVENT_ACTION_DOWN = 0,
    AMOTION_EVENT_ACTION_UP = 1,
    AMOTION_EVENT_ACTION_MOVE = 2,
};

// Android 电源模式
enum ScreenPowerMode {
    SCREEN_POWER_MODE_OFF = 0,
    SCREEN_POWER_MODE_DOZE = 1,
    SCREEN_POWER_MODE_NORMAL = 2,
};


class ControlSender : public QObject
{
    Q_OBJECT
public:
    explicit ControlSender(QObject *parent = nullptr);
    ~ControlSender();

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();

    // -- 公共接口，用于发送各种控制命令 --
    void postInjectTouch(AndroidMotionEventAction action, QPoint pos, QSize screenSize);
    void postInjectKeycode(AndroidKeyEventAction action, int keyCode, int metaState = 0);
    void postInjectText(const QString &text);
    void postBackOrScreenOn(AndroidKeyEventAction action);
    void postSetScreenPowerMode(ScreenPowerMode mode);
    void postRotateDevice();
    void postExpandNotificationPanel();
    void postCollapseNotificationPanel();
    // ... 其他命令可按需添加

signals:
    void controlSocketConnected();
    void controlSocketDisconnected();

private:
    qint64 send(const QByteArray &data);
    QTcpSocket *mControlSocket;
};

#endif // CONTROLSENDER_H
