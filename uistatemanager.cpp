#include "uistatemanager.h"
#include "ui_mainwindow.h" // 必须包含这个头文件来访问 ui 成员

UiStateManager::UiStateManager(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent), m_ui(ui)
{
    // 将所有会影响其他控件状态的“源”控件的信号，全部连接到总更新槽
    connect(m_ui->checkBox_noVideo, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->comboBox_videoSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UiStateManager::updateAllControlStates);
    connect(m_ui->checkBox_noAudio, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->comboBox_audioSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UiStateManager::updateAllControlStates);
    connect(m_ui->checkBox_otg, &QCheckBox::toggled, this, &UiStateManager::updateAllControlStates);
    connect(m_ui->lineEdit_recordFile, &QLineEdit::textChanged, this, &UiStateManager::updateAllControlStates);
}

void UiStateManager::initializeStates()
{
    updateAllControlStates();
    updateConnectedDeviceStatus();

    // 初始化状态表格的表头
    m_ui->tableWidget_deviceStatus->setColumnCount(5);
    m_ui->tableWidget_deviceStatus->setHorizontalHeaderLabels({"名称/序列号", "连接方式", "状态", "分辨率", "设备名"});
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
    // 根据你的 UI 文件，状态栏里的 QLabel 叫 label_statusConnected
    m_ui->statusbar->findChild<QLabel*>("label_statusConnected")->setText(QString("已连接设备: %1").arg(count));

    // 根据你的 UI 文件，底部的 TabWidget 叫 tabWidget_bottom，状态页是第2页 (索引为1)
    m_ui->tabWidget_bottom->setTabEnabled(1, count > 0);

    // 如果没有设备连接，切换回欢迎页
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

    m_ui->tableWidget_deviceStatus->setItem(rowCount, 0, new QTableWidgetItem(serial));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 1, new QTableWidgetItem("USB")); // 暂定为USB
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 2, new QTableWidgetItem("连接中..."));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 3, new QTableWidgetItem("N/A"));
    m_ui->tableWidget_deviceStatus->setItem(rowCount, 4, new QTableWidgetItem("N/A"));
}

void UiStateManager::removeDeviceFromStatusTable(const QString &serial)
{
    for (int i = 0; i < m_ui->tableWidget_deviceStatus->rowCount(); ++i) {
        if (m_ui->tableWidget_deviceStatus->item(i, 0)->text() == serial) {
            m_ui->tableWidget_deviceStatus->removeRow(i);
            break;
        }
    }
}

void UiStateManager::updateDeviceStatusInfo(const QString &serial, const QString &deviceName, const QSize &frameSize)
{
    for (int i = 0; i < m_ui->tableWidget_deviceStatus->rowCount(); ++i) {
        if (m_ui->tableWidget_deviceStatus->item(i, 0)->text() == serial) {
            m_ui->tableWidget_deviceStatus->item(i, 2)->setText("已连接");
            m_ui->tableWidget_deviceStatus->item(i, 3)->setText(QString("%1x%2").arg(frameSize.width()).arg(frameSize.height()));
            m_ui->tableWidget_deviceStatus->item(i, 4)->setText(deviceName);
            break;
        }
    }
}


// --- 以下是具体的UI联动逻辑，从 MainWindow 迁移而来 ---

void UiStateManager::updateVideoControlsState()
{
    const bool videoDisabled = m_ui->checkBox_noVideo->isChecked();
    const QString videoSource = m_ui->comboBox_videoSource->currentData().toString();

    for (QWidget* widget : m_ui->groupBox_videoSettings->findChildren<QWidget*>()) {
        if (widget != m_ui->checkBox_noVideo) {
            widget->setEnabled(!videoDisabled);
        }
    }
    m_ui->comboBox_videoSource->setEnabled(!videoDisabled);

    const bool enableCameraSettings = !videoDisabled && (videoSource == "camera");
    m_ui->groupBox_cameraSettings->setEnabled(enableCameraSettings);
}

void UiStateManager::updateAudioControlsState()
{
    const bool audioDisabled = m_ui->checkBox_noAudio->isChecked();
    m_ui->groupBox_audioSettings->setEnabled(!audioDisabled);

    const QString audioSource = m_ui->comboBox_audioSource->currentData().toString();
    const bool enableAudioDup = !audioDisabled && (audioSource == "playback");
    m_ui->checkBox_audioDup->setEnabled(enableAudioDup);
    if (!enableAudioDup) {
        m_ui->checkBox_audioDup->setChecked(false);
    }
}

void UiStateManager::updateControlControlsState()
{
    const bool otgEnabled = m_ui->checkBox_otg->isChecked();
    m_ui->checkBox_noControl->setEnabled(!otgEnabled);
    m_ui->checkBox_stayAwake->setEnabled(!otgEnabled);
    m_ui->checkBox_turnScreenOff->setEnabled(!otgEnabled);
    m_ui->checkBox_powerOffOnClose->setEnabled(!otgEnabled);
    m_ui->checkBox_showTouches->setEnabled(!otgEnabled);
    m_ui->comboBox_keyboardMode->setEnabled(!otgEnabled);
    m_ui->comboBox_mouseMode->setEnabled(!otgEnabled);
}

void UiStateManager::updateRecordingControlsState()
{
    const bool recordFileIsEmpty = m_ui->lineEdit_recordFile->text().isEmpty();
    m_ui->comboBox_recordFormat->setEnabled(!recordFileIsEmpty);
}
