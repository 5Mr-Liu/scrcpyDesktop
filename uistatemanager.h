#ifndef UISTATEMANAGER_H
#define UISTATEMANAGER_H

#include <QObject>
#include <QMap>

// 向前声明，避免头文件循环依赖
namespace Ui {
class MainWindow;
}
class DeviceWindow;

class UiStateManager : public QObject
{
    Q_OBJECT
public:
    explicit UiStateManager(Ui::MainWindow *ui, QObject *parent = nullptr);

    void initializeStates();
    void setDeviceWindowsMap(const QMap<QString, DeviceWindow*> *deviceWindows);

public slots:
    // 公共槽，用于连接UI控件信号
    void updateAllControlStates();
    // 公共槽，用于更新设备状态
    void updateConnectedDeviceStatus();
    void addDeviceToStatusTable(const QString &serial);
    void removeDeviceFromStatusTable(const QString &serial);
    void updateDeviceStatusInfo(const QString &serial, const QString &deviceName, const QSize &frameSize);


private:
    void updateVideoControlsState();
    void updateAudioControlsState();
    void updateControlControlsState();
    void updateRecordingControlsState();

    Ui::MainWindow *m_ui;
    // 持有一个指向 MainWindow 中设备窗口Map的指针，用于获取数据
    const QMap<QString, DeviceWindow*> *m_deviceWindows = nullptr;
};

#endif // UISTATEMANAGER_H

