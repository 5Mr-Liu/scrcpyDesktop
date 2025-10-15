#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 创建 DeviceManager 实例
    mDeviceManager = new DeviceManager(this);

    // 2. 建立信号-槽连接
    //   - 点击"刷新"按钮 -> 触发 DeviceManager 开始扫描
    connect(ui->btn_refreshUSB, &QPushButton::clicked, mDeviceManager, &DeviceManager::refreshDevices);

    //   - DeviceManager 完成扫描 -> 触发 MainWindow 更新 UI
    connect(mDeviceManager, &DeviceManager::devicesUpdated, this, &MainWindow::onDevicesUpdated);

    //   - 连接日志信号
    connect(mDeviceManager, &DeviceManager::logMessage, this, &MainWindow::onLogMessage);

    // （可选）添加一个欢迎日志
    onLogMessage("欢迎使用 Scrcpy 多设备控制器！");

    // 3. 应用程序启动时，自动进行一次设备扫描
    onLogMessage("正在进行首次设备扫描...");
    mDeviceManager->refreshDevices();
}

MainWindow::~MainWindow()
{
    delete ui;
    // mDeviceManager 会因为是 MainWindow 的子对象而被自动销毁
}

// 实现槽函数，用于更新 USB 设备列表
void MainWindow::onDevicesUpdated(const QList<DeviceInfo> &devices)
{
    // 清空当前的列表
    ui->listWidget_usbDevices->clear();

    if (devices.isEmpty()) {
        ui->listWidget_usbDevices->addItem("未检测到 USB 设备");
        return;
    }

    // 遍历新的设备列表并添加到 UI 控件中
    for (const DeviceInfo &device : devices) {
        QString itemText = QString("%1 (%2)").arg(device.serial, device.status);
        QListWidgetItem* item = new QListWidgetItem(itemText, ui->listWidget_usbDevices);
        // 这里可以根据设备状态设置不同的图标或颜色
        if (device.status == "unauthorized") {
            item->setForeground(Qt::red);
            item->setToolTip("设备未授权，请在手机上确认 USB 调试许可。");
        } else if (device.status == "offline") {
            item->setForeground(Qt::gray);
        }
    }
}

// 实现槽函数，用于在日志区域添加消息
void MainWindow::onLogMessage(const QString &message)
{
    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit_logs->appendPlainText(QString("[%1] %2").arg(currentTime, message));
}

// UI 设计器自动生成的槽，如果不存在，手动连接
void MainWindow::on_btn_clearLogs_clicked()
{
    ui->textEdit_logs->clear();
}

void MainWindow::on_btn_connectUSB_clicked()
{
    // 获取在 listWidget_usbDevices 中所有被选中的项
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("提示：请先在列表中选择要连接的设备。");
        return;
    }
    for (QListWidgetItem *item : selectedItems) {
        // 从列表项的文本中解析出序列号
        // 我们的格式是 "serial (status)"
        QString itemText = item->text();
        QString serial = itemText.left(itemText.indexOf(' '));
        // 检查设备是否是有效的 "device" 状态
        if (!itemText.contains("(device)")) {
            onLogMessage(QString("警告：设备 %1 状态不是 'device'，无法连接。").arg(serial));
            continue; // 跳过这个设备
        }
        // 检查这个设备的窗口是否已经打开
        if (mDeviceWindows.contains(serial)) {
            onLogMessage(QString("提示：设备 %1 的窗口已经打开。").arg(serial));
            // 可以选择将已有的窗口激活并提到最前
            mDeviceWindows[serial]->activateWindow();
            mDeviceWindows[serial]->raise();
            continue; // 跳过这个设备
        }
        onLogMessage(QString("正在为设备 %1 创建窗口...").arg(serial));
        // 创建一个新的 DeviceWindow 实例
        DeviceWindow *deviceWindow = new DeviceWindow(serial);

        // 监听这个新窗口的关闭信号
        connect(deviceWindow, &DeviceWindow::windowClosed, this, &MainWindow::onDeviceWindowClosed);
        // 将它添加到我们的管理器中
        mDeviceWindows.insert(serial, deviceWindow);
        // 显示窗口
        deviceWindow->show();
    }
}

// DeviceWindow 关闭时调用的槽函数
void MainWindow::onDeviceWindowClosed(const QString &serial)
{
    onLogMessage(QString("设备 %1 的窗口已关闭。").arg(serial));

    // 从我们的管理器中移除这个窗口的记录
    // 注意：此时窗口对象还存在，但很快会被 Qt 自动销毁
    mDeviceWindows.remove(serial);
    // 如果需要，可以在这里更新状态栏等信息
}
