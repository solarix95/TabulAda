#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "model/adamodel.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void init();

private:
    QWidget *createSheetView(int index);

    Ui::MainWindow *ui;

    AdaModel mModel;
};

#endif // MAINWINDOW_H
