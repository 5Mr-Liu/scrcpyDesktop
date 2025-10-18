// mainwindow.cpp - 完整替换版

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "adbprocess.h"
#include "devicemanager.h"
#include "uistatemanager.h"
#include <QDateTime>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // =================================================================
    // MODIFIED: 更新所有 connect 语句以匹配新的槽函数名
    // 这样就彻底解决了信号槽重复连接的问题
    // =================================================================

    // --- 左侧面板：设备连接与管理 ---
    connect(ui->btn_connectUSB,     &QPushButton::clicked, this, &MainWindow::handleConnectUsbClick);
    connect(ui->btn_enableTCPIP,    &QPushButton::clicked, this, &MainWindow::handleEnableTcpIpClick);
    connect(ui->btn_connectWiFi,    &QPushButton::clicked, this, &MainWindow::handleConnectWifiClick);
    connect(ui->btn_removeWiFi,     &QPushButton::clicked, this, &MainWindow::handleRemoveWifiClick);
    connect(ui->btn_connectSerial,  &QPushButton::clicked, this, &MainWindow::handleConnectSerialClick);
    connect(ui->btn_disconnectAll,  &QPushButton::clicked, this, &MainWindow::handleDisconnectAllClick);
    connect(ui->btn_killADB,        &QPushButton::clicked, this, &MainWindow::handleKillAdbClick);

    // --- 左侧面板：设置与文件 ---
    // **注意**: 你的 UI 文件中按钮是 'btn_saveRecordAs'
    connect(ui->btn_saveRecordAs,   &QPushButton::clicked, this, &MainWindow::handleSaveRecordAsClick);

    // --- 右侧面板：日志 ---
    connect(ui->btn_clearLogs,      &QPushButton::clicked, this, &MainWindow::handleClearLogsClick);


    // --- 核心逻辑连接 (这部分保持不变) ---
    mDeviceManager = new DeviceManager(this);
    connect(ui->btn_refreshUSB, &QPushButton::clicked, mDeviceManager, &DeviceManager::refreshDevices);
    connect(mDeviceManager, &DeviceManager::devicesUpdated, this, &MainWindow::onDevicesUpdated);
    connect(mDeviceManager, &DeviceManager::logMessage, this, &MainWindow::onLogMessage);


    // --- 初始化 UI 默认值 ---
    // (这部分保持不变)
    ui->comboBox_videoSource->setItemData(0, "display");
    ui->comboBox_videoSource->setItemData(1, "camera");
    ui->comboBox_videoCodec->setItemData(0, "h264");
    ui->comboBox_videoCodec->setItemData(1, "h265");
    ui->comboBox_videoCodec->setItemData(2, "av1");
    ui->comboBox_audioSource->setItemData(0, "output");
    ui->comboBox_audioSource->setItemData(1, "playback");
    ui->comboBox_audioSource->setItemData(2, "mic");
    ui->comboBox_audioCodec->setItemData(0, "opus");
    ui->comboBox_audioCodec->setItemData(1, "aac");
    ui->comboBox_audioCodec->setItemData(2, "flac");
    ui->comboBox_audioCodec->setItemData(3, "raw");
    ui->comboBox_cameraFacing->setItemData(0, "any");
    ui->comboBox_cameraFacing->setItemData(1, "front");
    ui->comboBox_cameraFacing->setItemData(2, "back");
    ui->comboBox_cameraFacing->setItemData(3, "external");
    ui->comboBox_keyboardMode->setItemData(0, "sdk");
    ui->comboBox_keyboardMode->setItemData(1, "uhid");
    ui->comboBox_keyboardMode->setItemData(2, "aoa");
    ui->comboBox_keyboardMode->setItemData(3, "disabled");
    ui->comboBox_mouseMode->setItemData(0, "sdk");
    ui->comboBox_mouseMode->setItemData(1, "uhid");
    ui->comboBox_mouseMode->setItemData(2, "aoa");
    ui->comboBox_mouseMode->setItemData(3, "disabled");
    ui->comboBox_recordFormat->setItemData(0, "auto");
    ui->comboBox_recordFormat->setItemData(1, "mp4");
    ui->comboBox_recordFormat->setItemData(2, "mkv");

    mUiStateManager = new UiStateManager(ui, this);
    mUiStateManager->setDeviceWindowsMap(&mDeviceWindows);
    mUiStateManager->initializeStates();

    onLogMessage("欢迎使用 Scrcpy 多设备控制器！");
    onLogMessage("正在进行首次设备扫描...");
    mDeviceManager->refreshDevices();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// FIXED: 修正了设备列表为空时的 UI 提示逻辑
