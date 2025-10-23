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
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/assert/title.ico"));

    // --- Connect buttons from the left panel and log panel ---
    connect(ui->btn_connectUSB,     &QPushButton::clicked, this, &MainWindow::handleConnectUsbClick);
    connect(ui->btn_enableTCPIP,    &QPushButton::clicked, this, &MainWindow::handleEnableTcpIpClick);
    connect(ui->btn_connectWiFi,    &QPushButton::clicked, this, &MainWindow::handleConnectWifiClick);
    connect(ui->btn_removeWiFi,     &QPushButton::clicked, this, &MainWindow::handleRemoveWifiClick);
    connect(ui->btn_connectSerial,  &QPushButton::clicked, this, &MainWindow::handleConnectSerialClick);
    connect(ui->btn_disconnectAll,  &QPushButton::clicked, this, &MainWindow::handleDisconnectAllClick);
    connect(ui->btn_killADB,        &QPushButton::clicked, this, &MainWindow::handleKillAdbClick);
    connect(ui->btn_saveRecordAs,   &QPushButton::clicked, this, &MainWindow::handleSaveRecordAsClick);
    connect(ui->btn_clearLogs,      &QPushButton::clicked, this, &MainWindow::handleClearLogsClick);

    // --- Connect menu bar actions ---
    setupMenuConnections(); // Call the new setup function

    // --- Core logic connections ---
    mDeviceManager = new DeviceManager(this);
    connect(ui->btn_refreshUSB, &QPushButton::clicked, mDeviceManager, &DeviceManager::refreshDevices);
    // Also connect the menu action to the refresh logic
    connect(ui->action_refreshUSB, &QAction::triggered, mDeviceManager, &DeviceManager::refreshDevices);
    connect(mDeviceManager, &DeviceManager::devicesUpdated, this, &MainWindow::onDevicesUpdated);
    connect(mDeviceManager, &DeviceManager::logMessage, this, &MainWindow::onLogMessage);

    // --- Initialize UI default values (using itemData for robustness) ---
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

    onLogMessage("Welcome to the Scrcpy Multi-Device Controller!");
    onLogMessage("Performing initial device scan...");
    mDeviceManager->refreshDevices();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// =========================================================================
// ========================= Menu Bar Implementation =========================
// =========================================================================

void MainWindow::setupMenuConnections()
{
    // --- File Menu ---
    connect(ui->action_saveProfile, &QAction::triggered, this, &MainWindow::handleSaveProfileAction);
    connect(ui->action_loadProfile, &QAction::triggered, this, &MainWindow::handleLoadProfileAction);
    connect(ui->action_settings, &QAction::triggered, this, &MainWindow::handleSettingsAction);
    connect(ui->action_exit, &QAction::triggered, this, &MainWindow::handleExitAction);

    // --- Device Menu ---
    // action_refreshUSB is already connected in the constructor.
    connect(ui->action_connectAllUSB, &QAction::triggered, this, &MainWindow::handleConnectAllUsbAction);
    // Connect the "Disconnect All" menu item to the existing handler.
    connect(ui->action_disconnectAll, &QAction::triggered, this, &MainWindow::handleDisconnectAllClick);

    // --- View Menu ---
    connect(ui->action_toggleLeftPanel, &QAction::triggered, this, &MainWindow::handleToggleLeftPanel);
    connect(ui->action_toggleBottomPanel, &QAction::triggered, this, &MainWindow::handleToggleBottomPanel);
    connect(ui->action_appAlwaysOnTop, &QAction::triggered, this, &MainWindow::handleAppAlwaysOnTop);
    connect(ui->action_gridLayout, &QAction::triggered, this, &MainWindow::handleGridLayoutAction);

    // --- Help Menu ---
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::handleAboutAction);
    connect(ui->action_shortcuts, &QAction::triggered, this, &MainWindow::handleShortcutsAction);
    connect(ui->action_checkUpdates, &QAction::triggered, this, &MainWindow::handleCheckUpdatesAction);
}

