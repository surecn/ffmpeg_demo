
#include "videoplayer.h"

#include <sys/stat.h>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIOQ_SIZE (1 * 1024 * 1024)
#define MAX_AUDIO_FRAME_SIZE 192000
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
//https://andy-zhangtao.github.io/ffmpeg-examples/screenrecord-h264.html
VideoPlayer::VideoPlayer(ImageLoader *loader) : imageLoader(loader)
{
    audio_frame = av_frame_alloc();
}

VideoPlayer::~VideoPlayer() {

}


void VideoPlayer::run() {
        //int parse_tid = SDL_CreateThread(, "decode", NULL);
    decode_thread();
}

int VideoPlayer::quit = 0;

int VideoPlayer::decodeVideoThread(void *userdata) {
    VideoPlayer *videoPlayer = (VideoPlayer *)userdata;
    videoPlayer->decodeVideo();
    return 0;
}

void VideoPlayer::decodeVideo() {
    AVPacket packet;
    static struct SwsContext *img_convert_ctx;
    AVFrame *pFrame, *pFrameRGB;
    int numBytes;
    double video_pts;
    uint8_t *out_buffer;
    int got_picture;
    int ret;
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
                pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height,
                AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);

    long mVideoStartTime = av_gettime();
    double time;

    while (true) {

        if (quit) return;
        if (mVideoQueue.packet_queue_get(&packet, 1) < 0)  {//get next packet
            mConditionVideo.wait(&mMutexVideo);
            continue;
        }

        while (true) {
            if (packet.dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE)
            {
                video_pts = *(uint64_t *) pFrame->opaque;
            }
            else if (packet.dts != AV_NOPTS_VALUE)
            {
                video_pts = packet.dts;
            }
            else
            {
                video_pts = 0;
            }
            video_pts *= av_q2d(pVideoStream->time_base);

            time = (av_gettime() - mVideoStartTime) / 1000000.0;
            if (video_pts <= time) break;

            int delayTime = (video_pts - time) * 1000;

            //delayTime = delayTime > 5 ? 5:delayTime;

            msleep(delayTime);
        }
        printf("%d==pts:%f==dts:%d==offset:%d==current:%d==start:%d\n", pFrame->opaque, video_pts, packet.dts,  time, av_gettime(), mVideoStartTime);
        //视频里面的数据是经过编码压缩的，因此这里我们需要将其解码：
        ret = avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, &packet);
        if (ret < 0) {
            printf("decode error");
            break;
        }

        if (got_picture) {
            //基本上所有解码器解码之后得到的图像数据都是YUV420的格式，而这里我们需要将其保存成图片文件，因此需要将得到的YUV420数据转换成RGB格式，转换格式也是直接使用FFMPEG来完成：
            sws_scale(img_convert_ctx,
                                    (uint8_t const * const *) pFrame->data,
                                    pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data,
                                    pFrameRGB->linesize);
//                //得到RGB数据之后就是直接写入文件了
//                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index++);//图片保存
            QImage tmpImage((uchar *)out_buffer, pVideoCodecCtx->width,  pVideoCodecCtx->height, QImage::Format_RGB32);
            QImage image = tmpImage.copy();
//                imageLoader->load(image);

            emit sig_GetOneFrame(image);
        }
        av_free_packet(&packet);
    }
}


void VideoPlayer::audioCallback(void *userdata, Uint8 *stream, int len) {
    VideoPlayer *videoPlayer = (VideoPlayer *)userdata;
    videoPlayer->readAudioData(stream, len);
}

void VideoPlayer::readAudioData(Uint8 *stream, int len) {
    int len1, audio_data_size;
    //printf("audio callback 1 len=%d\n",len);

    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            audio_data_size = audio_decode_frame();//decode one frame,return size
            if(audio_data_size < 0) {
                /* silence */
                audio_buf_size = 1024;
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_data_size;
            }
            audio_buf_index = 0;
            //printf("audio callback 2 (audio_buf_size,audio_buf_index) = (%d,%d)\n",audio_buf_size, audio_buf_index);
        }

        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        //将解码后的音频数据发给SDL
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
        //printf("audio callback 3 (len1,len,audio_buf_index) = (%d,%d,%d)\n",len1,len,audio_buf_index);
    }
}

