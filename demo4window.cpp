#include "demo4window.h"
#include "ui_demo4window.h"
#include "yuvplayer.h"

#include <QFileDialog>
#include <QPainter>
#include <QThread>
#include <sys/stat.h>
#include "SDL2/SDL.h"

Demo4Window::Demo4Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Demo4Window)
{
    ui->setupUi(this);
    ui->textEdit_2->setText("1280");
    ui->textEdit_3->setText("720");
    ui->textEdit_4->setText("30");
    ui->textEdit->setText("/Users/surecn/output.yuv");

    yuvPlayer = new YUVPlayer();
    connect(yuvPlayer, SIGNAL(sig_GetOneFrame(QImage)), this, SLOT(slotGetOneFrame(QImage)));
    yuvPlayer->start();
}

Demo4Window::~Demo4Window()
{
    delete ui;
}

void Demo4Window::slotGetOneFrame(QImage image) {
    mImage = image;
    update();
}

void Demo4Window::on_pushButton_clicked()
{
    QString qstring = QFileDialog::getOpenFileName(this, "选择文件", nullptr, nullptr, nullptr, nullptr);
    ui->textEdit->setText(qstring);
}

void Demo4Window::paintEvent(QPaintEvent *event) {

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




void Demo4Window::on_pushButton_2_clicked()
{
    char* file = ui->textEdit->toPlainText().toUtf8().data();
    int width = ui->textEdit_2->toPlainText().toInt();
    int height = ui->textEdit_3->toPlainText().toInt();
    int framerate = ui->textEdit_4->toPlainText().toInt();

    yuvPlayer->setFile(file, width, height, framerate);
}
