#include "demo1window.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{


}

void MainWindow::on_pushButton_2_clicked()
{

}


void MainWindow::on_pushButton_3_clicked()
{
    demo1Window = new Demo1Window;
    demo1Window->show();
}

void MainWindow::on_pushButton_4_clicked()
{
    demo2Window = new Demo2Window;
    demo2Window->show();
}

void MainWindow::on_pushButton_5_clicked()
{
    demo3Window = new Demo3Window;
    demo3Window->show();
}

void MainWindow::on_pushButton_6_clicked()
{
    demo4Window = new Demo4Window;
    demo4Window->show();
}
