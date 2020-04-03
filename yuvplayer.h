#ifndef YUVPLAYER_H
#define YUVPLAYER_H

#include <QThread>
#include <QImage>
extern "C" {
#include <QWaitCondition>
}

typedef unsigned char BYTE;

typedef struct RGB32 {
    BYTE blue;
    BYTE green;
    BYTE red;
    BYTE alpha;
} RGB32;

class YUVPlayer : public QThread
{
Q_OBJECT
public:
    YUVPlayer();
    void setFile(char *_file, int width, int height, int framerage);
    void setQuit(int _quit);
signals:
    void sig_GetOneFrame(QImage);
protected:
    virtual void run() override;
    void yuv420pToRgb32(const BYTE *yuvBufferIn, const BYTE *rgbBufferOut, int width, int height);
    void process();
private:
    char *file;
    int width;
    int height;
    int framerate;
    int quit;
    QWaitCondition *mConditionVideo;
    QMutex *mMutexVideo;


};


#endif // YUVPLAYER_H
