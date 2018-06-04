#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "capturethread.h"
#include "common.h"

/***********************/

/***********************/
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    int g_pic_width;
    int g_pic_height;
    int fd_cam;
    struct _buffers buffers[BUFFERS_NUM];
    QImage *qt_image;
    int exposure_mode;
    int exposure_value;

    int v4l2_init_camera();
    int v4l2_mmap();
    int v4l2_capture();
    int v4l2_exposure_mode(int e_mode);


    void paintEvent(QPaintEvent *);

    captureThread *capThread;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    int v4l2_exposure_mode_manual();
    int v4l2_exposure_mode_autoA();
    int v4l2_exposure_value(QString string);

private:
    Ui::MainWindow *ui;

};



#endif // MAINWINDOW_H
