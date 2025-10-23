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

DeviceWindow::DeviceWindow(const QString &serial, const ScrcpyOptions &options, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceWindow),
    mSerial(serial),
    mOptions(options),
    mLocalPort(27183),
    mConnectionRetries(0),
    mCurrentFrameSize(0, 0)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/assert/title.ico"));

    // Apply window settings
    if (!mOptions.window_title.isEmpty()) {
        setWindowTitle(mOptions.window_title);
    } else {
        setWindowTitle(tr("Device - %1").arg(mSerial));
    }

    Qt::WindowFlags flags = windowFlags();
    if (mOptions.always_on_top) {
        flags |= Qt::WindowStaysOnTopHint;
    }
    if (mOptions.window_borderless) {
        flags |= Qt::FramelessWindowHint;
    }
    setWindowFlags(flags);

    // OPTIMIZATION: Pre-configure video label for best performance
    ui->label_videoStream->setStyleSheet("color: white; background-color: black;");
    ui->label_videoStream->setAlignment(Qt::AlignCenter);
    ui->label_videoStream->setText(tr("Connecting to device..."));

    // CRITICAL: Enable automatic scaling - this is KEY for performance
    // QLabel will handle scaling internally using hardware acceleration
    ui->label_videoStream->setScaledContents(true);

    setupToolbarActions();
    this->setFocusPolicy(Qt::StrongFocus);

    // Delay start to allow window initialization
    QTimer::singleShot(100, this, &DeviceWindow::startStreaming);

    // Handle fullscreen after window is shown
    if (mOptions.fullscreen) {
        QTimer::singleShot(250, this, [this](){
            if (this) {
                this->showFullScreen();
            }
        });
    }

#ifdef QT_DEBUG
    mLastFpsTime = QDateTime::currentMSecsSinceEpoch();
#endif
}

DeviceWindow::~DeviceWindow()
{
    qDebug() << "[DeviceWindow]" << mSerial << "destructor called";
    stopAll();
    delete ui;
}

QString DeviceWindow::getSerial() const
{
    return mSerial;
}

void DeviceWindow::showError(const QString &title, const QString &message, bool fatal)
{
    qCritical() << "[DeviceWindow]" << mSerial << "-" << title << ":" << message;

    if (isVisible()) {
        QMessageBox::critical(this, title, message);
    }

    if (fatal) {
        QTimer::singleShot(100, this, &DeviceWindow::close);
    }
}

void DeviceWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "[DeviceWindow] Closing window for" << mSerial;

    // Handle recording file pull with SAFE this pointer
    if (!mOptions.record_file.isEmpty()) {
        QString format = (mOptions.record_format == "auto" ? "mkv" : mOptions.record_format);
        QString device_path = QString("/sdcard/%1.%2").arg("temp_record_file").arg(format);
        QString pc_path = mOptions.record_file;

        qDebug() << "[DeviceWindow] Pulling recording from" << device_path << "to" << pc_path;

        AdbProcess *pullProcess = new AdbProcess();

        // CRITICAL FIX: Use QPointer to safely capture 'this'
        QPointer<DeviceWindow> safeThis(this);

        connect(pullProcess, &AdbProcess::finished, pullProcess,
                [safeThis, pullProcess, device_path, pc_path](int exitCode, QProcess::ExitStatus status){
                    if (status == QProcess::NormalExit && exitCode == 0) {
                        qInfo() << "[DeviceWindow] Record file pulled successfully";

                        // Only show message if window still exists
                        if (safeThis) {
                            QMessageBox::information(safeThis, tr("Recording Successful"),
                                                     tr("The recording has been saved to:\n%1").arg(QDir::toNativeSeparators(pc_path)));
                        }

                        // Delete temp file from device
                        AdbProcess *rmProcess = new AdbProcess();
                        connect(rmProcess, &AdbProcess::finished, rmProcess, &QObject::deleteLater);

                        if (safeThis) {
                            rmProcess->execute(safeThis->mSerial, {"shell", "rm", device_path});
                        } else {
                            rmProcess->deleteLater();
                        }
                    } else {
                        qWarning() << "[DeviceWindow] Failed to pull record file:" << pullProcess->getOutput();

                        if (safeThis) {
                            QMessageBox::warning(safeThis, tr("Recording Failed"),
                                                 tr("Could not pull the recording from the device.\n") + pullProcess->getOutput());
                        }
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
    ui->label_videoStream->setText(tr("Step 1: Pushing server..."));
    qDebug() << "[DeviceWindow] Step 1: Pushing server file";
    pushServer();
}

void DeviceWindow::pushServer()
{
    QString serverFileName = QString("scrcpy-server-v%1").arg(mOptions.version);
    QString serverLocalPath = QDir(QCoreApplication::applicationDirPath()).filePath(serverFileName);

    if (!QFile::exists(serverLocalPath)) {
        showError(tr("Fatal Error"),
                  tr("scrcpy-server file not found!\nPath: %1").arg(serverLocalPath),
                  true);
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
    if (!process) return;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "[DeviceWindow] Server pushed successfully";
        ui->label_videoStream->setText(tr("Step 2: Forwarding port..."));
        forwardPort();
    } else {
        showError(tr("Error"),
                  tr("Failed to push scrcpy-server file!\n") + process->getOutput(),
                  true);
    }
    process->deleteLater();
}

void DeviceWindow::forwardPort()
{
    QString forwardRule = QString("tcp:%1").arg(mLocalPort);
    AdbProcess *forwardProcess = new AdbProcess(this);
    connect(forwardProcess, &AdbProcess::finished, this, &DeviceWindow::onForwardPortFinished);
    forwardProcess->execute(mSerial, {"forward", forwardRule, "localabstract:scrcpy"});
}

void DeviceWindow::onForwardPortFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto process = qobject_cast<AdbProcess*>(sender());
    if (!process) return;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "[DeviceWindow] Port forwarded successfully";
        ui->label_videoStream->setText(tr("Step 3: Starting server..."));
        startServer();

        qDebug() << "[DeviceWindow] Step 4: Preparing to connect video socket";
        ui->label_videoStream->setText(tr("Step 4: Connecting video stream..."));
        mConnectionRetries = 0;

        // Initial delay before first connection attempt
        QTimer::singleShot(500, this, &DeviceWindow::connectToSocketWithRetry);
    } else {
        showError(tr("Error"),
                  tr("Port forwarding failed!\n") + process->getOutput(),
                  true);
    }
    process->deleteLater();
}

void DeviceWindow::startServer()
{
    if (mServerProcess) {
        mServerProcess->deleteLater();
    }

    mServerProcess = new AdbProcess(this);

    // OPTIMIZATION: Use QPointer in lambda to prevent accessing deleted object
    QPointer<DeviceWindow> safeThis(this);
    connect(mServerProcess.data(), &AdbProcess::finished,
            [safeThis](int, QProcess::ExitStatus){
                qDebug() << "[DeviceWindow] ADB shell process finished";
                if (safeThis && safeThis->isVisible()) {
                    safeThis->ui->label_videoStream->setText(safeThis->tr("Connection lost."));
                }
            });

    QStringList args = mOptions.toAdbShellArgs();
    qDebug() << "[DeviceWindow] Starting server with args:" << args.join(" ");
    mServerProcess->execute(mSerial, args);
}

void DeviceWindow::connectToSocketWithRetry()
{
    if (mConnectionRetries >= DisplayConfig::MAX_CONNECTION_RETRIES) {
        showError(tr("Connection Failed"),
                  tr("Could not connect to the scrcpy service on the device after %1 attempts.\n"
                     "Please check if the device is unlocked or has a permissions dialog.")
                      .arg(DisplayConfig::MAX_CONNECTION_RETRIES),
                  true);
        return;
    }

    mConnectionRetries++;
    qDebug() << "[DeviceWindow] Connection attempt" << mConnectionRetries
             << "of" << DisplayConfig::MAX_CONNECTION_RETRIES;

    // Initialize decoder once
    if (!mDecoder) {
        mDecoder = new VideoDecoderThread(mOptions.video_codec, this);

        connect(mDecoder.data(), &VideoDecoderThread::frameDecoded,
                this, &DeviceWindow::onFrameDecoded,
                Qt::QueuedConnection);

        connect(mDecoder.data(), &VideoDecoderThread::decodingFinished,
                this, &DeviceWindow::onDecodingFinished);

        QPointer<DeviceWindow> safeThis(this);
        connect(mDecoder.data(), &VideoDecoderThread::deviceNameReady,
                [safeThis](const QString &name){
                    if (!safeThis) return;

                    safeThis->mDeviceName = name;
                    if (safeThis->mOptions.window_title.isEmpty()) {
                        safeThis->setWindowTitle(QString("%1 - %2").arg(name).arg(safeThis->mSerial));
                    }
                    if (!safeThis->mCurrentFrameSize.isEmpty()) {
                        emit safeThis->statusUpdated(safeThis->mSerial, safeThis->mDeviceName,
                                                     safeThis->mCurrentFrameSize);
                    }
                });

        mDecoder->start();
    }

    // Create new socket for this attempt
    if (!mVideoSocket) {
        mVideoSocket = new QTcpSocket(this);

        optimizeSocketForLowLatency(mVideoSocket.data());

        connect(mVideoSocket.data(), &QTcpSocket::connected,
                this, &DeviceWindow::onSocketConnected);
        connect(mVideoSocket.data(), &QTcpSocket::disconnected,
                this, &DeviceWindow::onSocketDisconnected);
        connect(mVideoSocket.data(), &QTcpSocket::readyRead,
                this, &DeviceWindow::onSocketReadyRead);

        // FIXED: Use standard Qt::AutoConnection (default)
        QPointer<DeviceWindow> safeThis(this);
        connect(mVideoSocket.data(), &QTcpSocket::errorOccurred,
                [safeThis](QAbstractSocket::SocketError error) {
                    if (!safeThis || !safeThis->mVideoSocket) return;

                    // Only log non-connection errors (connection refused is expected during retry)
                    if (error != QAbstractSocket::ConnectionRefusedError) {
                        qWarning() << "[DeviceWindow] Socket error:"
                                   << safeThis->mVideoSocket->errorString();
                    }

                    // Clean up socket
                    safeThis->mVideoSocket->deleteLater();
                    safeThis->mVideoSocket.clear();

                    // Retry after delay
                    QTimer::singleShot(DisplayConfig::RETRY_DELAY_MS, safeThis,
                                       &DeviceWindow::connectToSocketWithRetry);
                });  //Removed Qt::SingleShotConnection
    }

    // Use 127.0.0.1 for faster connection
    mVideoSocket->connectToHost("127.0.0.1", mLocalPort);
}


void DeviceWindow::onSocketConnected()
{
    qDebug() << "[DeviceWindow] Video socket connected successfully";
    ui->label_videoStream->setText(tr("Connection successful, waiting for device metadata..."));
    mConnectionRetries = 0;

    // Connect control socket
    qDebug() << "[DeviceWindow] Connecting control socket";
    if (!mControlSender) {
        mControlSender = new ControlSender(this);

        connect(mControlSender.data(), &ControlSender::controlSocketConnected,
                [](){
                    qDebug() << "[Control] Control socket connected";
                });
        connect(mControlSender.data(), &ControlSender::controlSocketDisconnected,
                [](){
                    qDebug() << "[Control] Control socket disconnected";
                });
    }
    mControlSender->connectToServer("127.0.0.1", mLocalPort);
}

void DeviceWindow::onSocketReadyRead()
{
    if (!mVideoSocket || !mDecoder) return;

    // OPTIMIZATION: Read ALL available data at once (reduces system calls)
    qint64 available = mVideoSocket->bytesAvailable();
    if (available <= 0) return;

    // Read in optimal chunks to reduce overhead
    static constexpr qint64 MAX_READ_SIZE = 65536; // 64KB chunks

    while (mVideoSocket->bytesAvailable() > 0) {
        qint64 toRead = qMin(mVideoSocket->bytesAvailable(), MAX_READ_SIZE);
        QByteArray data = mVideoSocket->read(toRead);

        if (data.isEmpty()) break;

        // Queue data to decoder thread (already thread-safe via Qt::QueuedConnection)
        QMetaObject::invokeMethod(mDecoder.data(), "decodeData",
                                  Qt::QueuedConnection,
                                  Q_ARG(QByteArray, data));
    }
}


void DeviceWindow::updateCoordinateTransform()
{
    if (mCurrentFrameSize.isEmpty()) {
        mTransform.isValid = false;
        qDebug() << "[DeviceWindow] Transform invalid - no frame size";
        return;
    }

    QSize labelSize = ui->label_videoStream->size();
    if (labelSize.isEmpty() || labelSize.width() < 10 || labelSize.height() < 10) {
        mTransform.isValid = false;
        qDebug() << "[DeviceWindow] Transform invalid - label not ready:" << labelSize;
        return;
    }

    // Calculate how the video fits in the label (keeping aspect ratio)
    QSize videoSize = mCurrentFrameSize;
    videoSize.scale(labelSize, Qt::KeepAspectRatio);


    if (videoSize.width() <= 0 || videoSize.height() <= 0) {
        mTransform.isValid = false;
        qDebug() << "[DeviceWindow] Transform invalid - zero videoSize";
        return;
    }

    // Calculate letterboxing/pillarboxing margins
    mTransform.videoSize = videoSize;
    mTransform.marginX = (labelSize.width() - videoSize.width()) / 2;
    mTransform.marginY = (labelSize.height() - videoSize.height()) / 2;

    // Calculate scaling factors
    mTransform.scaleX = static_cast<double>(mCurrentFrameSize.width()) / videoSize.width();
    mTransform.scaleY = static_cast<double>(mCurrentFrameSize.height()) / videoSize.height();
    mTransform.isValid = true;

    qDebug() << "[DeviceWindow] Transform updated:"
             << "Label:" << labelSize
             << "Video:" << videoSize
             << "Device:" << mCurrentFrameSize
             << "Scale:" << QString("%1x%2").arg(mTransform.scaleX, 0, 'f', 2).arg(mTransform.scaleY, 0, 'f', 2)
             << "Margins:" << mTransform.marginX << "," << mTransform.marginY;
}



void DeviceWindow::onFrameDecoded(const QImage &frame)
{
#ifdef QT_DEBUG
    // FPS counter for performance monitoring
    mFrameCount++;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - mLastFpsTime >= 1000) {
        qDebug() << "[DeviceWindow] FPS:" << mFrameCount;
        mFrameCount = 0;
        mLastFpsTime = now;
    }
#endif

    if (frame.isNull()) {
        qWarning() << "[DeviceWindow] Received null frame";
        return;
    }

    QSize newFrameSize = frame.size();


    bool resolutionChanged = !newFrameSize.isEmpty() && newFrameSize != mCurrentFrameSize;

    if (resolutionChanged) {
        qDebug() << "[DeviceWindow] Resolution change detected:"
                 << mCurrentFrameSize << "->" << newFrameSize;

        mCurrentFrameSize = newFrameSize;

        // Emit status update
        if (!mDeviceName.isEmpty()) {
            emit statusUpdated(mSerial, mDeviceName, mCurrentFrameSize);
        }


        QSize windowSize;
        double aspectRatio = static_cast<double>(newFrameSize.width()) / newFrameSize.height();

        if (aspectRatio < 1.0) { // Portrait mode
            int targetHeight = DisplayConfig::BASE_HEIGHT_PORTRAIT;
            int targetWidth = qRound(targetHeight * aspectRatio);
            windowSize = QSize(targetWidth, targetHeight);
            qDebug() << "[DeviceWindow] Portrait mode - Window size:" << windowSize;
        } else { // Landscape mode
            int targetWidth = DisplayConfig::BASE_WIDTH_LANDSCAPE;
            int targetHeight = qRound(targetWidth / aspectRatio);
            windowSize = QSize(targetWidth, targetHeight);
            qDebug() << "[DeviceWindow] Landscape mode - Window size:" << windowSize;
        }


        ui->label_videoStream->setFixedSize(windowSize);
        adjustSize();


        QPointer<DeviceWindow> safeThis(this);
        QTimer::singleShot(100, this, [safeThis, windowSize]() {
            if (!safeThis) return;

            // Allow resizing but set minimum size
            safeThis->ui->label_videoStream->setMinimumSize(windowSize / 2);
            safeThis->ui->label_videoStream->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);


            safeThis->updateCoordinateTransform();

            qDebug() << "[DeviceWindow] Window setup complete, ready for interaction";
        });

        // Configure scaling on first frame
        if (mFirstFrame) {
            ui->label_videoStream->setScaledContents(true);
            mFirstFrame = false;
        }
    }


    ui->label_videoStream->setPixmap(QPixmap::fromImage(frame));
}



