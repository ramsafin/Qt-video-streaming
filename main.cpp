#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    VideoStreamer v;

    v.show();

    return app.exec();
}
