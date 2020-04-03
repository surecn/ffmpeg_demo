#ifndef DEMO3WINDOW_H
#define DEMO3WINDOW_H

#include "screenrecorder.h"

#include <QMainWindow>

namespace Ui {
class Demo3Window;
}

class Demo3Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Demo3Window(QWidget *parent = nullptr);
    ~Demo3Window();

private slots:
    void on_pushButton_clicked();

private:
    Ui::Demo3Window *ui;

    ScreenRecorder *screenRecoder;
};

#endif // DEMO3WINDOW_H
