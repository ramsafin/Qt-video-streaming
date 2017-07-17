#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include "v4l2device.h"
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

class CaptureThread {
public:

    CaptureThread(const v4l2_device_param& = {});

    CaptureThread(const CaptureThread&) = delete;

    CaptureThread& operator=(const CaptureThread&) = delete;

    // ================================= //

    void start();

    void stop();

    void changeState();

    // ================================== //

    void run();

    bool readFrame();

    bool isCapturing();

    // =================================== //

    void setCallback(const std::function<void(const Buffer&, const struct v4l2_buffer&)> &);


private:
    std::unique_ptr<V4L2Device> _device;
    std::mutex _mutex;
};

#endif // CAPTURETHREAD_H
