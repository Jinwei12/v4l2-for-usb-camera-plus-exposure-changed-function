#include "capturethread.h"
#include "mainwindow.h"
#include "common.h"
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <sys/ioctl.h>
#include <QDebug>

extern QImage * g_img;
captureThread::captureThread(void)
{

}
void captureThread::run()
{
    struct v4l2_buffer buf_info;
    while(1)
    {
        memset(&buf_info,0,sizeof(buf_info));
        buf_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf_info.memory = V4L2_MEMORY_MMAP;
        if(ioctl(fd_cam_t,VIDIOC_DQBUF,&buf_info)<0)
        {
            qDebug()<<"VIDIOC_DQBUF failed in v4l2_capture().";
            exit(EXIT_FAILURE);
        }

        cv::Mat pic(pic_height,pic_width,CV_8UC2,buffers_t[buf_info.index].start);
        cv::Mat img(pic_height,pic_width,CV_8UC3);
        cv::cvtColor(pic,img,CV_YUV2RGB_YUY2);
        //qt_img_t = new QImage(img.data,img.cols,img.rows,QImage::Format_RGB888);
        g_img = new QImage(img.data,img.cols,img.rows,QImage::Format_RGB888);

        sendSignal();

        //cv::imshow("img",img);
        //cv::waitKey(10);

        if(ioctl(fd_cam_t,VIDIOC_QBUF,&buf_info)<0)
        {
            qDebug()<<"VIDIOC_QBUF failed in v4l2_capture()";
            exit(EXIT_FAILURE);
        }
    }
    //MainWindow::v4l2_capture();
}
