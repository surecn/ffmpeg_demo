#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QImage>
#include <QThread>



class ScreenCapture : public QThread
{
    Q_OBJECT
public:
    ScreenCapture();
protected:
    virtual void run() override;
signals:
    void sig_GetOneFrame(QImage);
};

#endif // SCREENCAPTURE_H
