#include "devicewindow.h"
#include "ui_devicewindow.h"
#include "videodecoderthread.h"
#include <QCloseEvent>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <QMouseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDateTime>
#include "androidkeycodes.h"

const int BASE_HEIGHT_PORTRAIT = 800;  // 竖屏时，我们希望窗口内容区的高度
const int BASE_WIDTH_LANDSCAPE = 960; // 横屏时，我们希望窗口内容区的宽度

DeviceWindow::DeviceWindow(const QString &serial,const ScrcpyOptions &options, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceWindow),
    mSerial(serial),
    mOptions(options),
    mLocalPort(27183),
    mServerProcess(nullptr),
    mVideoSocket(nullptr),
    mDecoder(nullptr),
    mControlSender(nullptr),
    mConnectionRetries(0),
    mCurrentFrameSize(0,0)
{
    ui->setupUi(this);
    // --- 1. 应用客户端窗口设置 ---
    // 设置窗口标题
    if (!mOptions.window_title.isEmpty()) {
        setWindowTitle(mOptions.window_title);
    } else {
        setWindowTitle(QString("设备 - %1").arg(mSerial));
    }
    // 设置窗口标志 (置顶、无边框)
    Qt::WindowFlags flags = windowFlags();
    if (mOptions.always_on_top) {
        flags |= Qt::WindowStaysOnTopHint;
    }
    if (mOptions.window_borderless) {
        flags |= Qt::FramelessWindowHint;
    }
    setWindowFlags(flags);
    // --- 2. 初始化 UI 和组件 ---
    ui->label_videoStream->setStyleSheet("color: white; background-color: black;");
    ui->label_videoStream->setAlignment(Qt::AlignCenter);
    ui->label_videoStream->setText("正在连接设备...");
    // ▲▲▲ 【问题修复】删除 mOptions.control = true; 这一行硬编码 ▲▲▲
    // 现在 mOptions.control 的值将由 MainWindow 的 UI 决定
    setupToolbarActions();
    this->setFocusPolicy(Qt::StrongFocus);
    // 延迟启动流，给窗口初始化留出时间
    QTimer::singleShot(100, this, &DeviceWindow::startStreaming);
    // --- 3. 处理全屏 ---
    // 全屏必须在 show() 之后调用，所以使用延迟定时器
    if (mOptions.fullscreen) {
        QTimer::singleShot(250, this, [this](){
            if (this) { // 检查以防窗口已被关闭
                this->showFullScreen();
            }
        });
    }
}

DeviceWindow::~DeviceWindow()
{
    qDebug() << "DeviceWindow for" << mSerial << "is being destroyed.";
    stopAll();
    delete ui;
}

QString DeviceWindow::getSerial() const
{
    return mSerial;
}

void DeviceWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "Closing window for" << mSerial;
    if (!mOptions.record_file.isEmpty()) {
        // 存在录制任务，执行 adb pull
        QString format = (mOptions.record_format == "auto" ? "mkv" : mOptions.record_format);

        // 与 ScrcpyOptions 中构造的设备路径保持一致
        QString device_path = QString("/sdcard/%1.%2").arg("temp_record_file").arg(format);
        QString pc_path = mOptions.record_file;
        qDebug() << "Pulling record file from" << device_path << "to" << pc_path;
        // 创建一个 adb process 来拉取文件
        AdbProcess *pullProcess = new AdbProcess(); // 不指定父对象，让它独立运行
        connect(pullProcess, &AdbProcess::finished, this, [this, pullProcess, device_path, pc_path](int exitCode, QProcess::ExitStatus status){
            if (status == QProcess::NormalExit && exitCode == 0) {
                qInfo() << "Record file pulled successfully to" << pc_path;
                QMessageBox::information(nullptr, "录制成功", QString("录制文件已成功保存到：\n%1").arg(QDir::toNativeSeparators(pc_path)));
                // 拉取成功后，删除设备上的临时文件
                AdbProcess *rmProcess = new AdbProcess();
                connect(rmProcess, &AdbProcess::finished, rmProcess, &QObject::deleteLater);
                rmProcess->execute(mSerial, {"shell", "rm", device_path});
            } else {
                qWarning() << "Failed to pull record file:" << pullProcess->getOutput();
                QMessageBox::warning(nullptr, "录制失败", "无法从设备拉取录制文件。\n" + pullProcess->getOutput());
            }
            pullProcess->deleteLater();
        });
        pullProcess->execute(mSerial, {"pull", device_path, pc_path});
    }
    stopAll();
    emit windowClosed(mSerial);
    event->accept();
}

