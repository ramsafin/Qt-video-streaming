#include <QApplication>
#include "videostreamer.h"

int main(int argc, char *argv[])
{

  QApplication app(argc, argv);

  v4l2_device_param p = {};

  p.dev_name = "/dev/v4l/by-id/usb-Twiga_TWIGACam-video-index0";
  VideoStreamer frontCenterCamera(p, true);

  p.pixel_format = V4L2_PIX_FMT_SGRBG8;
  p.dev_name ="/dev/v4l/by-id/usb-The_Imaging_Source_Europe_GmbH_DFM_22BUC03-ML_03610446-video-index0";
  VideoStreamer leftFrontCamera(p);


  p.dev_name = "/dev/v4l/by-id/usb-The_Imaging_Source_Europe_GmbH_DFM_22BUC03-ML_03610450-video-index0";
  VideoStreamer backCamera(p);

  p.dev_name = "/dev/v4l/by-id/usb-The_Imaging_Source_Europe_GmbH_DFM_22BUC03-ML_03610453-video-index0";
  VideoStreamer rightFrontCamera(p);

//  rightFrontCamera.show();

//  backCamera.show();

//  leftFrontCamera.show();

//  frontCenterCamera.show();

  return app.exec();
}