void DeviceWindow::onDecodingFinished(const QString &message)
{
    qDebug() << "[DeviceWindow] Decoder:" << message;

    if (message.contains("Error", Qt::CaseInsensitive) && isVisible()) {
        QMessageBox::warning(this, tr("Decoding Error"), message);
    }
}

void DeviceWindow::onSocketError(QAbstractSocket::SocketError socketError)
{
    // Only handle errors after successful connection
    // Connection-time errors are handled by retry logic
    if (mVideoSocket &&
        mVideoSocket->state() != QAbstractSocket::ConnectingState &&
        isVisible()) {
        showError(tr("Network Error"),
                  tr("Socket connection interrupted: ") + mVideoSocket->errorString(),
                  true);
    }
}

void DeviceWindow::onSocketDisconnected()
{
    qDebug() << "[DeviceWindow] Socket disconnected";
    if (isVisible()) {
        ui->label_videoStream->setText(tr("Connection lost."));
    }
}

void DeviceWindow::stopAll()
{
    qDebug() << "[DeviceWindow] Stopping all services for" << mSerial;

    // Stop decoder thread (non-blocking)
    if (mDecoder) {
        mDecoder->stop();
        mDecoder->quit();

        // OPTIMIZATION: Don't block UI thread with wait()
        QPointer<VideoDecoderThread> decoder = mDecoder;
        QTimer::singleShot(0, [decoder]() {
            if (!decoder) return;

            if (decoder->wait(DisplayConfig::DECODER_STOP_TIMEOUT_MS)) {
                qDebug() << "[DeviceWindow] Decoder stopped gracefully";
            } else {
                qWarning() << "[DeviceWindow] Decoder timeout, terminating";
                decoder->terminate();
                decoder->wait(1000);
            }
            decoder->deleteLater();
        });

        mDecoder.clear();
    }

    // Close video socket
    if (mVideoSocket) {
        mVideoSocket->close();
        mVideoSocket->deleteLater();
        mVideoSocket.clear();
    }

    // Stop server process
    if (mServerProcess) {
        mServerProcess->terminate();
        if (!mServerProcess->waitForFinished(DisplayConfig::SERVER_PROCESS_TIMEOUT_MS)) {
            mServerProcess->kill();
        }
        mServerProcess->deleteLater();
        mServerProcess.clear();
    }

    // Disconnect control sender
    if (mControlSender) {
        mControlSender->disconnectFromServer();
        mControlSender->deleteLater();
        mControlSender.clear();
    }

    // Remove port forwarding
    AdbProcess *removeForwardProcess = new AdbProcess();
    connect(removeForwardProcess, &AdbProcess::finished,
            removeForwardProcess, &QObject::deleteLater);
    removeForwardProcess->execute(mSerial, {"forward", "--remove",
                                            QString("tcp:%1").arg(mLocalPort)});
}