void DeviceWindow::startStreaming()
{
    ui->label_videoStream->setText("Step 1: Pushing server...");
    qDebug() << "Step 1: Pushing server file to device...";
    pushServer();
}

void DeviceWindow::pushServer()
{
    // !! 注意: 确保这里的文件名和版本号与你的 ScrcpyOptions 中的版本号一致
    QString serverFileName = QString("scrcpy-server-v%1").arg(mOptions.version);
    QString serverLocalPath = QDir(QCoreApplication::applicationDirPath()).filePath(serverFileName);

    if (!QFile::exists(serverLocalPath)) {
        QString errorMsg = QString("找不到 scrcpy-server 文件！\n路径: %1").arg(serverLocalPath);
        qCritical() << "FATAL:" << errorMsg;
        QMessageBox::critical(this, "严重错误", errorMsg);
        ui->label_videoStream->setText("错误: 找不到 server 文件！");
        close();
        return;
    }
    const QString serverRemotePath = "/data/local/tmp/scrcpy-server.jar";
    AdbProcess *pushProcess = new AdbProcess(this);
    connect(pushProcess, &AdbProcess::finished, this, &DeviceWindow::onPushServerFinished);
    pushProcess->execute(mSerial, {"push", serverLocalPath, serverRemotePath});
}

void DeviceWindow::onPushServerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto process = qobject_cast<AdbProcess*>(sender());
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "Server pushed successfully.";
        ui->label_videoStream->setText("Step 2: Forwarding port...");
        forwardPort();
    } else {
        qCritical() << "Push server failed:" << process->getOutput();
        QMessageBox::critical(this, "错误", "推送scrcpy-server文件失败！\n" + process->getOutput());
        close();
    }
    process->deleteLater();
}

void DeviceWindow::forwardPort()
{
    // 注意：你的 ScrcpyOptions 默认 tunnel_forward=true，所以这里的 localabstract 必须是 scrcpy
    QString forwardRule = QString("tcp:%1").arg(mLocalPort);
    AdbProcess *forwardProcess = new AdbProcess(this);
    connect(forwardProcess, &AdbProcess::finished, this, &DeviceWindow::onForwardPortFinished);
    forwardProcess->execute(mSerial, {"forward", forwardRule, "localabstract:scrcpy"});
}

// 【关键修改】修复竞态条件的核心逻辑
void DeviceWindow::onForwardPortFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto process = qobject_cast<AdbProcess*>(sender());
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "Port forwarded successfully.";
        ui->label_videoStream->setText("Step 3: Starting server...");
        startServer();
        qDebug() << "Step 4: Preparing to connect video socket with retries...";
        ui->label_videoStream->setText("Step 4: Connecting video stream...");
        mConnectionRetries = 0; // 重置计数器
        // 给予服务器 500ms 的初始启动时间，然后开始连接【视频】Socket
        QTimer::singleShot(500, this, &DeviceWindow::connectToSocketWithRetry);
    } else {
        qCritical() << "Forward port failed:" << process->getOutput();
        QMessageBox::critical(this, "错误", "端口转发失败！\n" + process->getOutput());
        close();
    }
    process->deleteLater();
}

void DeviceWindow::startServer()
{
    if (mServerProcess) {
        mServerProcess->deleteLater();
    }
    mServerProcess = new AdbProcess(this);
    connect(mServerProcess, &AdbProcess::finished, [this](int, QProcess::ExitStatus){
        qDebug() << "ADB shell process finished (server likely stopped on device).";
        if (this->isVisible()) {
            ui->label_videoStream->setText("连接已断开");
        }
    });

    // 从你的 ScrcpyOptions 生成参数
    QStringList args = mOptions.toAdbShellArgs();
    qDebug() << "Starting server with args:" << args.join(" ");
    mServerProcess->execute(mSerial, args);
}

