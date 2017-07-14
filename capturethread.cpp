#include "capturethread.h"

CaptureThread::CaptureThread() :
    _device(new V4L2Device) {

    // start thread
    std::thread([&]() {
        _device->stream();
    }).detach();
}

CaptureThread::CaptureThread(const v4l2_device_param &parameters) :
    _device(new V4L2Device(parameters))
{
    std::thread([&]() {
        _device->stream();
    }).detach();
}