int VideoPlayer::audio_decode_frame() {//core code   decoded information is saved in videostate-is
    int len1, len2, decoded_data_size;
    int got_frame = 0;
    int64_t dec_channel_layout;
    int wanted_nb_samples, resampled_data_size;
    AVPacket *pkt = &audio_pkt;
    for (;;) {//dead loop
        while (audio_pkt_size > 0) {
            len1 = avcodec_decode_audio4(pAudioCodecCtx, audio_frame, &got_frame,  pkt);
            if (len1 < 0) {
                // error, skip the frame
                audio_pkt_size = 0;
                break;
            }

            audio_pkt_size -= len1;

            if (!got_frame)
                continue;

           /* decoded_data_size = av_samples_get_buffer_size(NULL,
                                is->audio_frame->channels,
                               is->audio_frame->nb_samples,
                                is->audio_frame->format, 1);*/
            decoded_data_size = av_samples_get_buffer_size(NULL,
                pAudioCodecCtx->channels,
                audio_frame->nb_samples,
                AVSampleFormat(audio_frame->format), 1);//get decoded_data_size

          /*  dec_channel_layout = (is->audio_frame->channel_layout && is->audio_frame->channels
                                  == av_get_channel_layout_nb_channels(is->audio_frame->channel_layout))
                                 ? is->audio_frame->channel_layout
                                 : av_get_default_channel_layout(is->audio_frame->channels);*/

            dec_channel_layout = (pAudioCodecCtx->channel_layout && pAudioCodecCtx->channels
                == av_get_channel_layout_nb_channels(pAudioCodecCtx->channel_layout))
                ? pAudioCodecCtx->channel_layout
                : av_get_default_channel_layout(pAudioCodecCtx->channels);

            wanted_nb_samples =  audio_frame->nb_samples;

            //fprintf(stderr, "wanted_nb_samples = %d\n", wanted_nb_samples);
            //解码出来的音频与原先设定的格式不一致,则重采样
            if (audio_frame->format != audio_src_fmt ||
                dec_channel_layout != audio_src_channel_layout ||
                pAudioCodecCtx->sample_rate != audio_src_freq ||
                (wanted_nb_samples != audio_frame->nb_samples && !swr_ctx)) {
                if (swr_ctx) swr_free(&swr_ctx);
                swr_ctx = swr_alloc_set_opts(NULL, //get swr_ctx
                                                 audio_tgt_channel_layout,
                                                 audio_tgt_fmt,
                                                 audio_tgt_freq,
                                                 dec_channel_layout,
                                                 AVSampleFormat(audio_frame->format),
                                                 pAudioCodecCtx->sample_rate,
                                                 0, NULL);
                if (!swr_ctx || swr_init(swr_ctx) < 0) {//init
                    fprintf(stderr, "swr_init() failed\n");
                    break;
                }//get src parameters
                audio_src_channel_layout = dec_channel_layout;
                audio_src_channels = pAudioCodecCtx->channels;
                audio_src_freq = pAudioCodecCtx->sample_rate;
                audio_src_fmt = pAudioCodecCtx->sample_fmt;
            }

            if (swr_ctx) {
               // const uint8_t *in[] = { is->audio_frame->data[0] };
                const uint8_t **in = (const uint8_t **)audio_frame->extended_data;
                uint8_t *out[] = { audio_buf2 };
//                if (wanted_nb_samples != audio_frame->nb_samples) {//compensate samples
//                     swr_compensate(swr_ctx, (wanted_nb_samples - audio_frame->nb_samples)
//                                                 * audio_tgt_freq / pAudioCodecCtx->sample_rate,
//                                                 wanted_nb_samples * audio_tgt_freq / pAudioCodecCtx->sample_rate);
//                 }

                len2 = swr_convert(swr_ctx, out,// in data is changed by swr_ctx
                                   sizeof(audio_buf2)
                                   / audio_tgt_channels
                                   / av_get_bytes_per_sample(audio_tgt_fmt),//tgt_nb_samples
                                   in, audio_frame->nb_samples);//convert
                if (len2 < 0) {
                    fprintf(stderr, "swr_convert() failed\n");
                    break;
                }
                if (len2 == sizeof(audio_buf2) / audio_tgt_channels / av_get_bytes_per_sample(audio_tgt_fmt)) {
                    fprintf(stderr, "warning: audio buffer is probably too small\n");
                    swr_init(swr_ctx);
                }
                audio_buf = audio_buf2;//audio_buff
                resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(audio_tgt_fmt);//resampled_data_size
            } else {
                resampled_data_size = decoded_data_size;//not resampled
                audio_buf = audio_frame->data[0];
            }
            // We have data, return it and come back for more later
            return resampled_data_size;  //返回重采样后的长度
        }//decode one frame

        if (pkt->data) av_free_packet(pkt);
        memset(pkt, 0, sizeof(*pkt));
        if (quit) return -1;
        if (mAudioQueue.packet_queue_get(pkt, 1) < 0) return -1;//get next packet

        audio_pkt_size = pkt->size;
    }
}