// 【新增】带重试的连接函数
void DeviceWindow::connectToSocketWithRetry()
{
    if (mConnectionRetries >= 15) { // 最多重试15次 (约3秒)
        qCritical() << "Failed to connect to socket after all retries.";
        QMessageBox::critical(this, "连接失败", "无法连接到设备上的 scrcpy 服务。\n请检查设备是否已解锁或有权限弹窗。");
        close();
        return;
    }
    mConnectionRetries++;
    qDebug() << "Connection attempt #" << mConnectionRetries;

    if (!mDecoder) {
        mDecoder = new VideoDecoderThread(mOptions.video_codec,this);
        connect(mDecoder, &VideoDecoderThread::frameDecoded, this, &DeviceWindow::onFrameDecoded);
        connect(mDecoder, &VideoDecoderThread::decodingFinished, this, &DeviceWindow::onDecodingFinished);
        connect(mDecoder, &VideoDecoderThread::deviceNameReady, this, [this](const QString &name){
            mDeviceName = name;
            if (mOptions.window_title.isEmpty()) {
                // 如果没有自定义标题，才使用默认的 "设备名 - 序列号" 格式
                setWindowTitle(QString("%1 - %2").arg(name).arg(mSerial));
            }
            if(!mCurrentFrameSize.isEmpty()){
                emit statusUpdated(mSerial,mDeviceName,mCurrentFrameSize);
            }
        });
        mDecoder->start();
    }

    if (!mVideoSocket) {
        mVideoSocket = new QTcpSocket(this);
        connect(mVideoSocket, &QTcpSocket::connected, this, &DeviceWindow::onSocketConnected);
        connect(mVideoSocket, &QTcpSocket::disconnected, this, &DeviceWindow::onSocketDisconnected);
        connect(mVideoSocket, &QTcpSocket::readyRead, this, &DeviceWindow::onSocketReadyRead);

        // 【关键】当连接出错时，延迟后重试
        connect(mVideoSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            qWarning() << "Socket connection failed:" << mVideoSocket->errorString() << ". Retrying...";
            mVideoSocket->deleteLater();
            mVideoSocket = nullptr;
            QTimer::singleShot(200, this, &DeviceWindow::connectToSocketWithRetry);
        });
    }
    mVideoSocket->connectToHost("localhost", mLocalPort);
}

void DeviceWindow::onSocketConnected()
{
    qDebug() << "Video socket connected! Handing over to decoder thread...";
    ui->label_videoStream->setText("连接成功，等待设备元数据...");
    mConnectionRetries = 0; // 成功后重置
    // ============= 新增的核心逻辑 ==============
    // 视频 Socket 成功后，立刻连接控制 Socket
    qDebug() << "Now connecting control socket...";
    if (!mControlSender) {
        mControlSender = new ControlSender(this);
        connect(mControlSender, &ControlSender::controlSocketConnected, this, [](){
            qDebug() << "[Control] Control socket connected successfully!";
        });
        connect(mControlSender, &ControlSender::controlSocketDisconnected, this, [](){
            qDebug() << "[Control] Control socket disconnected.";
        });
    }
    mControlSender->connectToServer("localhost", mLocalPort);
    // ==========================================
}

void DeviceWindow::onSocketReadyRead()
{
    if (mVideoSocket && mDecoder && mDecoder->isRunning()) {
        QByteArray data = mVideoSocket->readAll();
        if (!data.isEmpty()) {
            QMetaObject::invokeMethod(mDecoder, "decodeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
        }
    }
}

void DeviceWindow::onFrameDecoded(const QImage &frame)
{
    if (ui->label_videoStream->text().isEmpty() == false) {
        ui->label_videoStream->clear();
    }
    QSize newFrameSize = frame.size();
    if (!newFrameSize.isEmpty() && newFrameSize != mCurrentFrameSize) {
        mCurrentFrameSize = newFrameSize;
        if (!mDeviceName.isEmpty()) {
            emit statusUpdated(mSerial, mDeviceName, mCurrentFrameSize);
        }
        qDebug() << "Frame size changed to:" << newFrameSize << ". Adjusting window size...";
        QSize videoDisplaySize;
        double aspectRatio = static_cast<double>(newFrameSize.width()) / newFrameSize.height();
        if (aspectRatio < 1.0) { // 竖屏
            videoDisplaySize.setHeight(BASE_HEIGHT_PORTRAIT);
            videoDisplaySize.setWidth(qRound(BASE_HEIGHT_PORTRAIT * aspectRatio));
        } else { // 横屏
            videoDisplaySize.setWidth(BASE_WIDTH_LANDSCAPE);
            videoDisplaySize.setHeight(qRound(BASE_WIDTH_LANDSCAPE / aspectRatio));
        }

        ui->label_videoStream->setMinimumSize(videoDisplaySize);
        ui->label_videoStream->setMaximumSize(videoDisplaySize);

        this->adjustSize();
        ui->label_videoStream->setMinimumSize(0, 0);
        ui->label_videoStream->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        // UI刷新时序修复：放弃这一帧，等待下一帧在尺寸正确的窗口中绘制
        return;
    }
    // 窗口尺寸正确，直接更新图像
    ui->label_videoStream->setPixmap(QPixmap::fromImage(frame).scaled(
        ui->label_videoStream->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        ));
}

void DeviceWindow::onDecodingFinished(const QString &message)
{
    qDebug() << "From DecoderThread:" << message;
    if (message.contains("错误") && this->isVisible()) {
        QMessageBox::warning(this, "解码错误", message);
    }
}

void DeviceWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    // 这个函数现在只处理“连接成功后”发生的错误
    if (mVideoSocket && mVideoSocket->state() != QAbstractSocket::ConnectingState && this->isVisible()) {
        QMessageBox::critical(this, "网络错误", "Socket 连接中断: " + mVideoSocket->errorString());
        close();
    }
}

