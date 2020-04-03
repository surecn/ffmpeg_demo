#ifndef DEMO4WINDOW_H
#define DEMO4WINDOW_H

#include "yuvplayer.h"

#include <QMainWindow>
#include <QPaintEvent>

namespace Ui {
class Demo4Window;
}



class Demo4Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Demo4Window(QWidget *parent = nullptr);
    ~Demo4Window();

protected:
    void paintEvent(QPaintEvent *event) override;
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void slotGetOneFrame(QImage image);

private:
    Ui::Demo4Window *ui;
     QImage mImage;
     YUVPlayer *yuvPlayer;

};

#endif // DEMO4WINDOW_H
