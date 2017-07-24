#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    v4l2_device_param p = {};

    p.dev_name = "/dev/video0";

    // NOTE: there's no constructur with parameters yet
    VideoStreamer streamer0(p);

//    VideoStreamer streamer1("/dev/video1");

    streamer0.show();
//    streamer1.show();

    return app.exec();
}