void DeviceWindow::onSocketDisconnected()
{
    qDebug() << "Socket disconnected.";
    if (this->isVisible()) {
        ui->label_videoStream->setText("连接已断开");
    }
}


void DeviceWindow::stopAll()
{
    qDebug() << "Stopping all services for" << mSerial;
    if (mDecoder) {
        mDecoder->stop();
        mDecoder->quit();
        mDecoder->wait(2000);
        mDecoder->deleteLater();
        mDecoder = nullptr;
    }
    if (mVideoSocket) {
        mVideoSocket->close();
        mVideoSocket->deleteLater();
        mVideoSocket = nullptr;
    }
    if (mServerProcess) {
        mServerProcess->terminate();
        mServerProcess->waitForFinished(1000);
        mServerProcess->deleteLater();
        mServerProcess = nullptr;
    }
    if (mControlSender) {
        mControlSender->disconnectFromServer();
        mControlSender->deleteLater();
        mControlSender = nullptr;
    }
    AdbProcess *removeForwardProcess = new AdbProcess(this);
    connect(removeForwardProcess, &AdbProcess::finished, removeForwardProcess, &QObject::deleteLater);
    removeForwardProcess->execute(mSerial, {"forward", "--remove", QString("tcp:%1").arg(mLocalPort)});
    qDebug() << "Cleanup commands sent for" << mSerial;
}


