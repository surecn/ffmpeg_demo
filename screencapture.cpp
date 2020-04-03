#include "screencapture.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
}

ScreenCapture::ScreenCapture()
{

}

void ScreenCapture::run() {
    AVCodecContext *pCodecContext;
    AVCodec *pCodec;
      AVDictionary *options;
    //第一步初始化ffmpeg
    av_register_all();
   avcodec_register_all();
   avdevice_register_all();



    //第二步获取AVFormatContext;
    AVFormatContext *pFormatContext = avformat_alloc_context();

//    //第三部打开设备


    //AVInputFormat *pInputFormat=av_find_input_format("avfoundation");


    //打开摄像头，同时需要配置Info.plist文件
    //av_dict_set(&options, "framerate", "30", 0);
    //av_dict_set(&options, "video_size", "1280x720", 0);
    //av_dict_set(&options, "pixel_format", "uyvy422", 0);
    //if (avformat_open_input(&pFormatContext, "0", pInputFormat, &options) < 0) {

    //录屏配置
    av_dict_set(&options, "capture_cursor", "1", 0);
    av_dict_set(&options, "capture_mouse_clicks", "1", 0);
    av_dict_set(&options, "pixel_format", "uyvy422", 0);
    av_dict_set(&options, "formats", "uyvy422", 0);
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options, "vcodec", "copy", 0);
    AVInputFormat *pInputFormat = av_find_input_format("avfoundation");
    if (avformat_open_input(&pFormatContext, "1", pInputFormat, &options) < 0)  {
        printf("Couldn't open device");
        return;
    }
    //打开麦克风
    //avformat_open_input(&pFormatContext, "audio=Built-in Microphone", pInputFormat, NULL);
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        printf("Couldn't find stream information");
        return;
    }

    //搜索视频流
    int videoIndex = -1;
    for(int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        }
    }
    if (videoIndex == -1) {
        printf("Couldn't find a video stream");
        return;
    }

    //获取解码器
    pCodecContext = pFormatContext->streams[videoIndex]->codec;
    pCodec = avcodec_find_decoder(pCodecContext->codec_id);
    if (pCodec == NULL) {
        printf("Codec not found");
        return;
    }

    //打开解码器
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        printf("Could not open codec");
        return;
    }

    FILE *file_yuv = fopen("/Users/surecn/output.yuv", "wb");
    int got_picture;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    AVFrame *pFrameYUV = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecContext->width, pCodecContext->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecContext->width, pCodecContext->height);

    struct SwsContext *img_convert_context;
    img_convert_context = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt, pCodecContext->width, pCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    while(1) {
        if (av_read_frame(pFormatContext, &packet) < 0) {
            break;
        }
        if (packet.stream_index == videoIndex) {
            avcodec_decode_video2(pCodecContext, frame, &got_picture, &packet);
            if (got_picture) {
                sws_scale(img_convert_context, (const uint8_t* const*)frame->data, frame->linesize, 0, pCodecContext->height, pFrameYUV->data, pFrameYUV->linesize);

                int y_size = pCodecContext->width * pCodecContext->height;
                fwrite(pFrameYUV->data[0], 1, y_size, file_yuv);
                fwrite(pFrameYUV->data[1], 1, y_size / 4, file_yuv);
                fwrite(pFrameYUV->data[2], 1, y_size / 4, file_yuv);
            }
        }
        av_free_packet(&packet);
    }
}
