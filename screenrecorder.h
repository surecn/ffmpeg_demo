#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <QThread>

extern "C" {
    #include <libavdevice/avdevice.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
#include "libavutil/time.h"
#include "libavutil/imgutils.h"
}

class ScreenRecorder : public QThread
{
public:
    ScreenRecorder();

    int inputDeviceInit();

    int decodePacket(AVCodecContext *codecCtx, AVPacket *in, AVFrame *out);

    int encodeFrame(AVCodecContext *codecCtx, AVFrame *in, AVPacket *out);

    int outputDeviceInit();

    virtual void run() override;

private:
    int mRecordAudio;


    AVCodecContext *pH264CodecCtx;
    AVFormatContext *pH264FormatCtx;
};

#endif // SCREENRECORDER_H
