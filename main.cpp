#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    // ============== main camera ================== //

//    p.dev_name = "/dev/video0";
//    p.denominator = 50;
//    p.numerator = 1;
//    p.n_buffers = 32; /* max number of buffers */

//    VideoStreamer streamer1(p, true);

    // ============== 3 additional cameras ============== //

    v4l2_device_param p = {};

    p.pixel_format = V4L2_PIX_FMT_SGRBG8;

    p.dev_name = "/dev/video3";
    p.denominator = 60;
    p.numerator = 1;
    p.n_buffers =32;
    p.height = 480;
    p.width = 744;

    VideoStreamer streamer3(p);

    // =============================== //

    streamer3.show();
//    streamer0.show();

    return app.exec();
}