void MainWindow::onDevicesUpdated(const QList<DeviceInfo> &devices)
{
    // 1. 清空两个列表
    ui->listWidget_usbDevices->clear();
    ui->listWidget_wifiDevices->clear();
    // 2. 遍历从 DeviceManager 收到的设备列表
    if (!devices.isEmpty()) {
        for (const DeviceInfo &device : devices) {
            QString itemText = QString("%1 (%2)").arg(device.serial, device.status);
            // =============================================================
            // ▼▼▼  【核心修改】 使用更可靠的方式判断设备类型 ▼▼▼
            // =============================================================
            if (device.serial.contains(':')) {
                // 如果序列号包含 ':', 那么它是一个 WiFi (TCP/IP) 设备
                QListWidgetItem* item = new QListWidgetItem(itemText, ui->listWidget_wifiDevices);
                if (device.status != "device") {
                    item->setForeground(Qt::gray);
                }
            } else {
                // 否则，它是一个 USB 设备
                QListWidgetItem* item = new QListWidgetItem(itemText, ui->listWidget_usbDevices);
                if (device.status == "unauthorized") {
                    item->setForeground(Qt::red);
                    item->setToolTip("设备未授权，请在手机上确认 USB 调试许可。");
                } else if (device.status != "device") {
                    item->setForeground(Qt::gray);
                }
            }
            // =============================================================
            // ▲▲▲  【修改结束】 ▲▲▲
            // =============================================================
        }
    }
    // 3. 循环结束后，检查每个列表是否为空，如果为空则添加提示信息
    if (ui->listWidget_usbDevices->count() == 0) {
        ui->listWidget_usbDevices->addItem("无 USB 设备");
    }
    if (ui->listWidget_wifiDevices->count() == 0) {
        ui->listWidget_wifiDevices->addItem("无 WiFi 设备");
    }
}

void MainWindow::onLogMessage(const QString &message)
{
    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit_logs->appendPlainText(QString("[%1] %2").arg(currentTime, message));
}

// MODIFIED: Renamed slot
void MainWindow::handleClearLogsClick()
{
    ui->textEdit_logs->clear();
}

// MODIFIED: Renamed slot
void MainWindow::handleConnectUsbClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("提示：请先在列表中选择要连接的 USB 设备。");
        return;
    }
    for (QListWidgetItem *item : selectedItems) {
        QString itemText = item->text();
        QString serial = itemText.left(itemText.indexOf(' '));
        if (!itemText.contains("(device)")) {
            onLogMessage(QString("警告：设备 %1 状态不是 'device'，无法连接。").arg(serial));
            continue;
        }
        startDeviceWindow(serial);
    }
}

void MainWindow::onDeviceWindowClosed(const QString &serial)
{
    onLogMessage(QString("设备 %1 的窗口已关闭。").arg(serial));
    mDeviceWindows.remove(serial);
    mUiStateManager->removeDeviceFromStatusTable(serial);
    mUiStateManager->updateConnectedDeviceStatus();
}

ScrcpyOptions MainWindow::gatherScrcpyOptions() const
{
    // این قسمت بدون تغییر است (This part is unchanged)
    ScrcpyOptions opts;
    opts.video_source = ui->comboBox_videoSource->currentData().toString();
    opts.max_size = ui->spinBox_maxSize->value();
    opts.video_bit_rate = ui->spinBox_videoBitrate->value() * 1000000;
    opts.max_fps = ui->spinBox_maxFPS->value();
    opts.video_codec = ui->comboBox_videoCodec->currentData().toString();
    opts.display_id = ui->spinBox_displayID->value();
    opts.video = !ui->checkBox_noVideo->isChecked();
    opts.no_video_playback = ui->checkBox_noVideoPlayback->isChecked();
    opts.audio_source = ui->comboBox_audioSource->currentData().toString();
    opts.audio_bit_rate = ui->spinBox_audioBitrate->value() * 1000;
    opts.audio_codec = ui->comboBox_audioCodec->currentData().toString();
    opts.audio_buffer = ui->spinBox_audioBuffer->value();
    opts.audio_dup = ui->checkBox_audioDup->isChecked();
    opts.audio = !ui->checkBox_noAudio->isChecked();
    opts.require_audio = ui->checkBox_requireAudio->isChecked();
    opts.camera_id = ui->comboBox_cameraID->currentText();
    opts.camera_facing = ui->comboBox_cameraFacing->currentData().toString();
    opts.camera_size = ui->comboBox_cameraSize->currentText();
    opts.camera_fps = ui->spinBox_cameraFPS->value();
    opts.camera_high_speed = ui->checkBox_cameraHighSpeed->isChecked();
    opts.control = !ui->checkBox_noControl->isChecked();
    opts.stay_awake = ui->checkBox_stayAwake->isChecked();
    opts.power_off_on_close = ui->checkBox_powerOffOnClose->isChecked();
    opts.show_touches = ui->checkBox_showTouches->isChecked();
    opts.clipboard_autosync = !ui->checkBox_noClipboardAutosync->isChecked();
    opts.keyboard_mode = ui->comboBox_keyboardMode->currentData().toString();
    opts.mouse_mode = ui->comboBox_mouseMode->currentData().toString();
    opts.otg = ui->checkBox_otg->isChecked();
    opts.fullscreen = ui->checkBox_fullscreen->isChecked();
    opts.always_on_top = ui->checkBox_alwaysOnTop->isChecked();
    opts.window_borderless = ui->checkBox_windowBorderless->isChecked();
    opts.window_title = ui->lineEdit_windowTitle->text();
    opts.record_file = ui->lineEdit_recordFile->text();
    opts.record_format = ui->comboBox_recordFormat->currentData().toString();
    opts.no_playback = ui->checkBox_noPlayback->isChecked();
    return opts;
}