// 坐标转换辅助函数
QPoint DeviceWindow::mapMousePosition(const QPoint &pos)
{
    if (mCurrentFrameSize.isEmpty()) {
        return QPoint(); // 无效
    }

    // 计算视频在 QLabel 中的实际显示区域
    QSize labelSize = ui->label_videoStream->size();
    QSize videoSize = mCurrentFrameSize;
    videoSize.scale(labelSize, Qt::KeepAspectRatio);
    int marginX = (labelSize.width() - videoSize.width()) / 2;
    int marginY = (labelSize.height() - videoSize.height()) / 2;

    // 从 QLabel 坐标转换到视频内的相对坐标
    int videoX = pos.x() - marginX;
    int videoY = pos.y() - marginY;

    // 检查是否在视频区域内
    if (videoX < 0 || videoY < 0 || videoX > videoSize.width() || videoY > videoSize.height()) {
        return QPoint(); // 点击在黑边上，无效
    }
    // 将视频内的相对坐标映射到手机的真实分辨率坐标
    int phoneX = videoX * mCurrentFrameSize.width() / videoSize.width();
    int phoneY = videoY * mCurrentFrameSize.height() / videoSize.height();

    return QPoint(phoneX, phoneY);
}
void DeviceWindow::mousePressEvent(QMouseEvent *event)
{
    if (mControlSender && event->button() == Qt::LeftButton) {
        // --- 核心修改：坐标转换 ---
        // 1. 将窗口坐标 event->pos() 映射到 QLabel 的坐标系中
        QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
        // 2. 使用 QLabel 的局部坐标进行后续计算
        QPoint devicePos = mapMousePosition(labelPos);
        // --- 修改结束 ---
        if (!devicePos.isNull()) {
            qDebug() << "Mouse Press at device coords:" << devicePos;
            mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_DOWN, devicePos, mCurrentFrameSize);
            mIsMousePressed = true;
        }
    }
}
void DeviceWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (mControlSender && event->button() == Qt::LeftButton) {
        // --- 核心修改：坐标转换 ---
        QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
        QPoint devicePos = mapMousePosition(labelPos);
        // --- 修改结束 ---
        if (!devicePos.isNull()) {
            qDebug() << "Mouse Release at device coords:" << devicePos;
            mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_UP, devicePos, mCurrentFrameSize);
        }
        mIsMousePressed = false;
    }
}
void DeviceWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (mControlSender && mIsMousePressed) {
        // --- 核心修改：坐标转换 ---
        QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
        QPoint devicePos = mapMousePosition(labelPos);
        // --- 修改结束 ---

        if (!devicePos.isNull()) {
            mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_MOVE, devicePos, mCurrentFrameSize);
        }
    }
}
void DeviceWindow::keyPressEvent(QKeyEvent *event)
{
    if(mControlSender) {
        int androidKey = qtKeyToAndroidKey(event->key());
        if (androidKey != AKEYCODE_UNKNOWN) {
            mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, androidKey, qtModifiersToAndroidMetaState(event->modifiers()));
        }
        // 同时发送文本，这样输入法才能正常工作
        if (!event->text().isEmpty()) {
            mControlSender->postInjectText(event->text());
        }
    }
}
void DeviceWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (mControlSender) {
        int androidKey = qtKeyToAndroidKey(event->key());
        if (androidKey != AKEYCODE_UNKNOWN) {
            mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, androidKey, qtModifiersToAndroidMetaState(event->modifiers()));
        }
    }
}
// ========== 新增：工具栏按钮槽函数实现 ==========
void DeviceWindow::setupToolbarActions()
{
    // connect(ui->action_power, &QAction::triggered, this, &DeviceWindow::on_action_power_triggered);
    // connect(ui->action_volumeUp, &QAction::triggered, this, &DeviceWindow::on_action_volumeUp_triggered);
    // connect(ui->action_volumeDown, &QAction::triggered, this, &DeviceWindow::on_action_volumeDown_triggered);
    // connect(ui->action_rotateDevice, &QAction::triggered, this, &DeviceWindow::on_action_rotateDevice_triggered);
    // connect(ui->action_home, &QAction::triggered, this, &DeviceWindow::on_action_home_triggered);
    // connect(ui->action_back, &QAction::triggered, this, &DeviceWindow::on_action_back_triggered);
    // connect(ui->action_appSwitch, &QAction::triggered, this, &DeviceWindow::on_action_appSwitch_triggered);
    // connect(ui->action_menu, &QAction::triggered, this, &DeviceWindow::on_action_menu_triggered);
    // connect(ui->action_expandNotifications, &QAction::triggered, this, &DeviceWindow::on_action_expandNotifications_triggered);
    // connect(ui->action_collapseNotifications, &QAction::triggered, this, &DeviceWindow::on_action_collapseNotifications_triggered);
    // connect(ui->action_screenshot, &QAction::triggered, this, &DeviceWindow::on_action_screenshot_triggered);
}
void DeviceWindow::on_action_power_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_POWER);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_POWER);
}
void DeviceWindow::on_action_volumeUp_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_VOLUME_UP);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_VOLUME_UP);
}
void DeviceWindow::on_action_volumeDown_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_VOLUME_DOWN);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_VOLUME_DOWN);
}
void DeviceWindow::on_action_rotateDevice_triggered()
{
    if (mControlSender) mControlSender->postRotateDevice();
}
void DeviceWindow::on_action_home_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_HOME);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_HOME);
}
void DeviceWindow::on_action_back_triggered()
{
    if (mControlSender) {
        mControlSender->postBackOrScreenOn(AKEY_EVENT_ACTION_DOWN);
        mControlSender->postBackOrScreenOn(AKEY_EVENT_ACTION_UP);
    }
}
void DeviceWindow::on_action_appSwitch_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_APP_SWITCH);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_APP_SWITCH);
}
void DeviceWindow::on_action_menu_triggered()
{
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_MENU);
    if (mControlSender) mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_MENU);
}
void DeviceWindow::on_action_expandNotifications_triggered()
{
    if (mControlSender) mControlSender->postExpandNotificationPanel();
}
void DeviceWindow::on_action_collapseNotifications_triggered()
{
    if (mControlSender) mControlSender->postCollapseNotificationPanel();
}
void DeviceWindow::on_action_screenshot_triggered()
{
    if (mCurrentFrameSize.isEmpty()) return;
    QPixmap screenshot = ui->label_videoStream->pixmap(Qt::ReturnByValue);
    if (screenshot.isNull()) return;
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString fileName = QString("Screenshot_%1_%2.png")
                           .arg(mSerial)
                           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString savePath = QFileDialog::getSaveFileName(this, "保存截图",
                                                    QDir(defaultPath).filePath(fileName),
                                                    "PNG Images (*.png)");
    if (!savePath.isEmpty()) {
        if (screenshot.save(savePath, "PNG")) {
            qDebug() << "Screenshot saved to" << savePath;
        } else {
            QMessageBox::warning(this, "保存失败", "无法将截图保存到指定位置。");
        }
    }
}

