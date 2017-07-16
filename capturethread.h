#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include "v4l2device.h"
#include <thread>
#include <mutex>

class CaptureThread {
public:

    CaptureThread(const v4l2_device_param& = {});

    CaptureThread(const CaptureThread&) = delete;

    CaptureThread& operator=(const CaptureThread&) = delete;

    void start();

    void stop();

    bool isCapturing();

    void changeState();

private:
    std::unique_ptr<V4L2Device> _device;
//    std::mutex _mutex;
};

#endif // CAPTURETHREAD_H
