#include "screenrecorder.h"



ScreenRecorder::ScreenRecorder()
{
    mRecordAudio = 1;
}

void ScreenRecorder::run() {
    inputDeviceInit();
}

int ScreenRecorder::outputDeviceInit() {
    AVCodec *pH264Codec;
    char *out_file = "/Users/surecn/out.h264";
    int value;
    AVOutputFormat *pOutFormat = av_guess_format(NULL, out_file, NULL); //根据文件名确定文件输出格式
    if (!pOutFormat) {
        printf("Could not deduce output format from file extension: using MPEG.");
        return -1;
    }

    avformat_alloc_output_context2(&pH264FormatCtx, pOutFormat, NULL, NULL);
    //pH264FormatCtx = avformat_alloc_context();



    pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!pH264Codec) {
        printf("h264 codec not found");
        return -1;
    }

    AVStream *outStream = avformat_new_stream(pH264FormatCtx, NULL);
    if (!outStream) {
        printf("Alloc OutStream Error");
    }
    outStream->id = pH264FormatCtx->nb_streams - 1;

    pH264CodecCtx = avcodec_alloc_context3(pH264Codec);
    pH264CodecCtx->codec_id = AV_CODEC_ID_H264;
    pH264CodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pH264CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pH264CodecCtx->width = 1280;
    pH264CodecCtx->height = 720;
    pH264CodecCtx->time_base = (AVRational) {1, 25};
    pH264CodecCtx->bit_rate = 400000;
    pH264CodecCtx->gop_size = 3;
    pH264CodecCtx->max_b_frames = 2;

    pH264FormatCtx ->video_codec = pH264Codec;

//    if (pH264CodecCtx->flags & AVFMT_GLOBALHEADER) {
//        pH264CodecCtx->flags != AV_CODEC_FLAG_GLOBAL_HEADER;
//    }

//    AVDictionary *options = 0;
//    av_dict_set(&options, "preset", "superfast", 0);
//    av_dict_set(&options, "tune", "zerolatency", 0);
//    pH264Codec = avcodec_find_encoder(pH264CodecCtx->codec_id);
//    if (!pH264Codec) {
//        printf("h264 codec not found");
//    }

    if (avcodec_open2(pH264CodecCtx, pH264Codec, NULL) < 0) {
        printf("failed to open video encoder");
    }

    avcodec_parameters_from_context(outStream->codecpar, pH264CodecCtx);

    if (!(pH264FormatCtx->oformat->flags & AVFMT_NOFILE)) {
        value = avio_open(&pH264FormatCtx->pb, out_file, AVIO_FLAG_WRITE);
        if (value < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'. err: %s", out_file, av_err2str(value));
            return value;
        }
    }

    /* init muxer, write output file header */
    value = avformat_write_header(pH264FormatCtx, NULL);
    if (value < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file. error: %s\n", av_err2str(value));
        return value;
    }

//    AVFrame *picture = av_frame_alloc();
//    uint8_t *pictureBuff = (uint8_t *)av_malloc(avpicture_get_size(pH264CodecContext->pix_fmt, 1280, 720));
//    avpicture_fill()
}

