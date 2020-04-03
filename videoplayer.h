#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H
#include "imageloader.h"

#include <QThread>
#include "packetqueue.h"
#include <stdio.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avstring.h>
#include <libavutil/pixfmt.h>
#include <libavutil/log.h>
#include <libavutil/time.h>
#include <QWaitCondition>
    #include "SDL2/SDL.h"
#include <SDL2/SDL_audio.h>
}
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

class VideoPlayer : public QThread
{
    Q_OBJECT
public:
    VideoPlayer(ImageLoader *loader);
    ~VideoPlayer();

protected:
    static void audioCallback(void *userdata, Uint8 *stream, int len);
    static int decodeVideoThread(void *userdata);

    void run();
    void test();
    void readAudioData(Uint8 *stream, int len);
    int audio_decode_frame();
    int decode_thread();
    int openAudioStream(int stream_index);
    void decodeVideo();
    static int quit;

private:
    ImageLoader *imageLoader;

    AVFormatContext *pFormatCtx;

    AVCodecContext *pAudioCodecCtx;
    AVCodecContext *pVideoCodecCtx;

    AVCodec *pAudioCodec;
    AVCodec *pVideoCodec;

    AVStream *pVideoStream;

    struct SwrContext *swr_ctx;

    PacketQueue mAudioQueue;
    PacketQueue mVideoQueue;

    AVFrame         *audio_frame;

    int audio_pkt_size;
    uint8_t         *audio_pkt_data;

    //采样格式
    enum AVSampleFormat  audio_src_fmt;
    enum AVSampleFormat  audio_tgt_fmt;

    //音频通道数
    int             audio_src_channels;
    int             audio_tgt_channels;

    //声道布局
    int64_t         audio_src_channel_layout;
    int64_t         audio_tgt_channel_layout;

    int             audio_src_freq;
    int             audio_tgt_freq;

    uint8_t         *audio_buf;
    uint8_t         *audio_buf1;
    DECLARE_ALIGNED(16,uint8_t,audio_buf2)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

    unsigned int    audio_buf_size;
    unsigned int    audio_buf_index;
    AVPacket        audio_pkt;

    QWaitCondition mConditionVideo;
    QMutex mMutexVideo;
signals:
    void sig_GetOneFrame(QImage);
};


#endif // VIDEOPLAYER_H