// --- "File" Menu Slot Implementations ---

void MainWindow::handleSaveProfileAction()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Profile", QDir::currentPath(), "Configuration Files (*.ini)");
    if (!filePath.isEmpty()) {
        saveUiToSettings(filePath);
        onLogMessage(QString("Profile saved to: %1").arg(filePath));
    }
}

void MainWindow::handleLoadProfileAction()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load Profile", QDir::currentPath(), "Configuration Files (*.ini)");
    if (!filePath.isEmpty()) {
        loadUiFromSettings(filePath);
        onLogMessage(QString("Profile loaded from %1.").arg(filePath));
    }
}

void MainWindow::handleSettingsAction()
{
    QMessageBox::information(this, "Information", "Application settings feature is currently under development.");
}

void MainWindow::handleExitAction()
{
    this->close(); // Triggers closeEvent for graceful shutdown.
}

// --- "Device" Menu Slot Implementations ---

void MainWindow::handleConnectAllUsbAction()
{
    onLogMessage("Attempting to connect to all available USB devices...");
    bool foundDevice = false;
    for (int i = 0; i < ui->listWidget_usbDevices->count(); ++i) {
        QListWidgetItem* item = ui->listWidget_usbDevices->item(i);
        QString itemText = item->text();
        if (itemText.contains("(device)")) {
            foundDevice = true;
            QString serial = itemText.left(itemText.indexOf(' '));
            startDeviceWindow(serial);
        }
    }
    if (!foundDevice) {
        onLogMessage("No USB devices with status 'device' were found.");
    }
}

// --- "View" Menu Slot Implementations ---

void MainWindow::handleToggleLeftPanel(bool checked)
{
    ui->widget_leftPanel->setVisible(checked);
}

void MainWindow::handleToggleBottomPanel(bool checked)
{
    ui->bottomSplitter->setVisible(checked);
}

void MainWindow::handleAppAlwaysOnTop(bool checked)
{
    Qt::WindowFlags flags = this->windowFlags();
    if (checked) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    this->setWindowFlags(flags);
    this->show(); // Re-show the window to apply the flag changes.
    onLogMessage(checked ? "Main window is now always on top." : "Main window is no longer always on top.");
}

void MainWindow::handleGridLayoutAction()
{
    QMessageBox::information(this, "Information", "Grid layout feature is currently under development.");
}

// --- "Help" Menu Slot Implementations ---

void MainWindow::handleAboutAction()
{
    QMessageBox::about(this, "About Scrcpy Multi-Device Controller",
                       "<h3>Scrcpy Multi-Device Controller</h3>"
                       "<p>Version: 1.0 (Based on Scrcpy 3.3.3)</p>"
                       "<p>This is a graphical user interface tool built with Qt and Scrcpy, designed to "
                       "easily manage and control multiple Android devices.</p>");
}

void MainWindow::handleShortcutsAction()
{
    QMessageBox::information(this, "Common Scrcpy Shortcuts",
                             "<b>In-Window Shortcuts (Partial List):</b>"
                             "<ul>"
                             "<li><code>Ctrl+H</code>: Home key</li>"
                             "<li><code>Ctrl+B</code> or <code>Right Mouse Click</code>: Back key</li>"
                             "<li><code>Ctrl+S</code>: App switch key</li>"
                             "<li><code>Ctrl+M</code>: Menu key</li>"
                             "<li><code>Ctrl+P</code>: Power key (screen on/off)</li>"
                             "<li><code>Ctrl+O</code>: Turn device screen off (keep mirroring)</li>"
                             "<li><code>Ctrl+Shift+O</code>: Turn device screen on</li>"
                             "<li><code>Ctrl+Up/Down</code>: Volume up/down</li>"
                             "<li><code>Ctrl+R</code>: Rotate screen</li>"
                             "<li><code>Ctrl+N</code>: Expand notification panel</li>"
                             "<li><code>Ctrl+Shift+N</code>: Collapse notification panel</li>"
                             "<li><code>Ctrl+C</code>: Copy to device clipboard</li>"
                             "<li><code>Ctrl+Shift+V</code>: Paste device clipboard to computer</li>"
                             "<li><code>Ctrl+I</code>: Enable/disable FPS counter</li>"
                             "</ul>"
                             "<p>For more shortcuts, please refer to the official Scrcpy documentation.</p>");
}

