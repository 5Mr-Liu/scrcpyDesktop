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

const int BASE_HEIGHT_PORTRAIT = 800;  // 竖屏时，我们希望窗口内容区的高度
const int BASE_WIDTH_LANDSCAPE = 960; // 横屏时，我们希望窗口内容区的宽度

DeviceWindow::DeviceWindow(const QString &serial, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceWindow),
    mSerial(serial),
    mLocalPort(27183),
    mServerProcess(nullptr),
    mVideoSocket(nullptr),
    mDecoder(nullptr),
    mConnectionRetries(0),
    mCurrentFrameSize(0,0)
{
    ui->setupUi(this);
    setWindowTitle(QString("设备 - %1").arg(mSerial));
    ui->label_videoStream->setStyleSheet("color: white; background-color: black;");
    ui->label_videoStream->setAlignment(Qt::AlignCenter);
    ui->label_videoStream->setText("正在连接设备...");

    // === 使用你的 ScrcpyOptions ===
    // 默认构造函数已经设置了很好的默认值。
    // 如果需要，可以在这里覆盖它们，例如：
    // mOptions.max_size = 1280;
    // mOptions.video_bit_rate = 6000000; // 6 Mbps

    QTimer::singleShot(100, this, &DeviceWindow::startStreaming);
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

        qDebug() << "Step 4: Preparing to connect with retries...";
        ui->label_videoStream->setText("Step 4: Connecting...");

        mConnectionRetries = 0; // 重置计数器
        // 给予服务器 500ms 的初始启动时间
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
        mDecoder = new VideoDecoderThread(this);
        connect(mDecoder, &VideoDecoderThread::frameDecoded, this, &DeviceWindow::onFrameDecoded);
        connect(mDecoder, &VideoDecoderThread::decodingFinished, this, &DeviceWindow::onDecodingFinished);
        connect(mDecoder, &VideoDecoderThread::deviceNameReady, this, [this](const QString &name){
            setWindowTitle(QString("%1 - %2").arg(name).arg(mSerial));
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
    qDebug() << "Socket connected! Handing over to decoder thread...";
    ui->label_videoStream->setText("连接成功，等待设备元数据...");
    mConnectionRetries = 0; // 成功后重置
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
    AdbProcess *removeForwardProcess = new AdbProcess(this);
    connect(removeForwardProcess, &AdbProcess::finished, removeForwardProcess, &QObject::deleteLater);
    removeForwardProcess->execute(mSerial, {"forward", "--remove", QString("tcp:%1").arg(mLocalPort)});
    qDebug() << "Cleanup commands sent for" << mSerial;
}
