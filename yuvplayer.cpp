#include "yuvplayer.h"

YUVPlayer::YUVPlayer()
{
    quit = 1;
    file = NULL;
    mConditionVideo = new QWaitCondition;
    mMutexVideo = new QMutex;
}

void YUVPlayer::setFile(char *_file, int _width, int _height, int _framerage) {
    file = _file;
    width = _width;
    height = _height;
    framerate = _framerage;
    mConditionVideo->wakeAll();
}

void YUVPlayer::run() {
    while (quit) {
        if (file == NULL) {
            mMutexVideo->lock();
            mConditionVideo->wait(mMutexVideo);
            mMutexVideo->unlock();
            continue;
        }
        process();
    }
}

void YUVPlayer::setQuit(int _quit) {
    quit = _quit;
}

void YUVPlayer::process() {
    FILE *file_yuv = fopen(file, "rb");
    int yuvSize = width * height * 3 / 2;
    int rgbSize = width * height * sizeof(RGB32);

    BYTE *yuvBuffer = (BYTE*)malloc(yuvSize);
    BYTE *rgbBuffer = (BYTE*)malloc(rgbSize);

    while (true) {
        fread(yuvBuffer, 1, yuvSize, file_yuv);
        yuv420pToRgb32(yuvBuffer, rgbBuffer, width, height);
//        QImage image((uchar *)rgbBuffer, width, height,  QImage::Format_RGB32, nullptr, nullptr);

        /*=====rgbBuffer=======*/
//        printf("==========rgb==================");
//        for (int y = 0; y < 1; y++)
//        {
//            for (int x = 0; x < width; x++)
//            {
//                printf("%d ", yuvBuffer + (y * width + x));
//            }
//            printf("\n");
//        }
//        printf("==========image==================");
//        for (int y = 0; y < 1; y++)
//        {
//            for (int x = 0; x < width; x++)
//            {
//                printf("%d ", tmpImage.bits() + (y * width + x));
//            }
//            printf("\n");
//        }

        QImage tmpImage1(rgbBuffer, width, height, QImage::Format_RGB32);
        QImage image = tmpImage1.copy();
        emit sig_GetOneFrame(image);
        msleep(30);
    }
    file = NULL;
}

void YUVPlayer::yuv420pToRgb32(const BYTE *yuvBufferIn, const BYTE *rgbBufferOut, int width, int height) {
    BYTE *yuvBuffer = (BYTE *)yuvBufferIn;
    RGB32 *rgbBuffer = (RGB32*)rgbBufferOut;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;

            int indexY = index;
            //int indexU = width * height + index / 4;
            //int indexV = width * height * 5 / 4 + index / 4;
            int indexU = width * height + y / 2 * width / 2 + x / 2;
            int indexV = width * height + width * height / 4 + y / 2 * width / 2 + x / 2;

            BYTE Y = yuvBuffer[indexY];
            BYTE U = yuvBuffer[indexU];
            BYTE V = yuvBuffer[indexV];
            RGB32 *rgbNode = &rgbBuffer[index];

            rgbNode->red = Y + 1.402 * (V-128);
            rgbNode->green = Y - 0.34413 * (U-128) - 0.71414*(V-128);
            rgbNode->blue = Y + 1.772*(U-128);
            rgbNode->alpha = 255;
        }
    }

//    BYTE *yuvBuffer = (BYTE *)yuvBufferIn;
//        RGB32 *rgb32Buffer = (RGB32 *)rgbBufferOut;

//        for (int y = 0; y < height; y++)
//        {
//            for (int x = 0; x < width; x++)
//            {
//                int index = y * width + x;

//                int indexY = y * width + x;
//                int indexU = width * height + y / 2 * width / 2 + x / 2;
//                int indexV = width * height + width * height / 4 + y / 2 * width / 2 + x / 2;

//                BYTE Y = yuvBuffer[indexY];
//                BYTE U = yuvBuffer[indexU];
//                BYTE V = yuvBuffer[indexV];

//                RGB32 *rgbNode = &rgb32Buffer[index];

//                ///这转换的公式 百度有好多 下面这个效果相对好一些

//                rgbNode->red = Y + 1.402 * (V-128);
//                rgbNode->green = Y - 0.34413 * (U-128) - 0.71414*(V-128);
//                rgbNode->blue = Y + 1.772*(U-128);
//                rgbNode->alpha = 255;

//                img->setPixel(x, y, rgbNode->red << 16 | rgbNode->green << 8 | rgbNode->blue);
//            }
//        }
}
