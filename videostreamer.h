#ifndef VIDEOSTREAMER_H
#define VIDEOSTREAMER_H

#include <QMainWindow>
#include <QPixmap>
#include "capturethread.h"


namespace Ui {
    class VideoStreamer;
}

class VideoStreamer : public QMainWindow
{
    Q_OBJECT

public:
    explicit VideoStreamer(QWidget *parent = 0);
    ~VideoStreamer();

    QPixmap pixmap; // ***

signals:
    void renderedImage(const QImage&); // ***

private slots:
    void on_streamButton_clicked();
    void setPicture(const QImage&); // ***

private:
    Ui::VideoStreamer *ui;
    std::unique_ptr<CaptureThread> _capture;

    void paintEvent(QPaintEvent *event); // ***
};

#endif // VIDEOSTREAMER_H