bool MainWindow::validateOptions(const ScrcpyOptions &options)
{
    // (This part is unchanged)
    if (!options.record_file.isEmpty()) {
        QFileInfo fileInfo(options.record_file);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            QMessageBox::warning(this, "配置错误", QString("指定的录制文件目录不存在！\n请检查路径: %1").arg(dir.path()));
            return false;
        }
    }
    if (options.video_source == "camera") {
        if (options.camera_id.isEmpty() && options.camera_facing == "any") {
            QMessageBox::warning(this, "配置错误", "您选择了摄像头作为视频源，但未指定摄像头ID或方向 (前置/后置)。");
            return false;
        }
    }
    if (!options.video && !options.audio) {
        auto reply = QMessageBox::question(this, "确认操作", "您同时禁用了视频和音频，这意味着连接后将没有任何内容显示或播放 (仅控制)。\n确定要继续吗？", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return false;
        }
    }
    return true;
}

// MODIFIED: Renamed slot (was on_pushButton_recordFile_clicked)
void MainWindow::handleSaveRecordAsClick()
{
    QString defaultPath = QDir::homePath();
    QString fileName = QString("scrcpy_record_%1.mkv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "选择录制文件保存位置", QDir(defaultPath).filePath(fileName), "Matroska Video (*.mkv);;MP4 Video (*.mp4)");

    if (!filePath.isEmpty()) {
        ui->lineEdit_recordFile->setText(filePath);
        if (filePath.endsWith(".mp4", Qt::CaseInsensitive)) {
            ui->comboBox_recordFormat->setCurrentText("mp4");
        } else {
            ui->comboBox_recordFormat->setCurrentText("mkv");
        }
    }
}

void MainWindow::startDeviceWindow(const QString &serial)
{
    // (This part is unchanged)
    if (mDeviceWindows.contains(serial)) {
        onLogMessage(QString("提示：设备 %1 的窗口已经打开。").arg(serial));
        mDeviceWindows[serial]->activateWindow();
        mDeviceWindows[serial]->raise();
        return;
    }
    ScrcpyOptions options = gatherScrcpyOptions();
    if (!validateOptions(options)) {
        onLogMessage("错误：配置参数验证失败，已取消连接。");
        return;
    }
    onLogMessage(QString("正在为设备 %1 创建窗口...").arg(serial));
    DeviceWindow *deviceWindow = new DeviceWindow(serial, options, nullptr);
    connect(deviceWindow, &DeviceWindow::windowClosed, this, &MainWindow::onDeviceWindowClosed);
    connect(deviceWindow, &DeviceWindow::statusUpdated, mUiStateManager, &UiStateManager::updateDeviceStatusInfo);
    mDeviceWindows.insert(serial, deviceWindow);
    mUiStateManager->addDeviceToStatusTable(serial);
    mUiStateManager->updateConnectedDeviceStatus();
    deviceWindow->show();
}

// MODIFIED: Renamed slot and FIXED critical bug (double delete)
void MainWindow::handleEnableTcpIpClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("警告: 请先选择一个 USB 设备以启用 TCP/IP。");
        QMessageBox::warning(this, "操作失败", "请先在 USB 设备列表中选择一个设备。");
        return;
    }
    QString serial = selectedItems.first()->text().left(selectedItems.first()->text().indexOf(' '));
    onLogMessage(QString("正在为设备 %1 启用 TCP/IP (端口 5555)...").arg(serial));
    AdbProcess *process = new AdbProcess(this);

    // FIXED: Only one connection to 'finished' signal to handle everything.
    connect(process, &AdbProcess::finished, this, [this, process, serial](int exitCode, QProcess::ExitStatus exitStatus) {
        QString output = process->readAllStandardOutput() + process->readAllStandardError();
        if (exitStatus == QProcess::NormalExit && output.contains("restarting in TCP mode port: 5555")) {
            onLogMessage(QString("成功: 设备 %1 已在 5555 端口开启 TCP/IP 模式。").arg(serial));
            QMessageBox::information(this, "成功", "TCP/IP 模式已在5555端口启动。\n请获取设备IP地址，并在 WiFi 连接 Tab 中进行连接。");
        } else {
            // FIXED: Corrected log message format string
            onLogMessage(QString("失败: 无法为设备 %1 开启 TCP/IP 模式。错误: %2").arg(serial, output.trimmed()));
            QMessageBox::critical(this, "失败", "启动 TCP/IP 模式失败！\n" + output);
        }
        process->deleteLater(); // The process is deleted here, and only here.
    });

    process->execute(serial, {"tcpip", "5555"});
}

