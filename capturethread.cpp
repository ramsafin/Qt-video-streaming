#include "capturethread.h"

CaptureThread::CaptureThread(const v4l2_device_param &parameters) :
    _device(new V4L2Device(parameters))
{
    std::thread([&]() {
        run();
    }).detach();
}

void CaptureThread::start() {
    _device->startCapturing();
}

void CaptureThread::stop() {
    _device->stopCapturing();
}

void CaptureThread::changeState() {

    std::unique_lock<std::mutex> lock(_mutex);

    if (_device->isCapturing()) {
        _device->stopCapturing();
    } else {
        _device->startCapturing();
    }
}

bool CaptureThread::isCapturing() {
    return _device->isCapturing();
}

bool CaptureThread::readFrame() {

    std::unique_lock<std::mutex> lock(_mutex);

    if (_device->isCapturing()) {
        return _device->readFrame();
    }

    return true;
}

void CaptureThread::run() {

    while (true) {

        while (_device->isCapturing()) {

            int r = _device->isStreamReadable();

            if (r == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error("Waiting for frame exception");
            }

            if (r == 0) {
                throw std::runtime_error("Select timeout exception");
            }

            if (readFrame()) {
                break;
            } else {
                std::cout << "2" << std::endl;
            }

        }
    }
}

void CaptureThread::setCallback(const std::function<void (const Buffer &, const struct v4l2_buffer &)> &callback) {
    _device->setCallback(callback);
}