int VideoPlayer::openAudioStream(int stream_index) {//open stream
    SDL_AudioSpec wanted_spec, spec;
    int64_t wanted_channel_layout = 0;
    int wanted_nb_channels;
    const int next_nb_channels[] = {0, 0, 1 ,6, 2, 6, 4, 6};

    if (stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
        return -1;
    }

    wanted_nb_channels = pAudioCodecCtx->channels;//wanted parameters
    if(!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    wanted_spec.channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.freq = pAudioCodecCtx->sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        fprintf(stderr, "Invalid sample rate or channel count!\n");
        return -1;
    }
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audioCallback;//callback
    wanted_spec.userdata = this;

    if (SDL_Init(SDL_INIT_AUDIO)) { //初始化音频SDL
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }
    while(SDL_OpenAudio(&wanted_spec, &spec) < 0) {//OpenAudio
        fprintf(stderr, "SDL_OpenAudio (%d channels): %s\n", wanted_spec.channels, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if(!wanted_spec.channels) {
            fprintf(stderr, "No more channel combinations to tyu, audio open failed\n");
            return -1;
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }

    if (spec.format != AUDIO_S16SYS) {
        fprintf(stderr, "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            fprintf(stderr, "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }

    fprintf(stderr, "%d: wanted_spec.format = %d\n", __LINE__, wanted_spec.format);
    fprintf(stderr, "%d: wanted_spec.samples = %d\n", __LINE__, wanted_spec.samples);
    fprintf(stderr, "%d: wanted_spec.channels = %d\n", __LINE__, wanted_spec.channels);
    fprintf(stderr, "%d: wanted_spec.freq = %d\n", __LINE__, wanted_spec.freq);

    fprintf(stderr, "%d: spec.format = %d\n", __LINE__, spec.format);
    fprintf(stderr, "%d: spec.samples = %d\n", __LINE__, spec.samples);
    fprintf(stderr, "%d: spec.channels = %d\n", __LINE__, spec.channels);
    fprintf(stderr, "%d: spec.freq = %d\n", __LINE__, spec.freq);

    //设置采样格式
    audio_src_fmt = audio_tgt_fmt = AV_SAMPLE_FMT_S16;//src parameters
    audio_src_freq = audio_tgt_freq = spec.freq;
    //设置的声道布局
    audio_src_channel_layout = audio_tgt_channel_layout = wanted_channel_layout;
    audio_src_channels = audio_tgt_channels = spec.channels;

//    if (!pAudioCodec || (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0)) {//Unsupported codec
//        fprintf(stderr, "Unsupported codec!\n");
//        return -1;
//    }

    pFormatCtx->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    audio_buf_size = 0;
    audio_buf_index = 0;
    memset(&audio_pkt, 0, sizeof(audio_pkt));
    SDL_PauseAudio(0);
}

int VideoPlayer::decode_thread() {
    //初始化参数，函数内部处理得到的相关参数赋给 is
    AVPacket *packet;
    int ret, i, audioStreamIndex = -1, videoStreamIndex = -1;


    //第一步初始化ffmpeg  使用这个函数完成编码器和解码器的初始化，只有初始化了编码器和解码器才能正常使用，否则会在打开编解码器的时候失败。
    av_register_all();

    //第二步， 接着需要分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
    pFormatCtx = avformat_alloc_context();

    //第三步， 接着调用打开视频文件
    if (avformat_open_input(&pFormatCtx, "/Users/surecn/Movies/my.mp4", NULL, NULL) != 0) {
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {//打开流信息
        return -1;
    }

    for (i=0; i<pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && audioStreamIndex < 0) {//打开第一条音频流
            audioStreamIndex=i;
            break;
        } else if (pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        }
    }

    pVideoStream = pFormatCtx->streams[videoStreamIndex];

    //第四步, 查找视频解码器
    pVideoCodecCtx = pFormatCtx->streams[videoStreamIndex]->codec;
    pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);

    //查找音频解码器
    pAudioCodecCtx = pFormatCtx->streams[audioStreamIndex]->codec;
    pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);

    if (pVideoCodec == NULL) {
        printf("Video Codec not found.");
        return -1;
    }

    if (pAudioCodec == NULL) {
        printf("Audio Codec not found.");
        return -1;
    }

    if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
        printf("Could not open codec.");
        return -1;
    }

    if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0) {
        printf("Could not open codec.");
        return -1;
    }

    if (audioStreamIndex >= 0) {
        //配置音频解码相关
        openAudioStream(audioStreamIndex);
    }

    if (videoStreamIndex >= 0) {
        //启动线程解码视频数据
        SDL_CreateThread(VideoPlayer::decodeVideoThread, "decodeVideo", this);
    }

    // main decode loop
    for(;;) {
        if(quit) break;
        if (mAudioQueue.size > MAX_AUDIOQ_SIZE) {
            SDL_Delay(10);//so fast
            continue;
        }
        packet = new AVPacket;
        ret = av_read_frame(pFormatCtx, packet);//read data to one packet
        if (ret < 0) {
            if(ret == AVERROR_EOF) {//error or end
                break;
            }
            if(pFormatCtx->pb && pFormatCtx->pb->error) {
                break;
            }
            continue;
        }

        if (packet->stream_index == audioStreamIndex) {//packet data to audioq
            mAudioQueue.packet_queue_put(packet);
        } else if (packet->stream_index == videoStreamIndex) {
            mVideoQueue.packet_queue_put(packet);
            mConditionVideo.wakeAll();
        } else {
            av_free_packet(packet);
        }
    }

    while (!quit) {// delay
        SDL_Delay(100);
    }

