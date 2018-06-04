#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <QPainter>

extern QImage * g_img;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    fd_cam(-1)
{
    ui->setupUi(this);
    if(v4l2_init_camera()<0)
        qDebug()<<"init camera failed.";
    if(v4l2_mmap()<0)
        qDebug()<<"mmap failed.";
    g_img = NULL;
    /****************/
    capThread = new captureThread();
    capThread->fd_cam_t = this->fd_cam;
    capThread->pic_height = this->g_pic_height;
    capThread->pic_width = this->g_pic_width;
    for(int i=0; i<BUFFERS_NUM; i++)
        capThread->buffers_t[i] = this->buffers[i];

    connect(capThread,
            &captureThread::drawPicPLZ,
            this,
            static_cast<void (MainWindow:: *)(void)>(&MainWindow::repaint));

    capThread->start();
    /****************/
}

MainWindow::~MainWindow()
{
    delete ui;
}
int MainWindow::v4l2_init_camera()
{
    qDebug()<<"begin to init camera! \n";

    fd_cam = open("/dev/video0",O_RDWR);
    if(fd_cam < 0)
    {
        //printf("failed to open camera.\n");
        qDebug()<<"failed to open camera. \n";
        return -1;
    }

    struct v4l2_capability caps;
    if(ioctl(fd_cam,VIDIOC_QUERYCAP,&caps)<0)
    {
        qDebug()<<"qurry camera's caps failed. \n";
        return -1;
    }
    printf("Driver name:%s\n Card name:%s\n Bus info:%s\n Driver info:%u.%u.%u\n ",
           caps.driver,caps.card,caps.bus_info,(caps.version>>16)%0xff,(caps.version>>8)%0xff,(caps.version)%0xff);

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qDebug()<<"support formats:";
    while(ioctl(fd_cam,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
        printf("\tpixel fromat = %c%c%c%c \n",fmtdesc.pixelformat&0xff,(fmtdesc.pixelformat>>8)&0xff,(fmtdesc.pixelformat>>16)&0xff,(fmtdesc.pixelformat>>24)&0xff);
        fmtdesc.index++;
    }

    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd_cam,VIDIOC_G_FMT,&fmt);
    g_pic_height = fmt.fmt.pix.height;
    g_pic_width = fmt.fmt.pix.width;
    printf("current pixel fromat = %c%c%c%c \n",fmt.fmt.pix.pixelformat&0xff,(fmt.fmt.pix.pixelformat>>8)&0xff,(fmt.fmt.pix.pixelformat>>16)&0xff,(fmt.fmt.pix.pixelformat>>24)&0xff);
    printf("Current data format infomation: \n\twidth:%d\n\theight:%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);

//    memset(&fmt,0,sizeof(fmt));
//    fmt.fmt.pix.width = g_pic_width;
//    fmt.fmt.pix.height = g_pic_height;
//    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;//?
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
//    if(ioctl(fd_cam,VIDIOC_S_FMT,&fmt)<0)
//    {
//        qDebug()<<"set pixle format failed!";
//        return -1;
//    }

//    fmtdesc.index = 0;
//    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    while(ioctl(fd_cam,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
//    {
//        if(fmtdesc.pixelformat & fmt.fmt.pix.pixelformat)
//        {
//            printf("\tpixelformat:%s\n",fmtdesc.description);
//            break;
//        }
//        fmtdesc.index++;
//    }

//    struct v4l2_input input;
//    memset(&input,0,sizeof(input));
//    if(-1==ioctl(fd_cam,VIDIOC_G_INPUT,&input.index))
//    {
//        perror("VIDIOC_G_INPUT");
//        exit(EXIT_FAILURE);
//    }
//    if(-1==ioctl(fd_cam,VIDIOC_ENUMINPUT,&input))
//    {
//        perror("VIDIOC_ENUMINPUT");
//        exit(EXIT_FAILURE);
//    }
//    printf("Current input '%s' supports:\n",input.name);
//    struct v4l2_standard standard;
//    memset(&standard,0,sizeof(standard));
//    standard.index = 0;
//    while(0==ioctl(fd_cam,VIDIOC_ENUMSTD,&standard))
//    {
//        if(standard.id & input.std)
//            printf("%s\n",standard.name);
//        standard.index++;
//    }
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd_cam,VIDIOC_G_PARM,&parm);
    qDebug()<<"parm.parm.capture.timeperframe.numerator :"<<parm.parm.capture.timeperframe.numerator;
    qDebug()<<"parm.parm.capture.timeperframe.denominator:"<<parm.parm.capture.timeperframe.denominator;
//    parm.parm.capture.timeperframe.denominator = 30;
//    if(ioctl(fd_cam,VIDIOC_S_PARM,&parm)<0)
//        qDebug()<<"VIDIOC_S_PARM failed.";


    qDebug()<<"camera init done.";
    return 0;
}


