#include "videodecoderthread.h"
#include <QDebug>
#include <QtEndian>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <pthread.h>
#include <sched.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
}

// Inline helpers for speed
static inline quint32 read_be32(const uchar *data) {
    quint32 val;
    memcpy(&val, data, 4);
    return qFromBigEndian(val);
}

static inline quint64 read_be64(const uchar *data) {
    quint64 val;
    memcpy(&val, data, 8);
    return qFromBigEndian(val);
}

VideoDecoderThread::VideoDecoderThread(const QString &codecName, QObject *parent)
    : QThread(parent), mRunning(false), mCodecName(codecName)
{
    // Pre-allocate buffer to reduce reallocations
    m_buffer.reserve(INITIAL_BUFFER_CAPACITY);
}

VideoDecoderThread::~VideoDecoderThread()
{
    stop();
    quit();
    wait();
    cleanup();
}

void VideoDecoderThread::stop()
{
    mRunning = false;
}

bool VideoDecoderThread::initializeDecoder()
{
    AVCodecID codecId;
    QString codecLower = mCodecName.toLower();

    if (codecLower == "h265" || codecLower == "hevc") {
        codecId = AV_CODEC_ID_HEVC;
    } else if (codecLower == "av1") {
        codecId = AV_CODEC_ID_AV1;
    } else {
        codecId = AV_CODEC_ID_H264;
    }

    const AVCodec *codec = avcodec_find_decoder(codecId);
    if (!codec) {
        emit errorOccurred(QString("Decoder not found: %1").arg(mCodecName));
        return false;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        emit errorOccurred("Failed to allocate codec context");
        return false;
    }

    // Performance optimizations
    m_codecContext->thread_count = 0; // Auto-detect (faster than manual)
    m_codecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    m_codecContext->max_b_frames = 0;  // No B-frames buffering
    m_codecContext->has_b_frames = 0;

    // Low-latency decoding for real-time streaming
    m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;

    av_opt_set_int(m_codecContext->priv_data, "delay", 0, 0);
    if (codecId == AV_CODEC_ID_H264) {
        av_opt_set(m_codecContext->priv_data, "tune", "zerolatency", 0);
        av_opt_set_int(m_codecContext->priv_data, "slice-max-size", 1500, 0); // MTU-sized slices
    }
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        emit errorOccurred("Failed to open codec");
        avcodec_free_context(&m_codecContext);
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();

    if (!m_packet || !m_frame) {
        emit errorOccurred("Failed to allocate packet/frame");
        return false;
    }

    return true;
}

void VideoDecoderThread::run()
{
    mRunning = true;

    #ifdef Q_OS_WIN
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    #else
        // Linux/macOS: Use nice value
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO) / 2;
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    #endif

    qDebug() << "[Decoder] Thread priority elevated for low latency";

    if (!initializeDecoder()) {
        emit decodingFinished("Decoder initialization failed");
        return;
    }

    emit decodingFinished("Decoder ready");
    exec();

    cleanup();
    emit decodingFinished("Decoder stopped");
}

void VideoDecoderThread::cleanup()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
}

void VideoDecoderThread::decodeData(const QByteArray &data)
{
    if (!mRunning || data.isEmpty()) return;

    // CRITICAL: Minimize lock duration
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.append(data);
    }

    // Process outside the lock
    processBuffer();
}

QImage VideoDecoderThread::convertFrameToImage(AVFrame* frame)
{
    if (!frame || frame->width <= 0 || frame->height <= 0) {
        return QImage();
    }

    // Recreate SwsContext only when resolution changes
    if (!m_swsContext || m_lastFrameWidth != frame->width || m_lastFrameHeight != frame->height) {
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
        }

        m_swsContext = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            frame->width, frame->height, AV_PIX_FMT_RGB32,
            SWS_FAST_BILINEAR, // Faster than SWS_BILINEAR
            nullptr, nullptr, nullptr);

        m_lastFrameWidth = frame->width;
        m_lastFrameHeight = frame->height;

        if (!m_swsContext) {
            return QImage();
        }
    }

    QImage image(frame->width, frame->height, QImage::Format_RGB32);
    const int stride[] = { static_cast<int>(image.bytesPerLine()) };
    uint8_t* dest[] = { image.bits() };

    sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height, dest, stride);

    return image;
}