QPoint DeviceWindow::mapMousePosition(const QPoint &pos)
{

    if (!mTransform.isValid || mCurrentFrameSize.isEmpty()) {
        qDebug() << "[DeviceWindow] Mouse mapping failed - transform not ready or no frame size";
        return QPoint();
    }

    // Calculate position relative to video area (excluding margins)
    int videoX = pos.x() - mTransform.marginX;
    int videoY = pos.y() - mTransform.marginY;

    // Check if click is within video area
    if (videoX < 0 || videoY < 0 ||
        videoX >= mTransform.videoSize.width() ||
        videoY >= mTransform.videoSize.height()) {
        // Click is on letterbox/pillarbox area
        return QPoint();
    }

    // Map to device coordinates
    int deviceX = qRound(videoX * mTransform.scaleX);
    int deviceY = qRound(videoY * mTransform.scaleY);


    deviceX = qBound(0, deviceX, mCurrentFrameSize.width() - 1);
    deviceY = qBound(0, deviceY, mCurrentFrameSize.height() - 1);

    return QPoint(deviceX, deviceY);
}



void DeviceWindow::mousePressEvent(QMouseEvent *event)
{

    if (!mControlSender || event->button() != Qt::LeftButton) {
        return;
    }
    QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
    QPoint devicePos = mapMousePosition(labelPos);
    if (!devicePos.isNull()) {
        qDebug() << "[DeviceWindow] Mouse press:"
                 << "Global:" << event->globalPos()
                 << "Label:" << labelPos
                 << "Device:" << devicePos;

        mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_DOWN, devicePos, mCurrentFrameSize);
        mIsMousePressed = true;
    } else {
        qDebug() << "[DeviceWindow] Mouse press ignored (outside video area or invalid transform)";
    }
}


