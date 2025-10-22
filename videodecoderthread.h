#ifndef VIDEODECODERTHREAD_H
#define VIDEODECODERTHREAD_H

#include <QThread>
#include <QImage>
#include <QByteArray>
#include <QMutex>

// Forward declarations
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

/**
 * @class VideoDecoderThread
 * @brief High-performance video decoder with minimal locking
 */
class VideoDecoderThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoDecoderThread(const QString &codecName, QObject *parent = nullptr);
    ~VideoDecoderThread();

    void stop();

public slots:
    void decodeData(const QByteArray &data);

signals:
    void frameDecoded(const QImage &frame);
    void decodingFinished(const QString &message);
    void deviceNameReady(const QString &name);
    void errorOccurred(const QString &error);

protected:
    void run() override;

private:
    void processBuffer();
    bool initializeDecoder();
    void cleanup();
    QImage convertFrameToImage(AVFrame* frame);

    // Inline helpers for better performance
    inline bool validateResolution(quint32 w, quint32 h) const {
        return w > 0 && h > 0 && w <= 8192 && h <= 8192;
    }

    inline bool validatePacketSize(quint32 size) const {
        return size > 0 && size <= 5 * 1024 * 1024;
    }

    // Use simple bool instead of QAtomicInt
    volatile bool mRunning;

    // Mutex only for buffer access (minimal locking)
    QMutex m_bufferMutex;


    AVCodecContext *m_codecContext = nullptr;
    AVFrame *m_frame = nullptr;
    AVPacket *m_packet = nullptr;
    SwsContext *m_swsContext = nullptr;
    QString mCodecName;
    int m_lastFrameWidth = 0;
    int m_lastFrameHeight = 0;

    enum StreamingState {
        STATE_READING_DUMMY_BYTE,
        STATE_READING_DEVICE_META,
        STATE_READING_VIDEO_HEADER,
        STATE_READING_PACKET_HEADER,
        STATE_READING_PACKET_PAYLOAD
    };

    StreamingState m_state = STATE_READING_DUMMY_BYTE;
    QByteArray m_buffer;
    quint32 m_payloadSize = 0;

    // Pre-allocate for better performance
    static constexpr int INITIAL_BUFFER_CAPACITY = 512 * 1024; // 512KB
};

#endif // VIDEODECODERTHREAD_H
