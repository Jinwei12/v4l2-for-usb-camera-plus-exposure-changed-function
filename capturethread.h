#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include "common.h"


class captureThread : public QThread
{
    Q_OBJECT

public:
    captureThread(void);
    int fd_cam_t;
    int pic_width;
    int pic_height;
    struct _buffers buffers_t[BUFFERS_NUM];
    QImage * qt_img_t;
    void sendSignal()
    {
        emit drawPicPLZ();
    }
signals:
    void drawPicPLZ();

protected:
    void run();
};

#endif // CAPTURETHREAD_H