int MainWindow::v4l2_mmap()
{
    struct v4l2_requestbuffers req;
    memset(&req,0,sizeof(req));
    req.count = BUFFERS_NUM;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd_cam,VIDIOC_REQBUFS,&req)<0)//request device's buffers from kernel sapce
    {
        qDebug()<<"request buffers in kernel space for cam failed.";
        return -1;
    }

    for(int i=0; i<req.count; i++)
    {
        struct v4l2_buffer buf_info;
        memset(&buf_info,0,sizeof(buf_info));
        buf_info.type = req.type;
        buf_info.memory = V4L2_MEMORY_MMAP;
        buf_info.index = i;
        if(ioctl(fd_cam,VIDIOC_QUERYBUF,&buf_info)<0)//query status of a buffer in kernel space
        {
            qDebug()<<"VIDIOC_QUERYBUF failed.";
            return -1;
        }
        buffers[i].length = buf_info.length;
        buffers[i].start = mmap(NULL,buf_info.length,//map buffers in kernel space to user space
                                PROT_READ|PROT_WRITE,
                                MAP_SHARED,
                                fd_cam,buf_info.m.offset);
        if(buffers[i].start == MAP_FAILED)
        {
            qDebug()<<"MAP_FAILED";
            return -1;
        }
        buffers[i].offset = buf_info.m.offset;
    }
    for(int i=0; i<req.count; i++)
    {
        struct v4l2_buffer buf_info;
        memset(&buf_info,0,sizeof(buf_info));
        buf_info.type = req.type;
        buf_info.memory = V4L2_MEMORY_MMAP;
        buf_info.index = i;
        //buf_info.m.offset = buffers[i].offset;
        if(ioctl(fd_cam,VIDIOC_QBUF,&buf_info)<0)//enqueue an empty or filled buffer in the driver's incoming queue
        {
            qDebug()<<"VIDIOC_QBUF failed.";
            return -1;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd_cam,VIDIOC_STREAMON,&type)<0)
    {
        qDebug()<<"VIDIOC_STREAMON";
        return -1;
    }
    return 0;
}

int MainWindow::v4l2_capture()//shoule not be excuted in mainWindow
{
    struct v4l2_buffer buf_info;
    while(1)
    {
        memset(&buf_info,0,sizeof(buf_info));
        buf_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf_info.memory = V4L2_MEMORY_MMAP;
        if(ioctl(fd_cam,VIDIOC_DQBUF,&buf_info)<0)
        {
            qDebug()<<"VIDIOC_DQBUF failed in v4l2_capture().";
            return -1;
        }

        cv::Mat pic(g_pic_height,g_pic_width,CV_8UC2,buffers[buf_info.index].start);
        cv::Mat img(g_pic_height,g_pic_width,CV_8UC3);
        cv::cvtColor(pic,img,CV_YUV2RGB_YVYU);
        qt_image =new QImage(img.data,img.cols,img.rows,QImage::Format_RGB888);



        cv::imshow("img",img);
        cv::waitKey(10);

        if(ioctl(fd_cam,VIDIOC_QBUF,&buf_info)<0)
        {
            qDebug()<<"VIDIOC_QBUF failed in v4l2_capture()";
            return -1;
        }
    }
    return 0;
}
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter mypainter(this);
    if(g_img != NULL)
        mypainter.drawImage(QRect(QPoint(0,0),QSize(640,480)),*g_img);
    //qDebug()<<"painting~";
}
int MainWindow::v4l2_exposure_mode(int e_mode)
{
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type"<<ctrl.value;

    switch(e_mode)
    {
    case 0:
        ctrl.value = V4L2_EXPOSURE_AUTO;
        break;
    case 1:
        ctrl.value = V4L2_EXPOSURE_MANUAL;
        break;
    case 2:
        ctrl.value = V4L2_EXPOSURE_SHUTTER_PRIORITY;
        break;
    default :
        ctrl.value = V4L2_EXPOSURE_APERTURE_PRIORITY;
        break;
    }
    if(ioctl(fd_cam,VIDIOC_S_CTRL,&ctrl)<0)
    {
        qDebug()<<"VIDIOC_S_CTRL failed.";
        return -1;
    }
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type now"<<ctrl.value;

    return 0;
}
int MainWindow::v4l2_exposure_mode_manual()
{
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type"<<ctrl.value;
    ctrl.value = V4L2_EXPOSURE_MANUAL;
    if(ioctl(fd_cam,VIDIOC_S_CTRL,&ctrl)<0)
    {
        qDebug()<<"VIDIOC_S_CTRL failed.";
        return -1;
    }
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type now"<<ctrl.value;

    return 0;
}
int MainWindow::v4l2_exposure_mode_autoA()
{
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE_AUTO;
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type"<<ctrl.value;
    ctrl.value = V4L2_EXPOSURE_APERTURE_PRIORITY;
    if(ioctl(fd_cam,VIDIOC_S_CTRL,&ctrl)<0)
    {
        qDebug()<<"VIDIOC_S_CTRL failed.";
        return -1;
    }
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"exposure auto type now"<<ctrl.value;

    return 0;
}
int MainWindow::v4l2_exposure_value(QString string)
{
    int e_value = 0;
    e_value = string.toUInt();
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"absolute exposure value:"<<ctrl.value;
    ctrl.value = e_value;
    if(ioctl(fd_cam,VIDIOC_S_CTRL,&ctrl)<0)
    {
        qDebug()<<"VIDIOC_S_CTRL failed.";
        return -1;
    }
    ioctl(fd_cam,VIDIOC_G_CTRL,&ctrl);
    qDebug()<<"absolute exposure value now:"<<ctrl.value;

    return 0;
}
