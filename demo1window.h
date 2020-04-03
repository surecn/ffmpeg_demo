#ifndef DEMO1WINDOW_H
#define DEMO1WINDOW_H

#include <QMainWindow>

namespace Ui {
class Demo1Window;
}

class Demo1Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Demo1Window(QWidget *parent = nullptr);
    ~Demo1Window();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::Demo1Window *ui;
};

#endif // DEMO1WINDOW_H