void MainWindow::handleCheckUpdatesAction()
{
    QMessageBox::information(this, "Check for Updates", "You are using the latest version!<p>(This feature is a placeholder)</p>");
    // A real implementation could open a URL.
    // QDesktopServices::openUrl(QUrl("https://github.com/your/repo/releases"));
}

// --- Profile Save/Load Helper Functions ---

void MainWindow::saveUiToSettings(const QString& filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    settings.beginGroup("ScrcpyOptions");

    // Iterate through all child widgets of the settings toolBox and save their state.
    for (auto* widget : ui->toolBox_settings->findChildren<QWidget*>()) {
        QString name = widget->objectName();
        if (auto* box = qobject_cast<QComboBox*>(widget)) {
            settings.setValue(name, box->currentIndex());
        } else if (auto* box = qobject_cast<QSpinBox*>(widget)) {
            settings.setValue(name, box->value());
        } else if (auto* box = qobject_cast<QCheckBox*>(widget)) {
            settings.setValue(name, box->isChecked());
        } else if (auto* line = qobject_cast<QLineEdit*>(widget)) {
            settings.setValue(name, line->text());
        }
    }
    settings.endGroup();
}

void MainWindow::loadUiFromSettings(const QString& filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    settings.beginGroup("ScrcpyOptions");

    // Iterate through widgets and load their state from the settings file.
    for (auto* widget : ui->toolBox_settings->findChildren<QWidget*>()) {
        QString name = widget->objectName();
        if (!settings.contains(name)) continue;

        if (auto* box = qobject_cast<QComboBox*>(widget)) {
            box->setCurrentIndex(settings.value(name).toInt());
        } else if (auto* box = qobject_cast<QSpinBox*>(widget)) {
            box->setValue(settings.value(name).toInt());
        } else if (auto* box = qobject_cast<QCheckBox*>(widget)) {
            box->setChecked(settings.value(name).toBool());
        } else if (auto* line = qobject_cast<QLineEdit*>(widget)) {
            line->setText(settings.value(name).toString());
        }
    }
    settings.endGroup();
}

// ====================================================================
// ==================== Original Project Code =========================
// ====================================================================

void MainWindow::onDevicesUpdated(const QList<DeviceInfo> &devices)
{
    ui->listWidget_usbDevices->clear();
    ui->listWidget_wifiDevices->clear();

    for (const DeviceInfo &device : devices) {
        QString itemText = QString("%1 (%2)").arg(device.serial, device.status);
        if (device.serial.contains(':')) { // WiFi devices typically have ':' in their serial.
            QListWidgetItem* item = new QListWidgetItem(itemText, ui->listWidget_wifiDevices);
            if (device.status != "device") {
                item->setForeground(Qt::gray);
            }
        } else { // USB devices.
            QListWidgetItem* item = new QListWidgetItem(itemText, ui->listWidget_usbDevices);
            if (device.status == "unauthorized") {
                item->setForeground(Qt::red);
                item->setToolTip("Device is unauthorized. Please confirm the USB debugging prompt on your phone.");
            } else if (device.status != "device") {
                item->setForeground(Qt::gray);
            }
        }
    }

    if (ui->listWidget_usbDevices->count() == 0) {
        ui->listWidget_usbDevices->addItem("No USB devices found");
    }
    if (ui->listWidget_wifiDevices->count() == 0) {
        ui->listWidget_wifiDevices->addItem("No WiFi devices found");
    }
}

void MainWindow::onLogMessage(const QString &message)
{
    QString currentTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEdit_logs->appendPlainText(QString("[%1] %2").arg(currentTime, message));
}

