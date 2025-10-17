// videodecoderthread.cpp

#include "videodecoderthread.h"
#include <QDebug>
#include <QtEndian>
#include <QHostAddress> // for ntohl

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

// 辅助函数：从字节数组中读取32位大端整数
// Qt的QHostAddress::ntohl可以完美地完成网络字节序到主机字节序的转换
quint32 read_be32(const uchar *data) {
    quint32 val;
    memcpy(&val, data, 4);
    return qFromBigEndian(val);
}

VideoDecoderThread::VideoDecoderThread(const QString &codecName,QObject *parent)
    : QThread(parent), mRunning(false), mCodecName(codecName)
{
}

VideoDecoderThread::~VideoDecoderThread()
{
    stop();
    quit();
    wait();
    cleanup();
    qDebug() << "[Decoder] VideoDecoderThread destroyed.";
}

void VideoDecoderThread::stop()
{
    mRunning = false;
}

bool VideoDecoderThread::initializeDecoder()
{
    AVCodecID codecId;
    if (mCodecName.toLower() == "h265") {
        codecId = AV_CODEC_ID_HEVC; // H.265 在 FFmpeg 中被称为 HEVC
    } else if (mCodecName.toLower() == "av1") {
        codecId = AV_CODEC_ID_AV1;
    } else {
        codecId = AV_CODEC_ID_H264; // 默认为 H.264
    }
    qDebug() << "[Decoder Init] Finding decoder" << mCodecName.toUpper() << "...";
    const AVCodec *codec = avcodec_find_decoder(codecId);
    if (!codec) {
        emit decodingFinished(QString("错误：找不到 %1 解码器。请检查您的FFmpeg库是否支持。").arg(mCodecName.toUpper()));
        return false;
    }
    qDebug() << "[Decoder Init] Decoder found.";
    qDebug() << "[Decoder Init] Allocating codec context...";
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        emit decodingFinished("错误：无法创建解码器上下文。");
        return false;
    }
    qDebug() << "[Decoder Init] Context allocated.";
    qDebug() << "[Decoder Init] Opening codec...";
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        emit decodingFinished("错误：无法打开解码器。");
        return false;
    }
    qDebug() << "[Decoder Init] Codec opened successfully.";
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_packet || !m_frame) {
        emit decodingFinished("错误：无法分配 Frame 或 Packet。");
        return false;
    }
    return true;
}

void VideoDecoderThread::run()
{
    mRunning = true;
    if (initializeDecoder()) {
        emit decodingFinished("解码器已就绪，等待数据...");
        exec(); // 进入事件循环，等待 decodeData 被调用
    }
    mRunning = false;
    cleanup(); // 退出事件循环后清理资源
    emit decodingFinished("解码线程已停止。");
}

void VideoDecoderThread::cleanup()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
}

void VideoDecoderThread::decodeData(const QByteArray &data)
{
    if (!mRunning) return;
    m_buffer.append(data);
    processBuffer();
}

