// mainwindow.h - 完整替换版

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- 核心逻辑槽 (非按钮点击) ---
    void onDevicesUpdated(const QList<DeviceInfo> &devices);
    void onLogMessage(const QString &message);
    void onDeviceWindowClosed(const QString &serial);

    // --- 按钮点击处理槽 (已重命名以避免自动连接) ---
    // USB Tab
    void handleConnectUsbClick();
    void handleEnableTcpIpClick();

    // WiFi Tab
    void handleConnectWifiClick();
    void handleRemoveWifiClick();

    // Serial Tab
    void handleConnectSerialClick();

    // 全局操作
    void handleDisconnectAllClick();
    void handleKillAdbClick();

    // 录制与文件
    // **注意**: 你的 UI 文件中按钮是 'btn_saveRecordAs'
    // 你的 cpp 文件中实现是 'on_pushButton_recordFile_clicked'
    // 我们将它们统一起来
    void handleSaveRecordAsClick();

    // 日志
    void handleClearLogsClick();

private:
    Ui::MainWindow *ui;
    DeviceManager *mDeviceManager;
    QMap<QString, DeviceWindow*> mDeviceWindows;
    UiStateManager *mUiStateManager;

    ScrcpyOptions gatherScrcpyOptions() const;
    bool validateOptions(const ScrcpyOptions &options);
    void startDeviceWindow(const QString &serial);
};

#endif // MAINWINDOW_H
