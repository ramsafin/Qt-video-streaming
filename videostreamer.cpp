#include "videostreamer.h"
#include "ui_videostreamer.h"
#include <QPainter>
#include <QDebug>
#include <QByteArray>


VideoStreamer::VideoStreamer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VideoStreamer),
    _capture(new V4L2Device{})
{
    ui->setupUi(this);

    _capture->setCallback([&](const Buffer& buffer, const struct v4l2_buffer& buffer_info) {

        cout << "start callback" << endl;

        QImage img = QImage(WIDTH, HEIGHT, QImage::Format_RGB888);

        unsigned char *rgb_data = img.bits();

        v4lconvert_yuyv_to_rgb24((const unsigned char*) buffer.data, rgb_data, WIDTH, HEIGHT,
                                 2560);

        emit renderedImage(img);
    });

    // connect rendered image signal to set picture slot
    connect(this, SIGNAL(renderedImage(QImage)), this, SLOT(setPicture(QImage)));

    setAutoFillBackground(true);
}

VideoStreamer::VideoStreamer(string device) : QMainWindow(0), ui(new Ui::VideoStreamer)
{
    v4l2_device_param p = {};
    p.dev_name = device;

    _capture.reset(new V4L2Device(p));

    ui->setupUi(this);

    _width =  (int) _capture->getWidth();
    _height = (int) _capture->getHeight();
    _stride = (int) _capture->getStride();

    _capture->setCallback([&](const Buffer& buffer, const struct v4l2_buffer& buffer_info) {

        QImage img = QImage(_width, _height, QImage::Format_RGB888);

        unsigned char *rgb_data = img.bits();

        v4lconvert_yuyv_to_rgb24((const unsigned char*) buffer.data, rgb_data, _width, _height, _stride);

        emit renderedImage(img);
    });

    // connect rendered image signal to set picture slot
    connect(this, SIGNAL(renderedImage(QImage)), this, SLOT(setPicture(QImage)));

    setAutoFillBackground(true);
}

void VideoStreamer::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 30));
    painter.drawText(rect(), Qt::AlignCenter, "Qt");
    painter.drawPixmap(this->rect(), pixmap);
}

void VideoStreamer::setPicture(const QImage& image) {

    pixmap = QPixmap::fromImage(image);
    update();
    qApp->processEvents();
}

VideoStreamer::~VideoStreamer()
{
    delete ui;
}

void VideoStreamer::on_streamButton_clicked()
{
    _capture->changeState();

    if (_capture->isCapturing()) {
        ui->streamButton->setText("Stop");
    } else {
        ui->streamButton->setText("Start");
    }
}