void VideoDecoderThread::processBuffer()
{
    if (!mRunning) return;

    bool processedSomething = true;
    while (processedSomething) {
        processedSomething = false;

        switch (m_state) {
        case STATE_READING_DUMMY_BYTE: {
            if (!m_buffer.isEmpty()) {
                qDebug() << "[Decoder] State: Reading dummy byte.";
                // 源码显示dummy byte是0。如果不是0，可能是一些意外情况或者没有dummy byte。
                // 为了健壮性，无论是什么，我们都读一个字节并丢弃。
                m_buffer.remove(0, 1);
                m_state = STATE_READING_DEVICE_META;
                processedSomething = true;
            }
            break;
        }
        case STATE_READING_DEVICE_META: {
            if (m_buffer.size() >= 64) {
                qDebug() << "[Decoder] State: Reading device meta.";
                QByteArray metaData = m_buffer.left(64);
                QString deviceName = QString::fromUtf8(metaData).trimmed();
                qDebug() << "[Decoder] Device name:" << deviceName;

                emit deviceNameReady(deviceName); // 发送信号更新UI

                m_buffer.remove(0, 64);
                m_state = STATE_READING_VIDEO_HEADER;
                processedSomething = true;
            }
            break;
        }
        case STATE_READING_VIDEO_HEADER: {
            if (m_buffer.size() >= 12) {
                qDebug() << "[Decoder] State: Reading video header.";
                const uchar* header = reinterpret_cast<const uchar*>(m_buffer.constData());
                // quint32 codecId = read_be32(header); // 我们知道是h264，可以忽略
                quint32 width = read_be32(header + 4);
                quint32 height = read_be32(header + 8);

                qDebug() << "[Decoder] Video resolution:" << width << "x" << height;

                if (width == 0 || height == 0 || width > 8192 || height > 8192) {
                    emit decodingFinished(QString("错误: 接收到无效的分辨率(%1x%2)。").arg(width).arg(height));
                    stop();
                    return;
                }

                m_buffer.remove(0, 12);
                m_state = STATE_READING_PACKET_HEADER;
                processedSomething = true;
            }
            break;
        }
        case STATE_READING_PACKET_HEADER: {
            if (m_buffer.size() >= 12) {
                const uchar* header = reinterpret_cast<const uchar*>(m_buffer.constData());
                // PTS 是8字节，我们暂时不需要，直接读 4 字节的包大小
                m_payloadSize = read_be32(header + 8);

                if (m_payloadSize == 0) { // 配置帧，不是视频数据
                    m_buffer.remove(0, 12);
                    // 保持在 STATE_READING_PACKET_HEADER 状态，继续读下一个头
                    processedSomething = true;
                    continue;
                }

                m_buffer.remove(0, 12);
                m_state = STATE_READING_PACKET_PAYLOAD;
                processedSomething = true;
            }
            break;
        }
        case STATE_READING_PACKET_PAYLOAD: {
            if (m_payloadSize > 0 && (quint32)m_buffer.size() >= m_payloadSize) {

                // --- START OF FIX ---
                // 1. 创建一个新的Packet，让FFmpeg自己管理内存
                if (av_new_packet(m_packet, m_payloadSize) < 0) {
                    qWarning() << "[Decoder] Failed to create a new AVPacket.";
                    // 清理并重置状态，防止无限循环
                    m_buffer.remove(0, m_payloadSize);
                    m_state = STATE_READING_PACKET_HEADER;
                    m_payloadSize = 0;
                    processedSomething = true;
                    break; // 跳出 case
                }
                // 2. 将数据从 m_buffer 复制到新的 packet 中 (深拷贝)
                memcpy(m_packet->data, m_buffer.constData(), m_payloadSize);
                // --- END OF FIX ---
                // 现在 m_packet 拥有了数据的安全副本，可以继续后续操作
                if (avcodec_send_packet(m_codecContext, m_packet) == 0) {
                    while (avcodec_receive_frame(m_codecContext, m_frame) == 0) {
                        // ... (解码和转换图像的代码保持不变)
                        // 确保 SWS 上下文在分辨率变化时能重新创建
                        if (!m_swsContext || m_lastFrameWidth != m_frame->width || m_lastFrameHeight != m_frame->height) {
                            if (m_swsContext) {
                                sws_freeContext(m_swsContext);
                            }
                            m_swsContext = sws_getContext(m_frame->width, m_frame->height, (AVPixelFormat)m_frame->format,
                                                          m_frame->width, m_frame->height, AV_PIX_FMT_RGB32,
                                                          SWS_BILINEAR, nullptr, nullptr, nullptr);
                            m_lastFrameWidth = m_frame->width;
                            m_lastFrameHeight = m_frame->height;
                            if (!m_swsContext) {
                                emit decodingFinished("错误：无法初始化或更新图像转换上下文。");
                                stop();
                                return;
                            }
                        }

                        QImage image(m_frame->width, m_frame->height, QImage::Format_RGB32);
                        const int stride[] = { static_cast<int>(image.bytesPerLine()) };
                        uint8_t* dest[] = { image.bits() };
                        sws_scale(m_swsContext, m_frame->data, m_frame->linesize, 0, m_frame->height, dest, stride);
                        emit frameDecoded(image);
                    }
                }
                // av_packet_unref 必须调用，以释放 av_new_packet 分配的内存
                av_packet_unref(m_packet);
                m_buffer.remove(0, m_payloadSize);
                m_state = STATE_READING_PACKET_HEADER; // 回去读下一个包头
                m_payloadSize = 0;
                processedSomething = true;
            }
            break;
        }
        }
    }
}