fail: {//if  fail
        SDL_Event event;
        event.type = FF_QUIT_EVENT;
        event.user.data1 = NULL;
        SDL_PushEvent(&event);
    }

    return 0;
}

//第一节测试
void VideoPlayer::test() {
    AVCodec *pVideoCodec, *pAudioCodec;
    static struct SwsContext *img_convert_ctx;
    static struct SwrContext *au_convert_ctx;
    int64_t in_channel_layout;
    AVFrame *pFrame, *pFrameRGB;
    int numBytes;
    uint8_t *out_buffer;

    //第一步初始化ffmpeg  使用这个函数完成编码器和解码器的初始化，只有初始化了编码器和解码器才能正常使用，否则会在打开编解码器的时候失败。
    av_register_all();

    //第二步， 接着需要分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
    pFormatCtx = avformat_alloc_context();

    //char *path = ui->textEdit->toPlainText().toLatin1().data();

    //cout<<*path<<endl;
    //第三步， 接着调用打开视频文件
    char *file_path = "/Users/surecn/Movies/my.mp4";
    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
            printf("can't open the file.");
            return;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
            printf("Could't find stream infomation.");
            return;
    }

    int videoStream = -1;
    int audioStream = -1;

    int ret, got_picture;

    //第三步， 循环查询其中的流信息，直到找到视频相关的流
    for(int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        } else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
        }
    }

    //如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {
        printf("Didn't find a video stream.");
        return;
    }

    if (audioStream == -1) {
        printf("Didn't find a audio stream.");
        return;
    }

    //第四步, 查找视频解码器
    pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);

    //查找音频解码器
    pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;
    pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);

    if (pVideoCodec == NULL) {
        printf("Video Codec not found.");
        return;
    }

    if (pAudioCodec == NULL) {
        printf("Audio Codec not found.");
        return;
    }

    if (avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0) {
        printf("Could not open codec.");
        return;
    }

    if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0) {
        printf("Could not open codec.");
        return;
    }

    SDL_AudioSpec m_sdlAudioSpec, spec;
    int wanted_nb_channels;
    int64_t wanted_channel_layout = 0;
    const int next_nb_channels[] = {0, 0, 1 ,6, 2, 6, 4, 6};

    if(!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
            wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
            wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
        }

