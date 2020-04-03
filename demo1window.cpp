#include "demo1window.h"
#include "ui_demo1window.h"
#include <QTextStream>
#include <QFileDialog>
#include <stdio.h>
extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libswscale/swscale.h"
}

Demo1Window::Demo1Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Demo1Window)
{
    ui->setupUi(this);
}

Demo1Window::~Demo1Window()
{
    delete ui;
}

///现在我们需要做的是让SaveFrame函数能把RGB信息定稿到一个PPM格式的文件中。
///我们将生成一个简单的PPM格式文件，请相信，它是可以工作的。
void SaveFrame(AVFrame *pFrame, int width, int height,int index)
{

  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "frame%d.ppm", index);
  pFile=fopen(szFilename, "wb");

  if(pFile==NULL)
    return;

  // Write pixel data
  for(y=0; y<height; y++)
  {
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  }

  // Close file
  fclose(pFile);

}

//第一节测试
void test1() {
    AVCodecContext *pCodecCtx;
    AVFormatContext *pFormatCtx;
    AVCodec *pCodec;
    static struct SwsContext *img_convert_ctx;
    AVFrame *pFrame, *pFrameRGB;
    int numBytes;
    uint8_t *out_buffer;

    //第一步初始化ffmpeg  使用这个函数完成编码器和解码器的初始化，只有初始化了编码器和解码器才能正常使用，否则会在打开编解码器的时候失败。
    av_register_all();

    //第二步， 接着需要分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
    pFormatCtx = avformat_alloc_context();

    //char *path = ui->textEdit->toPlainText().toLatin1().data();
    QTextStream cout(stdout);
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

    int ret, got_picture;

    //第三步， 循环查询其中的流信息，直到找到视频相关的流
    for(int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }
    }

    //如果videoStream为-1 说明没有找到视频流
    if (videoStream == -1) {

        cout<<"Didn't find a video stream."<<endl;
        return;
    }

    //第四步, 查找解码器
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL) {
        cout<<"Codec not found."<<endl;
        return;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        cout<<"Could not open codec."<<endl;
        return;
    }

    cout<<"Open Success"<<endl;


    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();


    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

//    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    //第五步，读取视频
    int y_size = pCodecCtx->width * pCodecCtx->height;
    AVPacket *packet = (AVPacket*)malloc(sizeof (AVPacket));
    av_new_packet(packet, y_size);

    av_dump_format(pFormatCtx, 0, file_path, 0);

    int index = 0;
    while (1) {
        if (av_read_frame(pFormatCtx, packet) < 0) {
            break;//这里认为视频读取完毕
        }

        if (packet->stream_index == videoStream) {
            //视频里面的数据是经过编码压缩的，因此这里我们需要将其解码：
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                cout << "decode error" << endl;
                return;
            }

            if (got_picture) {
                //基本上所有解码器解码之后得到的图像数据都是YUV420的格式，而这里我们需要将其保存成图片文件，因此需要将得到的YUV420数据转换成RGB格式，转换格式也是直接使用FFMPEG来完成：
                sws_scale(img_convert_ctx,
                                        (uint8_t const * const *) pFrame->data,
                                        pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                                        pFrameRGB->linesize);
                //得到RGB数据之后就是直接写入文件了
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, index++);//图片保存
            }
        }
        av_free_packet(packet);
        if(index > 50) return;
    }
    av_free(out_buffer);
    av_free(pFrameRGB);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);


//    //解码
//    if (packet->stream_index == videoStream) {
//        ret = avcodec_decode_video2(pCodecCtx, pFrame, )
//    }
}


void Demo1Window::on_pushButton_clicked()
{
    QTextStream cout(stdout);
    QString qstring = QFileDialog::getOpenFileName(this, "选择文件", nullptr, nullptr, nullptr, nullptr);
    ui->textEdit->setText(qstring);
}

void Demo1Window::on_pushButton_2_clicked()
{
    test1();
}
