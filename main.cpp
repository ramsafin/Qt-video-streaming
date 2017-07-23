#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    // NOTE: there's no constructur with parameters yet
    VideoStreamer streamer0("/dev/video0");
    VideoStreamer streamer1("/dev/video1");

    streamer0.show();
    streamer1.show();

    return app.exec();
}
