#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

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


    // UI 联动: 视频源选择 -> 摄像头设置
    connect(ui->comboBox_videoSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_comboBox_videoSource_currentIndexChanged);
    // 录制文件另存为按钮
    connect(ui->btn_saveRecordAs, &QPushButton::clicked, this, &MainWindow::on_btn_saveRecordAs_clicked);
    // 初始化 ComboBox 的 userData，这比用 text 判断更可靠
    // 视频源
    ui->comboBox_videoSource->setItemData(0, "display");
    ui->comboBox_videoSource->setItemData(1, "camera");
    // 视频编码
    ui->comboBox_videoCodec->setItemData(0, "h264");
    ui->comboBox_videoCodec->setItemData(1, "h265");
    ui->comboBox_videoCodec->setItemData(2, "av1");
    // 音频源 (部分示例)
    ui->comboBox_audioSource->setItemData(0, "output");
    ui->comboBox_audioSource->setItemData(1, "playback");
    ui->comboBox_audioSource->setItemData(2, "mic");
    // 音频编码
    ui->comboBox_audioCodec->setItemData(0, "opus");
    ui->comboBox_audioCodec->setItemData(1, "aac");
    ui->comboBox_audioCodec->setItemData(2, "flac");
    ui->comboBox_audioCodec->setItemData(3, "raw");
    // 摄像头方向
    ui->comboBox_cameraFacing->setItemData(0, "any");
    ui->comboBox_cameraFacing->setItemData(1, "front");
    ui->comboBox_cameraFacing->setItemData(2, "back");
    ui->comboBox_cameraFacing->setItemData(3, "external");
    // 键盘/鼠标模式
    ui->comboBox_keyboardMode->setItemData(0, "sdk");
    ui->comboBox_keyboardMode->setItemData(1, "uhid");
    ui->comboBox_keyboardMode->setItemData(2, "aoa");
    ui->comboBox_keyboardMode->setItemData(3, "disabled");
    ui->comboBox_mouseMode->setItemData(0, "sdk");
    ui->comboBox_mouseMode->setItemData(1, "uhid");
    ui->comboBox_mouseMode->setItemData(2, "aoa");
    ui->comboBox_mouseMode->setItemData(3, "disabled");
    // 录制格式
    ui->comboBox_recordFormat->setItemData(0, "auto");
    ui->comboBox_recordFormat->setItemData(1, "mp4");
    ui->comboBox_recordFormat->setItemData(2, "mkv");

    // 1. 禁用视频
    connect(ui->checkBox_noVideo, &QCheckBox::toggled, this, [this](bool checked){
        QList<QWidget*> childWidgets = ui->groupBox_videoSettings->findChildren<QWidget*>();
        for (QWidget* widget : childWidgets) {
            // 唯独跳过 checkBox_noVideo 本身
            if (widget != ui->checkBox_noVideo) {
                widget->setEnabled(!checked);
            }
        }
        // 如果禁用了视频，那么摄像头源和摄像头设置也应该被禁用
        if (checked) {
            ui->comboBox_videoSource->setEnabled(false);
            ui->groupBox_cameraSettings->setEnabled(false);
        } else {
            ui->comboBox_videoSource->setEnabled(true);
            // 重新评估摄像头设置是否应该启用
            on_comboBox_videoSource_currentIndexChanged(ui->comboBox_videoSource->currentIndex());
        }
    });
    // 2. 禁用音频
    connect(ui->checkBox_noAudio, &QCheckBox::toggled, ui->groupBox_audioSettings, &QGroupBox::setDisabled);
    // 3. 录制文件路径
    connect(ui->lineEdit_recordFile, &QLineEdit::textChanged, this, [this](const QString &text){
        // 只有当录制文件路径不为空时，才允许选择录制格式
        ui->comboBox_recordFormat->setEnabled(!text.isEmpty());
    });
    // 初始化状态
    ui->comboBox_recordFormat->setEnabled(!ui->lineEdit_recordFile->text().isEmpty());
    // 4. OTG 模式与其他控制选项的互斥
    connect(ui->checkBox_otg, &QCheckBox::toggled, this, [this](bool checked) {
        // OTG 模式下，很多常规控制选项会失效，应该禁用它们
        ui->checkBox_noControl->setEnabled(!checked);
        ui->checkBox_stayAwake->setEnabled(!checked);
        ui->checkBox_turnScreenOff->setEnabled(!checked);
        ui->checkBox_powerOffOnClose->setEnabled(!checked);
        ui->checkBox_showTouches->setEnabled(!checked);
        ui->comboBox_keyboardMode->setEnabled(!checked);
        ui->comboBox_mouseMode->setEnabled(!checked);
        if(checked) {
            onLogMessage("提示: OTG模式下，将禁用大部分常规控制选项。");
        }
    });
    // 5. 音频双路输出仅对 playback 源有效
    connect(ui->comboBox_audioSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        QString source = ui->comboBox_audioSource->itemData(index).toString();
        ui->checkBox_audioDup->setEnabled(source == "playback");
        if(source != "playback") {
            ui->checkBox_audioDup->setChecked(false); // 如果不是 playback，自动取消勾选
        }
    });

    // 初始化状态
    ui->checkBox_audioDup->setEnabled(ui->comboBox_audioSource->currentData().toString() == "playback");
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
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("提示：请先在列表中选择要连接的设备。");
        return;
    }
    ScrcpyOptions options = gatherScrcpyOptions();
    // ====================== 在这里调用验证函数 ======================
    if (!validateOptions(options)) {
        onLogMessage("错误：配置参数验证失败，已取消连接。");
        return; // 中断执行
    }
    // =============================================================
    for (QListWidgetItem *item : selectedItems) {
        // ... (后面的循环逻辑保持不变)
        QString itemText = item->text();
        QString serial = itemText.left(itemText.indexOf(' '));
        if (!itemText.contains("(device)")) {
            onLogMessage(QString("警告：设备 %1 状态不是 'device'，无法连接。").arg(serial));
            continue;
        }
        if (mDeviceWindows.contains(serial)) {
            onLogMessage(QString("提示：设备 %1 的窗口已经打开。").arg(serial));
            mDeviceWindows[serial]->activateWindow();
            mDeviceWindows[serial]->raise();
            continue;
        }
        onLogMessage(QString("正在为设备 %1 创建窗口...").arg(serial));
        DeviceWindow *deviceWindow = new DeviceWindow(serial, options, this); // 建议将 this 作为 parent
        connect(deviceWindow, &DeviceWindow::windowClosed, this, &MainWindow::onDeviceWindowClosed);
        mDeviceWindows.insert(serial, deviceWindow);
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


ScrcpyOptions MainWindow::gatherScrcpyOptions() const
{
    ScrcpyOptions opts;
    // --- 视频设置 ---
    opts.video_source = ui->comboBox_videoSource->currentData().toString();
    opts.max_size = ui->spinBox_maxSize->value();
    // UI单位是Mbps, Scrcpy需要bps
    opts.video_bit_rate = ui->spinBox_videoBitrate->value() * 1000000;
    opts.max_fps = ui->spinBox_maxFPS->value();
    opts.video_codec = ui->comboBox_videoCodec->currentData().toString();
    opts.display_id = ui->spinBox_displayID->value();
    opts.video = !ui->checkBox_noVideo->isChecked();
    opts.no_video_playback = ui->checkBox_noVideoPlayback->isChecked();
    // --- 音频设置 ---
    opts.audio_source = ui->comboBox_audioSource->currentData().toString();
    opts.audio_bit_rate = ui->spinBox_audioBitrate->value() * 1000; // Kbps -> bps
    opts.audio_codec = ui->comboBox_audioCodec->currentData().toString();
    opts.audio_buffer = ui->spinBox_audioBuffer->value();
    opts.audio_dup = ui->checkBox_audioDup->isChecked();
    opts.audio = !ui->checkBox_noAudio->isChecked();
    opts.require_audio = ui->checkBox_requireAudio->isChecked();
    // --- 摄像头设置 ---
    opts.camera_id = ui->comboBox_cameraID->currentText(); // ID 通常是字符串
    opts.camera_facing = ui->comboBox_cameraFacing->currentData().toString();
    opts.camera_size = ui->comboBox_cameraSize->currentText(); // "1920:1080"
    opts.camera_fps = ui->spinBox_cameraFPS->value();
    opts.camera_high_speed = ui->checkBox_cameraHighSpeed->isChecked();
    // --- 控制与行为 ---
    opts.control = !ui->checkBox_noControl->isChecked();
    opts.stay_awake = ui->checkBox_stayAwake->isChecked();
    opts.power_off_on_close = ui->checkBox_powerOffOnClose->isChecked();
    opts.show_touches = ui->checkBox_showTouches->isChecked();
    opts.clipboard_autosync = !ui->checkBox_noClipboardAutosync->isChecked();
    opts.keyboard_mode = ui->comboBox_keyboardMode->currentData().toString();
    opts.mouse_mode = ui->comboBox_mouseMode->currentData().toString();
    opts.otg = ui->checkBox_otg->isChecked();
    // --- 窗口与录制 ---
    opts.fullscreen = ui->checkBox_fullscreen->isChecked();
    opts.always_on_top = ui->checkBox_alwaysOnTop->isChecked();
    opts.window_borderless = ui->checkBox_windowBorderless->isChecked();
    opts.window_title = ui->lineEdit_windowTitle->text();
    opts.record_file = ui->lineEdit_recordFile->text();
    opts.record_format = ui->comboBox_recordFormat->currentData().toString();
    opts.no_playback = ui->checkBox_noPlayback->isChecked();
    return opts;
}


void MainWindow::on_comboBox_videoSource_currentIndexChanged(int index)
{
    // 根据选择的视频源，启用或禁用摄像头设置
    QString source = ui->comboBox_videoSource->itemData(index).toString();
    ui->groupBox_cameraSettings->setEnabled(source == "camera");
}
void MainWindow::on_btn_saveRecordAs_clicked()
{
    QString defaultPath = QDir::homePath();
    QString fileName = QString("record_%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "选择录制文件保存路径",
                                                    QDir(defaultPath).filePath(fileName),
                                                    "Video Files (*.mp4 *.mkv);;All Files (*)");
    if (!filePath.isEmpty()) {
        ui->lineEdit_recordFile->setText(filePath);
    }
}

bool MainWindow::validateOptions(const ScrcpyOptions &options)
{
    // 验证 1: 录制文件路径
    if (!options.record_file.isEmpty()) {
        QFileInfo fileInfo(options.record_file);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            QMessageBox::warning(this, "配置错误",
                                 QString("指定的录制文件目录不存在！\n请检查路径: %1").arg(dir.path()));
            return false; // 验证失败
        }
    }
    // 验证 2: 摄像头设置
    if (options.video_source == "camera") {
        if (options.camera_id.isEmpty() && options.camera_facing == "any") {
            QMessageBox::warning(this, "配置错误",
                                 "您选择了摄像头作为视频源，但未指定摄像头ID或方向 (前置/后置)。");
            return false;
        }
    }
    // 验证 3: 如果禁用了视频和音频，给出提示
    if (!options.video && !options.audio) {
        auto reply = QMessageBox::question(this, "确认操作",
                                           "您同时禁用了视频和音频，这意味着连接后将没有任何内容显示或播放 (仅控制)。\n确定要继续吗？",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return false;
        }
    }
    // ... 未来可以添加更多复杂的验证规则 ...
    return true; // 所有验证通过
}