// MODIFIED: Renamed slot
void MainWindow::handleConnectWifiClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_wifiDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        QString ip = ui->lineEdit_ipAddress->text().trimmed();
        if (ip.isEmpty()) {
            onLogMessage("提示: 请在 WiFi 列表中选择设备，或输入 IP 地址进行连接。");
            return;
        }
        QString port = QString::number(ui->spinBox_tcpPort->value());
        QString fullAddress = ip.contains(':') ? ip : (ip + ":" + port);

        onLogMessage(QString("正在尝试通过 adb connect 连接到 %1...").arg(fullAddress));
        AdbProcess *process = new AdbProcess(this);
        connect(process, &AdbProcess::finished, this, [this, process, fullAddress](int exitCode, QProcess::ExitStatus exitStatus){
            QString output = process->readAllStandardOutput() + process->readAllStandardError();
            if (exitStatus == QProcess::NormalExit && (output.contains("connected to") || output.contains("already connected"))) {
                onLogMessage(QString("成功连接到 %1").arg(fullAddress));
                QMessageBox::information(this, "成功", "无线连接成功或已连接！");
            } else {
                onLogMessage(QString("连接 %1 失败: %2").arg(fullAddress, output.trimmed()));
                QMessageBox::critical(this, "失败", "无线连接失败！\n" + output);
            }
            mDeviceManager->refreshDevices();
            process->deleteLater();
        });
        process->execute("", {"connect", fullAddress});
    } else {
        for (QListWidgetItem *item : selectedItems) {
            QString serial = item->text().left(item->text().indexOf(' '));
            startDeviceWindow(serial);
        }
    }
}

// MODIFIED: Renamed slot and OPTIMIZED logic
void MainWindow::handleRemoveWifiClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_wifiDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("提示: 请先在 WiFi 列表中选择要断开的设备。");
        return;
    }
    for (QListWidgetItem* item : selectedItems) {
        QString serial = item->text().left(item->text().indexOf(' '));
        onLogMessage(QString("正在断开设备 %1...").arg(serial));
        AdbProcess *process = new AdbProcess(this);

        // OPTIMIZED: Combined both actions into one lambda
        connect(process, &AdbProcess::finished, this, [this, process](){
            mDeviceManager->refreshDevices(); // Refresh list after disconnect
            process->deleteLater();
        });

        process->execute("", {"disconnect", serial});
    }
}

// MODIFIED: Renamed slot
void MainWindow::handleConnectSerialClick()
{
    QString serial = ui->lineEdit_serial->text().trimmed();
    if (serial.isEmpty()) {
        onLogMessage("警告: 请输入要连接的设备序列号。");
        QMessageBox::warning(this, "输入错误", "请输入有效的设备序列号。");
        return;
    }
    startDeviceWindow(serial);
}

// MODIFIED: Renamed slot and OPTIMIZED logic
void MainWindow::handleDisconnectAllClick()
{
    onLogMessage("正在执行 adb disconnect...");
    AdbProcess* process = new AdbProcess(this);

    // OPTIMIZED: Combined both actions into one lambda
    connect(process, &AdbProcess::finished, this, [this, process](){
        mDeviceManager->refreshDevices();
        process->deleteLater();
    });

    process->execute("", {"disconnect"});
}

// MODIFIED: Renamed slot
void MainWindow::handleKillAdbClick()
{
    onLogMessage("正在重启 ADB 服务 (kill-server & start-server)...");
    AdbProcess *killProcess = new AdbProcess(this);
    connect(killProcess, &AdbProcess::finished, this, [this](int, QProcess::ExitStatus){
        onLogMessage("ADB 服务已停止。正在启动...");
        AdbProcess* startProcess = new AdbProcess(this);
        connect(startProcess, &AdbProcess::finished, this, [this, startProcess](){
            onLogMessage("ADB 服务已启动。正在刷新设备列表...");
            mDeviceManager->refreshDevices();
            startProcess->deleteLater();
        });
        startProcess->execute("", {"start-server"});
    });
    connect(killProcess, &AdbProcess::finished, killProcess, &AdbProcess::deleteLater);
    killProcess->execute("", {"kill-server"});
}
