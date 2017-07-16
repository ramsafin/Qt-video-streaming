#include "videostreamer.h"
#include "ui_videostreamer.h"
#include <QPainter>
#include <QDebug>

VideoStreamer::VideoStreamer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VideoStreamer),
    _capture(new CaptureThread{})
{
    ui->setupUi(this);

    // connect rendered image signal to set picture slot
//    connect(this, SIGNAL(renderedImage(QImage)), this, SLOT(setPicture(QImage)));

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

    qDebug() << "capturing: " << _capture->isCapturing();
}