void MainWindow::handleClearLogsClick()
{
    ui->textEdit_logs->clear();
}

void MainWindow::handleConnectUsbClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("Info: Please select a USB device from the list to connect.");
        return;
    }
    for (QListWidgetItem *item : selectedItems) {
        QString itemText = item->text();
        QString serial = itemText.left(itemText.indexOf(' '));
        if (!itemText.contains("(device)")) {
            onLogMessage(QString("Warning: Device %1 is not in 'device' state. Cannot connect.").arg(serial));
            continue;
        }
        startDeviceWindow(serial);
    }
}

void MainWindow::onDeviceWindowClosed(const QString &serial)
{
    onLogMessage(QString("Window for device %1 has been closed.").arg(serial));
    delete mDeviceWindows.take(serial); // Take the pointer and delete the object.
    mUiStateManager->removeDeviceFromStatusTable(serial);
    mUiStateManager->updateConnectedDeviceStatus();
}

ScrcpyOptions MainWindow::gatherScrcpyOptions() const
{
    ScrcpyOptions opts;
    // --- Video Options ---
    opts.video_source = ui->comboBox_videoSource->currentData().toString();
    opts.max_size = ui->spinBox_maxSize->value();
    opts.video_bit_rate = ui->spinBox_videoBitrate->value() * 1000000; // M to bps
    opts.max_fps = ui->spinBox_maxFPS->value();
    opts.video_codec = ui->comboBox_videoCodec->currentData().toString();
    opts.display_id = ui->spinBox_displayID->value();
    opts.video = !ui->checkBox_noVideo->isChecked();
    opts.no_video_playback = ui->checkBox_noVideoPlayback->isChecked();
    // --- Audio Options ---
    opts.audio_source = ui->comboBox_audioSource->currentData().toString();
    opts.audio_bit_rate = ui->spinBox_audioBitrate->value() * 1000; // K to bps
    opts.audio_codec = ui->comboBox_audioCodec->currentData().toString();
    opts.audio_buffer = ui->spinBox_audioBuffer->value();
    opts.audio_dup = ui->checkBox_audioDup->isChecked();
    opts.audio = !ui->checkBox_noAudio->isChecked();
    opts.require_audio = ui->checkBox_requireAudio->isChecked();
    // --- Camera Options ---
    opts.camera_id = ui->comboBox_cameraID->currentText();
    opts.camera_facing = ui->comboBox_cameraFacing->currentData().toString();
    opts.camera_size = ui->comboBox_cameraSize->currentText();
    opts.camera_fps = ui->spinBox_cameraFPS->value();
    opts.camera_high_speed = ui->checkBox_cameraHighSpeed->isChecked();
    // --- Control Options ---
    opts.control = !ui->checkBox_noControl->isChecked();
    opts.stay_awake = ui->checkBox_stayAwake->isChecked();
    opts.power_off_on_close = ui->checkBox_powerOffOnClose->isChecked();
    opts.show_touches = ui->checkBox_showTouches->isChecked();
    opts.clipboard_autosync = !ui->checkBox_noClipboardAutosync->isChecked();
    opts.keyboard_mode = ui->comboBox_keyboardMode->currentData().toString();
    opts.mouse_mode = ui->comboBox_mouseMode->currentData().toString();
    opts.otg = ui->checkBox_otg->isChecked();
    // --- Window Options ---
    opts.fullscreen = ui->checkBox_fullscreen->isChecked();
    opts.always_on_top = ui->checkBox_alwaysOnTop->isChecked();
    opts.window_borderless = ui->checkBox_windowBorderless->isChecked();
    opts.window_title = ui->lineEdit_windowTitle->text();
    // --- Recording Options ---
    opts.record_file = ui->lineEdit_recordFile->text();
    opts.record_format = ui->comboBox_recordFormat->currentData().toString();
    opts.no_playback = ui->checkBox_noPlayback->isChecked();
    return opts;
}

