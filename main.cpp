#include <QApplication>
#include "mainwindow.h"
#include "v4l2device.h"

int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
//    MainWindow window;
//    window.show();
    v4l2_device_param params = {};
    params.force_format = true;

    V4L2Device device(params);

//    return a.exec();
    return 0;
}