//    m_sdlAudioSpec.channels = av_get_channel_layout_nb_channels(wanted_channel_layout);

    m_sdlAudioSpec.freq = pAudioCodecCtx->sample_rate; //根据你录制的PCM采样率决定
    m_sdlAudioSpec.format = AUDIO_S16SYS;
    m_sdlAudioSpec.channels = pAudioCodecCtx->channels;
    m_sdlAudioSpec.silence = 0;
    m_sdlAudioSpec.samples = SDL_AUDIO_BUFFER_SIZE;
    m_sdlAudioSpec.callback = VideoPlayer::audioCallback;
    m_sdlAudioSpec.userdata = this;

    //Out Audio Param
        uint64_t out_channel_layout=AV_CH_LAYOUT_STEREO;
        //nb_samples: AAC-1024 MP3-1152
        int out_nb_samples=pAudioCodecCtx->frame_size;
        audio_src_fmt = audio_tgt_fmt = AV_SAMPLE_FMT_S16;
        int out_sample_rate=44100;
        int out_channels=av_get_channel_layout_nb_channels(out_channel_layout);
        //Out Buffer Size
        int out_buffer_size=av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,audio_src_fmt, 1);

    int re = SDL_OpenAudio(&m_sdlAudioSpec, NULL);

    audio_buf_size = 0;
    audio_buf_index = 0;
    //in_channel_layout=av_get_default_channel_layout(pAudioCodecCtx->channels);

        //Swr

        swr_ctx = swr_alloc();
        swr_ctx=swr_alloc_set_opts(swr_ctx,out_channel_layout, audio_src_fmt, out_sample_rate,
            in_channel_layout,pAudioCodecCtx->sample_fmt , pAudioCodecCtx->sample_rate,0, NULL);
        swr_init(swr_ctx);

        audio_src_channel_layout = audio_tgt_channel_layout = m_sdlAudioSpec.channels;
        audio_src_freq = audio_tgt_freq = m_sdlAudioSpec.freq;
        audio_src_channels = audio_tgt_channels = m_sdlAudioSpec.channels;

    if (re < 0)
    {
        //std::cout << "can't open audio: " << GetErrorInfo(re);
    }
    else
    {
        //Start play audio
        SDL_PauseAudio(0);
    }

    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    img_convert_ctx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
                pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height,
                AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);

    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB32, pVideoCodecCtx->width, pVideoCodecCtx->height);

    //第五步，读取视频
    int y_size = pVideoCodecCtx->width * pVideoCodecCtx->height;


    AVPacket *packet = (AVPacket*)malloc(sizeof (AVPacket));
//    av_new_packet(packet, y_size);
//    av_new_packet(packet, 10);

    //av_dump_format(pFormatCtx, 0, file_path, 0);

    int index = 0;
    while (1) {
        if (mAudioQueue.size > MAX_AUDIOQ_SIZE) {
                    SDL_Delay(10);//so fast
                    continue;
                }
        if (av_read_frame(pFormatCtx, packet) < 0) {
            break;//这里认为视频读取完毕
        }

        if (ret < 0) {
                    if(ret == AVERROR_EOF) {//error or end
                        break;
                    }
                    if(pFormatCtx->pb && pFormatCtx->pb->error) {
                        break;
                    }
                    continue;
                }
        if (packet->stream_index == videoStream) {
            //视频里面的数据是经过编码压缩的，因此这里我们需要将其解码：
            ret = avcodec_decode_video2(pVideoCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                printf("decode error");
                return;
            }

            if (got_picture) {
                //基本上所有解码器解码之后得到的图像数据都是YUV420的格式，而这里我们需要将其保存成图片文件，因此需要将得到的YUV420数据转换成RGB格式，转换格式也是直接使用FFMPEG来完成：
                sws_scale(img_convert_ctx,
                                        (uint8_t const * const *) pFrame->data,
                                        pFrame->linesize, 0, pVideoCodecCtx->height, pFrameRGB->data,
                                        pFrameRGB->linesize);
//                //得到RGB数据之后就是直接写入文件了
//                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index++);//图片保存
                QImage tmpImage((uchar *)out_buffer, pVideoCodecCtx->width,  pVideoCodecCtx->height, QImage::Format_RGB32);
                QImage image = tmpImage.copy();
//                imageLoader->load(image);

                emit sig_GetOneFrame(image);
            }
            av_free_packet(packet);
        } else if(packet->stream_index==audioStream) {
            mAudioQueue.packet_queue_put(packet);
        } else {
            av_free_packet(packet);
        }


        //if(index > 50) return;
    }
    msleep(10000);
    av_free(out_buffer);
    av_free(pFrameRGB);
    avcodec_close(pVideoCodecCtx);
    avformat_close_input(&pFormatCtx);


//    //解码
//    if (packet->stream_index == videoStream) {
//        ret = avcodec_decode_video2(pCodecCtx, pFrame, )
//    }
}




