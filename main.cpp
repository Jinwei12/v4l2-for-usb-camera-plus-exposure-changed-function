#include "mainwindow.h"
#include "capturethread.h"
#include <QApplication>
#include <fcntl.h>
#include <QDebug>
#include <QImage>

QImage * g_img;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

