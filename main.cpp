#include "v4l2device.h"
#include <thread>
#include <chrono>
#include <QApplication>
#include <QWidget>
#include "capturethread.h"

int main(int argc, char *argv[])
{
    v4l2_device_param p = {};

    CaptureThread thread(p);

    QApplication app(argc, argv);

    QWidget *widget = new QWidget;

    widget->show();

    return app.exec();
}