void DeviceWindow::mouseReleaseEvent(QMouseEvent *event)
{

    if (!mControlSender || event->button() != Qt::LeftButton) {
        return;
    }
    QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
    QPoint devicePos = mapMousePosition(labelPos);
    if (!devicePos.isNull()) {
        qDebug() << "[DeviceWindow] Mouse release at device coords:" << devicePos;
        mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_UP, devicePos, mCurrentFrameSize);
    }
    mIsMousePressed = false;
}

void DeviceWindow::mouseMoveEvent(QMouseEvent *event)
{

    if (!mControlSender || !mIsMousePressed) {
        return;
    }
    QPoint labelPos = ui->label_videoStream->mapFromGlobal(event->globalPos());
    QPoint devicePos = mapMousePosition(labelPos);
    if (!devicePos.isNull()) {
        mControlSender->postInjectTouch(AMOTION_EVENT_ACTION_MOVE, devicePos, mCurrentFrameSize);
    }
}

void DeviceWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}


void DeviceWindow::keyPressEvent(QKeyEvent *event)
{
    if (!mControlSender) return;

    int androidKey = qtKeyToAndroidKey(event->key());
    if (androidKey != AKEYCODE_UNKNOWN) {
        mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, androidKey,
                                          qtModifiersToAndroidMetaState(event->modifiers()));
    }

    // Send text event for software keyboard support
    if (!event->text().isEmpty()) {
        mControlSender->postInjectText(event->text());
    }
}

void DeviceWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!mControlSender) return;

    int androidKey = qtKeyToAndroidKey(event->key());
    if (androidKey != AKEYCODE_UNKNOWN) {
        mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, androidKey,
                                          qtModifiersToAndroidMetaState(event->modifiers()));
    }
}

void DeviceWindow::setupToolbarActions()
{
    // Auto-connected by Qt's naming convention
}

// --- Toolbar Action Handlers ---

void DeviceWindow::on_action_power_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_POWER);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_POWER);
}

void DeviceWindow::on_action_volumeUp_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_VOLUME_UP);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_VOLUME_UP);
}

void DeviceWindow::on_action_volumeDown_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_VOLUME_DOWN);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_VOLUME_DOWN);
}


void DeviceWindow::on_action_home_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_HOME);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_HOME);
}

void DeviceWindow::on_action_back_triggered()
{
    if (!mControlSender) return;
    mControlSender->postBackOrScreenOn(AKEY_EVENT_ACTION_DOWN);
    mControlSender->postBackOrScreenOn(AKEY_EVENT_ACTION_UP);
}

void DeviceWindow::on_action_appSwitch_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_APP_SWITCH);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_APP_SWITCH);
}

void DeviceWindow::on_action_menu_triggered()
{
    if (!mControlSender) return;
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_DOWN, AKEYCODE_MENU);
    mControlSender->postInjectKeycode(AKEY_EVENT_ACTION_UP, AKEYCODE_MENU);
}

void DeviceWindow::on_action_expandNotifications_triggered()
{
    if (mControlSender) {
        mControlSender->postExpandNotificationPanel();
    }
}

void DeviceWindow::on_action_collapseNotifications_triggered()
{
    if (mControlSender) {
        mControlSender->postCollapseNotificationPanel();
    }
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

    QString savePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Screenshot"),
        QDir(defaultPath).filePath(fileName),
        tr("PNG Images (*.png)")
        );

    if (!savePath.isEmpty()) {
        if (screenshot.save(savePath, "PNG")) {
            qDebug() << "[DeviceWindow] Screenshot saved to" << savePath;
        } else {
            QMessageBox::warning(this, tr("Save Failed"),
                                 tr("Could not save the screenshot to the specified location."));
        }
    }

}

void DeviceWindow::optimizeSocketForLowLatency(QTcpSocket* socket)
{
    if (!socket) return;

    // CRITICAL: Disable Nagle's algorithm (reduces latency by 40-200ms!)
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Disable buffering for immediate delivery
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 0);

    // Set optimal buffer sizes (larger = more throughput, but we want low latency)
    // For USB connections, smaller buffers = lower latency
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 32768);   // 32KB
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 65536); // 64KB

    // For Wi-Fi, you might want larger buffers:
    // socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 131072); // 128KB

    qDebug() << "[DeviceWindow] Socket optimized for low latency";
}


// --- Key Mapping Helpers ---

int DeviceWindow::qtKeyToAndroidKey(int qtKey)
{
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
    default:                    return AKEYCODE_UNKNOWN;
    }
}

int DeviceWindow::qtModifiersToAndroidMetaState(Qt::KeyboardModifiers modifiers)
{
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
