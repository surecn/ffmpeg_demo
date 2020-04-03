#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "demo1window.h"
#include "demo2window.h"
#include "demo3window.h"
#include "demo4window.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Demo1Window *demo1Window;
    Demo2Window *demo2Window;
    Demo3Window *demo3Window;
    Demo4Window *demo4Window;


private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
};
#endif // MAINWINDOW_H
