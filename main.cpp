#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    VideoStreamer streamer;

    streamer.show();

    return app.exec();
}