bool MainWindow::validateOptions(const ScrcpyOptions &options)
{
    if (!options.record_file.isEmpty()) {
        QFileInfo fileInfo(options.record_file);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            QMessageBox::warning(this, "Configuration Error", QString("The specified recording file directory does not exist!\nPlease check the path: %1").arg(dir.path()));
            return false;
        }
    }
    if (options.video_source == "camera") {
        if (options.camera_id.isEmpty() && options.camera_facing == "any") {
            QMessageBox::warning(this, "Configuration Error", "You have selected the camera as the video source, but have not specified a camera ID or facing direction (front/back).");
            return false;
        }
    }
    if (!options.video && !options.audio) {
        auto reply = QMessageBox::question(this, "Confirm Action", "You have disabled both video and audio. This means nothing will be displayed or played after connecting (control-only mode).\nDo you want to continue?", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return false;
        }
    }
    return true;
}

void MainWindow::handleSaveRecordAsClick()
{
    QString defaultPath = QDir::homePath();
    QString fileName = QString("scrcpy_record_%1.mkv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "Select Recording File Location", QDir(defaultPath).filePath(fileName), "Matroska Video (*.mkv);;MP4 Video (*.mp4)");

    if (!filePath.isEmpty()) {
        ui->lineEdit_recordFile->setText(filePath);
        // Automatically select the correct format in the dropdown.
        if (filePath.endsWith(".mp4", Qt::CaseInsensitive)) {
            ui->comboBox_recordFormat->setCurrentText("mp4");
        } else {
            ui->comboBox_recordFormat->setCurrentText("mkv");
        }
    }
}

void MainWindow::startDeviceWindow(const QString &serial)
{
    if (mDeviceWindows.contains(serial)) {
        onLogMessage(QString("Info: The window for device %1 is already open.").arg(serial));
        mDeviceWindows[serial]->activateWindow();
        mDeviceWindows[serial]->raise();
        return;
    }

    ScrcpyOptions options = gatherScrcpyOptions();
    if (!validateOptions(options)) {
        onLogMessage("Error: Configuration validation failed. Connection cancelled.");
        return;
    }

    onLogMessage(QString("Creating window for device %1...").arg(serial));
    DeviceWindow *deviceWindow = new DeviceWindow(serial, options, nullptr);
    connect(deviceWindow, &DeviceWindow::windowClosed, this, &MainWindow::onDeviceWindowClosed);
    connect(deviceWindow, &DeviceWindow::statusUpdated, mUiStateManager, &UiStateManager::updateDeviceStatusInfo);

    mDeviceWindows.insert(serial, deviceWindow);
    mUiStateManager->addDeviceToStatusTable(serial);
    mUiStateManager->updateConnectedDeviceStatus();
    deviceWindow->show();
}

void MainWindow::handleEnableTcpIpClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_usbDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("Warning: Please select a USB device to enable TCP/IP mode.");
        QMessageBox::warning(this, "Operation Failed", "Please select a device from the USB device list first.");
        return;
    }
    QString serial = selectedItems.first()->text().left(selectedItems.first()->text().indexOf(' '));
    onLogMessage(QString("Enabling TCP/IP mode for device %1 (port 5555)...").arg(serial));
    AdbProcess *process = new AdbProcess(this);

    connect(process, &AdbProcess::finished, this, [this, process, serial](int exitCode, QProcess::ExitStatus exitStatus) {
        QString output = process->readAllStandardOutput() + process->readAllStandardError();
        if (exitStatus == QProcess::NormalExit && output.contains("restarting in TCP mode port: 5555")) {
            onLogMessage(QString("Success: TCP/IP mode enabled on port 5555 for device %1.").arg(serial));
            QMessageBox::information(this, "Success", "TCP/IP mode has been started on port 5555.\nPlease find your device's IP address and use the WiFi connection tab to connect.");
        } else {
            onLogMessage(QString("Failure: Could not enable TCP/IP mode for %1. Error: %2").arg(serial, output.trimmed()));
            QMessageBox::critical(this, "Failure", "Failed to start TCP/IP mode!\n" + output);
        }
        process->deleteLater();
    });

    process->execute(serial, {"tcpip", "5555"});
}

