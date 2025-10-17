#include<QMainWindow>
#include "devicemanager.h" // 包含头文件
#include "devicewindow.h"
#include <QMap>
#include "scrcpyoptions.h"
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
    // 响应 DeviceManager 的信号，更新设备列表 UI
    void onDevicesUpdated(const QList<DeviceInfo> &devices);

    // 响应 DeviceManager 的信号，在日志区域显示消息
    void onLogMessage(const QString &message);

    // 响应 UI 上的“清空日志”按钮
    void on_btn_clearLogs_clicked();

    // 连接到 UI 上的 "连接选中" 按钮
    void on_btn_connectUSB_clicked();
    // 当一个 DeviceWindow 关闭时，这个槽会被调用
    void onDeviceWindowClosed(const QString &serial);

    void on_comboBox_videoSource_currentIndexChanged(int index);
    void on_btn_saveRecordAs_clicked();

private:
    Ui::MainWindow *ui;
    DeviceManager *mDeviceManager; // 声明 DeviceManager 实例

    // 使用 QMap 来管理所有已打开的设备窗口
    // key 是设备序列号, value 是指向 DeviceWindow 的指针
    QMap<QString, DeviceWindow*> mDeviceWindows;

    ScrcpyOptions gatherScrcpyOptions() const;
    // 新增验证函数
    bool validateOptions(const ScrcpyOptions &options);
};
