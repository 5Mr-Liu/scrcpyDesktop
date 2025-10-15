// videodecoderthread.h

#ifndef VIDEODECODERTHREAD_H
#define VIDEODECODERTHREAD_H

#include <QThread>
#include <QImage>
#include <QByteArray>

// FFmpeg forward declarations
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class VideoDecoderThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoDecoderThread(QObject *parent = nullptr);
    ~VideoDecoderThread();

    void stop();

public slots:
    void decodeData(const QByteArray &data);

protected:
    void run() override;

private:
    void processBuffer();
    bool initializeDecoder();
    void cleanup();

signals:
    void frameDecoded(const QImage &frame);
    void decodingFinished(const QString &message);
    void deviceNameReady(const QString &name); // 新增信号，用于在UI上显示设备名

private:
    volatile bool mRunning;
    AVCodecContext *m_codecContext = nullptr;
    AVFrame *m_frame = nullptr;
    AVPacket *m_packet = nullptr;
    SwsContext *m_swsContext = nullptr;
    int m_lastFrameWidth = 0;
    int m_lastFrameHeight = 0;

    // 全新的、更精确的状态机
    enum StreamingState {
        STATE_READING_DUMMY_BYTE,    // 等待读取并丢弃哑元字节
        STATE_READING_DEVICE_META,   // 等待读取64字节设备名
        STATE_READING_VIDEO_HEADER,  // 等待读取12字节视频头(含宽高)
        STATE_READING_PACKET_HEADER, // 等待读取12字节的帧头
        STATE_READING_PACKET_PAYLOAD // 等待读取N字节的帧数据
    };

    StreamingState m_state = STATE_READING_DUMMY_BYTE; // 初始状态
    QByteArray m_buffer;
    quint32 m_payloadSize = 0;
};

#endif // VIDEODECODERTHREAD_H