void VideoDecoderThread::processBuffer()
{
    if (!mRunning) return;

    bool progress = true;
    while (progress && mRunning) {
        progress = false;

        // Lock only when reading/modifying buffer
        m_bufferMutex.lock();
        int bufferSize = m_buffer.size();

        switch (m_state) {
        case STATE_READING_DUMMY_BYTE: {
            if (bufferSize >= 1) {
                m_buffer.remove(0, 1);
                m_state = STATE_READING_DEVICE_META;
                progress = true;
            }
            m_bufferMutex.unlock();
            break;
        }

        case STATE_READING_DEVICE_META: {
            if (bufferSize >= 64) {
                QByteArray metaData = m_buffer.left(64);
                m_buffer.remove(0, 64);
                m_state = STATE_READING_VIDEO_HEADER;
                progress = true;
                m_bufferMutex.unlock();

                // Process meta outside lock
                int nullPos = metaData.indexOf('\0');
                if (nullPos >= 0) metaData.truncate(nullPos);
                emit deviceNameReady(QString::fromUtf8(metaData).trimmed());
            } else {
                m_bufferMutex.unlock();
            }
            break;
        }

        case STATE_READING_VIDEO_HEADER: {
            if (bufferSize >= 12) {
                const uchar* header = reinterpret_cast<const uchar*>(m_buffer.constData());
                quint32 width = read_be32(header + 4);
                quint32 height = read_be32(header + 8);
                m_buffer.remove(0, 12);
                m_state = STATE_READING_PACKET_HEADER;
                progress = true;
                m_bufferMutex.unlock();

                if (!validateResolution(width, height)) {
                    emit errorOccurred(QString("Invalid resolution: %1x%2").arg(width).arg(height));
                    stop();
                    return;
                }
            } else {
                m_bufferMutex.unlock();
            }
            break;
        }

        case STATE_READING_PACKET_HEADER: {
            if (bufferSize >= 12) {
                const uchar* header = reinterpret_cast<const uchar*>(m_buffer.constData());
                m_payloadSize = read_be32(header + 8);
                m_buffer.remove(0, 12);

                if (m_payloadSize == 0) {
                    progress = true;
                    m_bufferMutex.unlock();
                    continue;
                }

                if (!validatePacketSize(m_payloadSize)) {
                    m_bufferMutex.unlock();
                    emit errorOccurred(QString("Invalid packet size: %1").arg(m_payloadSize));
                    stop();
                    return;
                }

                m_state = STATE_READING_PACKET_PAYLOAD;
                progress = true;
            }
            m_bufferMutex.unlock();
            break;
        }

        case STATE_READING_PACKET_PAYLOAD: {
            if (m_payloadSize > 0 && bufferSize >= static_cast<int>(m_payloadSize)) {
                // Copy payload data while locked
                QByteArray payloadData = m_buffer.left(m_payloadSize);
                m_buffer.remove(0, m_payloadSize);
                m_state = STATE_READING_PACKET_HEADER;
                m_payloadSize = 0;
                progress = true;
                m_bufferMutex.unlock();
                // ✅ OPTIMIZATION: Decode immediately without queuing
                if (av_new_packet(m_packet, payloadData.size()) >= 0) {
                    memcpy(m_packet->data, payloadData.constData(), payloadData.size());

                    // ✅ NEW: Set packet flags for immediate decoding
                    m_packet->flags |= AV_PKT_FLAG_KEY; // Hint: treat as keyframe for faster decode
                    if (avcodec_send_packet(m_codecContext, m_packet) >= 0) {
                        // ✅ CRITICAL: Process ALL available frames immediately
                        int frameCount = 0;
                        while (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
                            QImage image = convertFrameToImage(m_frame);
                            if (!image.isNull()) {
                                emit frameDecoded(image);
                                frameCount++;
                            }
                        }
                    #ifdef QT_DEBUG
                        if (frameCount > 1) {
                            qDebug() << "[Decoder] Processed" << frameCount << "frames in one packet";
                        }
                    #endif
                    }
                    av_packet_unref(m_packet);
                }
            } else {
                m_bufferMutex.unlock();
            }
            break;
        }

        default:
            m_bufferMutex.unlock();
            break;
        }
    }
}
