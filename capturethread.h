#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include "v4l2device.h"
#include <thread>

class CaptureThread {
public:

    CaptureThread();

    CaptureThread(const v4l2_device_param&);

private:
    std::unique_ptr<V4L2Device> _device;
};

#endif // CAPTURETHREAD_H
