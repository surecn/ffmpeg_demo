#ifndef DEMO2WINDOW_H
#define DEMO2WINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include "videoplayer.h"

namespace Ui {
class Demo2Window;
}

class Demo2Window : public QMainWindow, public ImageLoader
{
    Q_OBJECT
public:
    explicit Demo2Window(QWidget *parent = nullptr);
    ~Demo2Window();
public slots:
    void slotGetOneFrame(QImage image);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    Ui::Demo2Window *ui;
    QImage mImage;
    VideoPlayer *mPlayer;

    // ImageLoader interface
public:
    virtual void load(QImage img) override;
};

#endif // DEMO2WINDOW_H
