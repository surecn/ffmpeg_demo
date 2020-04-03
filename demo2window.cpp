#include "demo2window.h"
#include "ui_demo2window.h"
#include <QPainter>
#include <iostream>
#include "stdio.h"
#include "SDL2/SDL.h"

Demo2Window::Demo2Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Demo2Window)
{
    ui->setupUi(this);
    mPlayer = new VideoPlayer(this);
    connect(mPlayer, SIGNAL(sig_GetOneFrame(QImage)), this, SLOT(slotGetOneFrame(QImage)));
    mPlayer->start();




}

Demo2Window::~Demo2Window()
{
    delete ui;
}

void Demo2Window::load(QImage img) {
//    emit sig_GetOneFrame(img);
}

void Demo2Window::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height());

    if (mImage.size().width() < 0) return;

    QImage img = mImage.scaled(this->size(), Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;
    painter.drawImage(QPoint(x, y), img);
}

void Demo2Window::slotGetOneFrame(QImage image) {
    mImage = image;
    update();
}

