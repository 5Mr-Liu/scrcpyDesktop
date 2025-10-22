#include "uistatemanager.h"
#include "ui_mainwindow.h" // Must include this header to access ui members.
#include <QLabel> // Required for findChild<QLabel*>

UiStateManager::UiStateManager(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), m_ui(ui)
{
    // Connect signals from all "source" controls that affect the state of other controls
    // to the main update slot.
    connect(m_ui->checkBox_noVideo, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->comboBox_videoSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UiStateManager::updateAllControlStates);
    connect(m_ui->checkBox_noAudio, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->comboBox_audioSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UiStateManager::updateAllControlStates);
    connect(m_ui->checkBox_otg, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->lineEdit_recordFile, &QLineEdit::textChanged, this, &UiStateManager::updateAllControlStates);
}

void UiStateManager::initializeStates()
{
    // Perform an initial update to set all controls to their correct default states.
    updateAllControlStates();
    updateConnectedDeviceStatus();

    // Initialize the headers for the device status table.
    m_ui->tableWidget_deviceStatus->setColumnCount(5);
    m_ui->tableWidget_deviceStatus->setHorizontalHeaderLabels({"Serial/ID", "Connection", "Status", "Resolution", "Device Name"});
    // Make the first and last columns stretch to fill available space.
    m_ui->tableWidget_deviceStatus->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->tableWidget_deviceStatus->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
}

void UiStateManager::setDeviceWindowsMap(const QMap<QString, DeviceWindow*> *deviceWindows)
{
    m_deviceWindows = deviceWindows;
}

void UiStateManager::updateAllControlStates()
{
    updateVideoControlsState();
    updateAudioControlsState();
    updateControlControlsState();
    updateRecordingControlsState();
}

void UiStateManager::updateConnectedDeviceStatus()
{
    if (!m_deviceWindows) return;

    int count = m_deviceWindows->count();
    // Find the status label in the status bar by object name and update its text.
    QLabel* statusLabel = m_ui->statusbar->findChild<QLabel*>("label_statusConnected");
    if (statusLabel) {
        statusLabel->setText(QString("Connected Devices: %1").arg(count));
    }


    // The "Status" tab is the second tab (index 1).
    m_ui->tabWidget_bottom->setTabEnabled(1, count > 0);

    // If no devices are connected, switch the central widget to the welcome page.
    if (count == 0) {
        m_ui->stackedWidget_devices->setCurrentWidget(m_ui->page_welcome);
    } else {
        m_ui->stackedWidget_devices->setCurrentWidget(m_ui->page_devicesGrid);
    }
}

void UiStateManager::addDeviceToStatusTable(const QString &serial)
{
    int rowCount = m_ui->tableWidget_deviceStatus->rowCount();
    m_ui->tableWidget_deviceStatus->insertRow(rowCount);

    // Populate the new row with initial "connecting" information.
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 0, new QTableWidgetItem(serial));
    // Determine connection type based on serial format.
    QString connType = serial.contains(':') ? "Wi-Fi" : "USB";
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 1, new QTableWidgetItem(connType));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 2, new QTableWidgetItem("Connecting..."));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 3, new QTableWidgetItem("N/A"));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 4, new QTableWidgetItem("N/A"));
}

void UiStateManager::removeDeviceFromStatusTable(const QString &serial)
{
    // Find the row with the matching serial number and remove it.
    for (int i = 0; i < m_ui->tableWidget_deviceStatus->rowCount(); ++i) {
        QTableWidgetItem* item = m_ui->tableWidget_deviceStatus->item(i, 0);
        if (item && item->text() == serial) {
            m_ui->tableWidget_deviceStatus->removeRow(i);
            break; // Exit the loop once the item is found and removed.
        }
    }
}

void UiStateManager::updateDeviceStatusInfo(const QString &serial, const QString &deviceName, const QSize &frameSize)
{
    // Find the row for the specified device and update its status, resolution, and name.
    for (int i = 0; i < m_ui->tableWidget_deviceStatus->rowCount(); ++i) {
        QTableWidgetItem* item = m_ui->tableWidget_deviceStatus->item(i, 0);
        if (item && item->text() == serial) {
            m_ui->tableWidget_deviceStatus->item(i, 2)->setText("Connected");
            m_ui->tableWidget_deviceStatus->item(i, 3)->setText(QString("%1x%2").arg(frameSize.width()).arg(frameSize.height()));
            m_ui->tableWidget_deviceStatus->item(i, 4)->setText(deviceName);
            break; // Exit the loop once the update is complete.
        }
    }
}

// --- Private Implementation of Specific UI State Logic ---

void UiStateManager::updateVideoControlsState()
{
    const bool videoDisabled = m_ui->checkBox_noVideo->isChecked();
    const QString videoSource = m_ui->comboBox_videoSource->currentData().toString();

    // Enable/disable all widgets within the video settings group box, except the main checkbox itself.
    for (QWidget* widget : m_ui->groupBox_videoSettings->findChildren<QWidget*>()) {
        if (widget != m_ui->checkBox_noVideo) {
            widget->setEnabled(!videoDisabled);
        }
    }
    m_ui->comboBox_videoSource->setEnabled(!videoDisabled);

    // Camera settings are only enabled if video is on AND the source is set to "camera".
    const bool enableCameraSettings = !videoDisabled && (videoSource == "camera");
    m_ui->groupBox_cameraSettings->setEnabled(enableCameraSettings);
}

void UiStateManager::updateAudioControlsState()
{
    const bool audioDisabled = m_ui->checkBox_noAudio->isChecked();
    m_ui->groupBox_audioSettings->setEnabled(!audioDisabled);

    const QString audioSource = m_ui->comboBox_audioSource->currentData().toString();
    // The "audio dup" option is only relevant for the "playback" audio source.
    const bool enableAudioDup = !audioDisabled && (audioSource == "playback");
    m_ui->checkBox_audioDup->setEnabled(enableAudioDup);
    if (!enableAudioDup) {
        // If the option is disabled, ensure it is also unchecked.
        m_ui->checkBox_audioDup->setChecked(false);
    }
}

void UiStateManager::updateControlControlsState()
{
    // When OTG mode is enabled, many standard control options become irrelevant and should be disabled.
    const bool otgEnabled = m_ui->checkBox_otg->isChecked();
    m_ui->checkBox_noControl->setEnabled(!otgEnabled);
    m_ui->checkBox_stayAwake->setEnabled(!otgEnabled);
    m_ui->checkBox_powerOffOnClose->setEnabled(!otgEnabled);
    m_ui->checkBox_showTouches->setEnabled(!otgEnabled);
    m_ui->comboBox_keyboardMode->setEnabled(!otgEnabled);
    m_ui->comboBox_mouseMode->setEnabled(!otgEnabled);
}

void UiStateManager::updateRecordingControlsState()
{
    // The recording format can only be selected if a recording file path is specified.
    const bool recordFileIsEmpty = m_ui->lineEdit_recordFile->text().isEmpty();
    m_ui->comboBox_recordFormat->setEnabled(!recordFileIsEmpty);
}