int ScreenRecorder::inputDeviceInit() {
    AVDictionary *videoOptions;
    AVCodecContext *pVideoCodecCtx;
    AVCodecContext *pAudioCodecCtx;

    //avdevice注册
    avdevice_register_all();

    //获取AVFormatContext;
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVInputFormat *pInputFormat = av_find_input_format("avfoundation");

    //打开录屏设备配置
    av_dict_set(&videoOptions, "capture_cursor", "1", 0);//记录鼠标移动
    av_dict_set(&videoOptions, "capture_mouse_clicks", "1", 0);//记录鼠标点击事件
    av_dict_set(&videoOptions, "pixel_format", "uyvy422", 0);// 设置像素格式
    av_dict_set(&videoOptions, "framerate", "20", 0);// 设置帧率
    av_dict_set(&videoOptions, "video_size", "1280*720", 0);// 设置图像尺寸
    av_dict_set(&videoOptions, "vcodec", "copy", 0);

    //打开录屏和录音设备
    if (avformat_open_input(&pFormatCtx, "1", pInputFormat, &videoOptions)) {
        printf("Couldn't open device");
        return -1;
    }

    int videoStreamIndex = -1;
    int audioStreamIndex = -1;

    //搜索音视频流
    for(int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
//            AVCodec *dec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
//            AVCodecContext *_inCodecContext = NULL;
//            _inCodecContext = avcodec_alloc_context3(dec);
//            if (!_inCodecContext) {
//                printf("\nCan not find in stream codec");
//                exit(1);
//            }

//            int value = avcodec_parameters_to_context(_inCodecContext, pFormatCtx->streams[i]->codecpar);
//            if (value < 0) {
//                av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
//                                           "for stream #%u\n", i);
//                return value;
//            }

//            _inCodecContext->framerate = av_guess_frame_rate(inFormatContext, inFormatContext->streams[i], NULL);
//            //stream_ctx[0].dec_ctx = _inCodecContext;
        } else if (pFormatCtx->streams[i]->codecpar->codec_type = AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }
    if (videoStreamIndex == -1) {
        printf("Couldn't find video stream");
        return -1;
    }
//    if (mRecordAudio && audioStreamIndex == -1) {
//        printf("Couldn't find audio stream");
//        return -1;
//    }


//    pVideoCodecCtx = pFormatCtx->streams[videoStreamIndex]->codec;
//    if (audioStreamIndex >= 0) {
//        pAudioCodecCtx = pFormatCtx->streams[audioStreamIndex]->codec;
//    }
    //获取解码器
    AVCodec *pVideoCodec = avcodec_find_decoder(pFormatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    if (pVideoCodec == NULL) {
        printf("Codec not found");
        return -1;
    }

    pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
    pVideoCodecCtx->width = 1920;
        pVideoCodecCtx->height = 1080;
        pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YVYU422;

        AVDictionary *_options = NULL;

        av_dict_set(&_options, "pixel_format", "yuyv422", 0);

    //打开解码器
    if (avcodec_open2(pVideoCodecCtx, pVideoCodec, &_options) < 0) {
        printf("Could not open codec");
        return -1;
    }

//    int got_picture;
    AVPacket *packet = av_packet_alloc();
    AVPacket *packetOut = av_packet_alloc();

    outputDeviceInit();

    //申请空间来存放图片数据
    uint8_t* pictureBuff = (uint8_t*)av_malloc(av_image_get_buffer_size(
                                                   pH264CodecCtx->pix_fmt,
                                                   pH264CodecCtx->width,
                                                   pH264CodecCtx->height,
                                                   32));
    AVFrame* pFrame = av_frame_alloc();
    //创建解码后的帧
    AVFrame* pFrameYUV = av_frame_alloc();
    //avpicture_fill((AVPicture*)pFrameYUV, pictureBuff, AV_PIX_FMT_YUV420P, 1280, 720);
    av_image_fill_arrays(
                pFrameYUV->data,
                pFrameYUV->linesize,
                pictureBuff,
                AV_PIX_FMT_YUV420P,
                pH264CodecCtx->width,
                pH264CodecCtx->height,
                1);

    av_image_alloc(pFrameYUV->data, pFrameYUV->linesize, pH264CodecCtx->width, pH264CodecCtx->height, AV_PIX_FMT_YUV420P,
                       1);
    struct SwsContext *imgConvertContext;
    imgConvertContext = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt, 1280, 720, pH264CodecCtx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);



    while (1) {
        if (av_read_frame(pFormatCtx, packet) < 0) {
            msleep(10);
        }
        if (packet->stream_index == videoStreamIndex) {
            //avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, packet);
            //decodePacket(pVideoCodecCtx, packet, pFrame);
            int ret;
            ret = avcodec_send_packet(pVideoCodecCtx, packet);
            if (ret < 0) {
                printf("Error sending a packet for decoding");
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(pVideoCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    return 1;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    return -1;
                }

                sws_scale(imgConvertContext, pFrame->data, pFrame->linesize, 0, pVideoCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                pFrameYUV->format = AV_PIX_FMT_YUV420P;
                pFrameYUV->width = pH264CodecCtx->width;
                pFrameYUV->height = pH264CodecCtx->height;
                pFrameYUV->pts=10;
                int64_t now = av_gettime();
                const AVRational codecTimebase = (AVRational) {1, 25};
                pFrameYUV->pts = av_rescale_q(now, (AVRational) {1, 1000000}, codecTimebase);
                //encodeFrame(pH264CodecCtx, pFrameYUV, packetOut);

                //int got_picture2 = 0;
                //avcodec_encode_video2(pH264CodecCtx, packetOut, pFrameYUV, &got_picture2);

                ret = avcodec_send_frame(pH264CodecCtx, pFrameYUV);
                if (ret < 0) {
                    printf("Error sending a packet for decoding");
                }

                av_init_packet(packetOut);
                packetOut->data = NULL;    // packet data will be allocated by the encoder
                packetOut->size = 0;
                while (ret >= 0) {
                    ret = avcodec_receive_packet(pH264CodecCtx, packetOut);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        return -1;
                    }

                    if (packetOut->pts != AV_NOPTS_VALUE)
                           packetOut->pts = av_rescale_q(packetOut->pts, pH264CodecCtx->time_base, pH264CodecCtx->time_base);

                    av_interleaved_write_frame(pH264FormatCtx, packetOut);
                }


            }
        }
    }


}

int ScreenRecorder::decodePacket(AVCodecContext *codecCtx, AVPacket *in, AVFrame *out) {
    int ret = -1;
    ret = avcodec_send_packet(codecCtx, in);
    if (ret < 0) {
        printf("Error sending a packet for decoding");
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codecCtx, out);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 1;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return -1;
        }
        printf("saving frame %3d\n", codecCtx->frame_number);
    }
}

int ScreenRecorder::encodeFrame(AVCodecContext *codecCtx, AVFrame *in, AVPacket *out) {
    int ret = -1;
    ret = avcodec_send_frame(codecCtx, in);
    if (ret < 0) {
        printf("Error sending a packet for decoding");
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, out);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 1;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        printf("saving frame %3d\n", codecCtx->frame_number);
    }
}