int DeviceWindow::qtKeyToAndroidKey(int qtKey) {
    switch (qtKey) {
    case Qt::Key_Home:          return AKEYCODE_HOME;
    case Qt::Key_Escape:        return AKEYCODE_BACK;
    case Qt::Key_Backspace:     return AKEYCODE_DEL;
    case Qt::Key_Delete:        return AKEYCODE_FORWARD_DEL;
    case Qt::Key_Return:        return AKEYCODE_ENTER;
    case Qt::Key_Enter:         return AKEYCODE_ENTER;
    case Qt::Key_Tab:           return AKEYCODE_TAB;
    case Qt::Key_Space:         return AKEYCODE_SPACE;
    case Qt::Key_Up:            return AKEYCODE_DPAD_UP;
    case Qt::Key_Down:          return AKEYCODE_DPAD_DOWN;
    case Qt::Key_Left:          return AKEYCODE_DPAD_LEFT;
    case Qt::Key_Right:         return AKEYCODE_DPAD_RIGHT;
    case Qt::Key_A:             return AKEYCODE_A;
    case Qt::Key_B:             return AKEYCODE_B;
    case Qt::Key_C:             return AKEYCODE_C;
    case Qt::Key_D:             return AKEYCODE_D;
    case Qt::Key_E:             return AKEYCODE_E;
    case Qt::Key_F:             return AKEYCODE_F;
    case Qt::Key_G:             return AKEYCODE_G;
    case Qt::Key_H:             return AKEYCODE_H;
    case Qt::Key_I:             return AKEYCODE_I;
    case Qt::Key_J:             return AKEYCODE_J;
    case Qt::Key_K:             return AKEYCODE_K;
    case Qt::Key_L:             return AKEYCODE_L;
    case Qt::Key_M:             return AKEYCODE_M;
    case Qt::Key_N:             return AKEYCODE_N;
    case Qt::Key_O:             return AKEYCODE_O;
    case Qt::Key_P:             return AKEYCODE_P;
    case Qt::Key_Q:             return AKEYCODE_Q;
    case Qt::Key_R:             return AKEYCODE_R;
    case Qt::Key_S:             return AKEYCODE_S;
    case Qt::Key_T:             return AKEYCODE_T;
    case Qt::Key_U:             return AKEYCODE_U;
    case Qt::Key_V:             return AKEYCODE_V;
    case Qt::Key_W:             return AKEYCODE_W;
    case Qt::Key_X:             return AKEYCODE_X;
    case Qt::Key_Y:             return AKEYCODE_Y;
    case Qt::Key_Z:             return AKEYCODE_Z;
    // ... 添加更多映射
    default:                    return AKEYCODE_UNKNOWN;
    }
}
int DeviceWindow::qtModifiersToAndroidMetaState(Qt::KeyboardModifiers modifiers) {
    int metaState = 0;
    if (modifiers & Qt::ShiftModifier) {
        metaState |= AMETA_SHIFT_ON;
    }
    if (modifiers & Qt::AltModifier) {
        metaState |= AMETA_ALT_ON;
    }
    if (modifiers & Qt::ControlModifier) {
        metaState |= AMETA_CTRL_ON;
    }
    return metaState;
}