void MainWindow::handleConnectWifiClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_wifiDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        // If no device is selected in the list, try connecting via IP address input.
        QString ip = ui->lineEdit_ipAddress->text().trimmed();
        if (ip.isEmpty()) {
            onLogMessage("Info: Please select a device from the WiFi list or enter an IP address to connect.");
            return;
        }
        QString port = QString::number(ui->spinBox_tcpPort->value());
        QString fullAddress = ip.contains(':') ? ip : (ip + ":" + port);

        onLogMessage(QString("Attempting to connect via 'adb connect' to %1...").arg(fullAddress));
        AdbProcess *process = new AdbProcess(this);
        connect(process, &AdbProcess::finished, this, [this, process, fullAddress](int, QProcess::ExitStatus exitStatus){
            QString output = process->readAllStandardOutput() + process->readAllStandardError();
            if (exitStatus == QProcess::NormalExit && (output.contains("connected to") || output.contains("already connected"))) {
                onLogMessage(QString("Successfully connected to %1").arg(fullAddress));
                QMessageBox::information(this, "Success", "Wireless connection successful or already established!");
            } else {
                onLogMessage(QString("Failed to connect to %1: %2").arg(fullAddress, output.trimmed()));
                QMessageBox::critical(this, "Failure", "Wireless connection failed!\n" + output);
            }
            mDeviceManager->refreshDevices();
            process->deleteLater();
        });
        process->execute("", {"connect", fullAddress});
    } else {
        // If devices are selected in the list, launch scrcpy windows for them.
        for (QListWidgetItem *item : selectedItems) {
            QString serial = item->text().left(item->text().indexOf(' '));
            startDeviceWindow(serial);
        }
    }
}

void MainWindow::handleRemoveWifiClick()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_wifiDevices->selectedItems();
    if (selectedItems.isEmpty()) {
        onLogMessage("Info: Please select a device from the WiFi list to disconnect.");
        return;
    }
    for (QListWidgetItem* item : selectedItems) {
        QString serial = item->text().left(item->text().indexOf(' '));
        onLogMessage(QString("Disconnecting device %1...").arg(serial));
        AdbProcess *process = new AdbProcess(this);

        connect(process, &AdbProcess::finished, this, [this, process](){
            mDeviceManager->refreshDevices();
            process->deleteLater();
        });

        process->execute("", {"disconnect", serial});
    }
}

void MainWindow::handleConnectSerialClick()
{
    QString serial = ui->lineEdit_serial->text().trimmed();
    if (serial.isEmpty()) {
        onLogMessage("Warning: Please enter a device serial number to connect.");
        QMessageBox::warning(this, "Input Error", "Please enter a valid device serial number.");
        return;
    }
    startDeviceWindow(serial);
}

void MainWindow::handleDisconnectAllClick()
{
    onLogMessage("Executing 'adb disconnect'...");
    AdbProcess* process = new AdbProcess(this);

    connect(process, &AdbProcess::finished, this, [this, process](){
        mDeviceManager->refreshDevices();
        process->deleteLater();
    });

    process->execute("", {"disconnect"});
}

void MainWindow::handleKillAdbClick()
{
    onLogMessage("Restarting ADB service (kill-server & start-server)...");
    AdbProcess *killProcess = new AdbProcess(this);
    connect(killProcess, &AdbProcess::finished, this, [this](int, QProcess::ExitStatus){
        onLogMessage("ADB service stopped. Starting...");
        AdbProcess* startProcess = new AdbProcess(this);
        connect(startProcess, &AdbProcess::finished, this, [this, startProcess](){
            onLogMessage("ADB service started. Refreshing device list...");
            mDeviceManager->refreshDevices();
            startProcess->deleteLater();
        });
        startProcess->execute("", {"start-server"});
    });
    connect(killProcess, &AdbProcess::finished, killProcess, &AdbProcess::deleteLater);
    killProcess->execute("", {"kill-server"});
}

