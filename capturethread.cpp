#include "capturethread.h"

CaptureThread::CaptureThread(const v4l2_device_param &parameters) :
    _device(new V4L2Device(parameters))
{
    std::thread([&]() {
        _device->stream();
    }).detach();
}

void CaptureThread::start() {
    _device->startCapturing();
}

void CaptureThread::stop() {
    _device->stopCapturing();
}

void CaptureThread::changeState() {

//    std::unique_lock<std::mutex> lock(_mutex);

    if (isCapturing()) {
        stop();
    } else {
        start();
    }
}

bool CaptureThread::isCapturing() {
    return _device->isCapturing();
}
