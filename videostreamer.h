#ifndef VIDEOSTREAMER_H
#define VIDEOSTREAMER_H

#include <memory>
#include <QMainWindow>
#include <QPixmap>
#include "v4l2device.h"

using namespace std;

namespace Ui {
    class VideoStreamer;
}

class VideoStreamer : public QMainWindow
{
    Q_OBJECT

public:
    explicit VideoStreamer(QWidget *parent = 0);
    VideoStreamer(string device);
    ~VideoStreamer();

    QPixmap pixmap;

signals:
    void renderedImage(const QImage&);

private slots:
    void on_streamButton_clicked();
    void setPicture(const QImage&);

private:
    Ui::VideoStreamer *ui;

    int _width;
    int _height;
    int _stride; // bytes per line

    unique_ptr<V4L2Device> _capture;

    void paintEvent(QPaintEvent *event);
};

/*
 * Taken from libv4l2
 * TODO move to separate header/source
 */

#define CLIP(color) (unsigned char)(((color) > 0xFF) ? 0xFF : (((color) < 0) ? 0 : (color)))

static void v4lconvert_yuyv_to_rgb24(
        const unsigned char* source,
        unsigned char *dest,
        int width, int height, int stride)
{
    int j;

    while (--height >= 0) {
        for (j = 0; j + 1 < width; j += 2) {
            int u = source[1];
            int v = source[3];
            int u1 = (((u - 128) << 7) +  (u - 128)) >> 6;
            int rg = (((u - 128) << 1) +  (u - 128) +
                      ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
            int v1 = (((v - 128) << 1) +  (v - 128)) >> 1;

            *dest++ = CLIP(source[0] + v1);
            *dest++ = CLIP(source[0] - rg);
            *dest++ = CLIP(source[0] + u1);

            *dest++ = CLIP(source[2] + v1);
            *dest++ = CLIP(source[2] - rg);
            *dest++ = CLIP(source[2] + u1);
            source += 4;
        }
        source += stride - (width * 2);
    }
}

#endif // VIDEOSTREAMER_H
