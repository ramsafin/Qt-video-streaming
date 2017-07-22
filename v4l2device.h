#ifndef V4L2DEVICE_H
#define V4L2DEVICE_H

#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <linux/videodev2.h>

#define DEV_NAME "/dev/video0"

#define BUFFER_SIZE 10

#define WIDTH  1280
#define HEIGHT 720

using namespace std;

/**
 * Frames buffer structure
 * @param data  - pointer to the raw frame data
 * @param size  - data size (in bytes)
 */
typedef struct {
    void *data;
    size_t size;
} Buffer;


/**
 * v4l2 device's parameters structure
 */
typedef struct {

    string dev_name = DEV_NAME;

    /* resolution */
    unsigned int width  = WIDTH;
    unsigned int height = HEIGHT;

    /* fps */
    unsigned int numerator   = 1001;
    unsigned int denominator = 30000;

    /* buffer */
    unsigned int n_buffers = BUFFER_SIZE;

    /* format */
    unsigned int pixel_format = V4L2_PIX_FMT_YUYV;
    unsigned int pix_field    = V4L2_FIELD_NONE;

} v4l2_device_param;


/**
 * Represents v4l2 device, i.e. /dev/video0
 */
class V4L2Device {

public:

    /* constructor with device's preferred parameters */
    V4L2Device(const v4l2_device_param& = {});

    /* destructor */
    ~V4L2Device();

    /* Prohibit copy constructor and assignment operator */
    V4L2Device(const V4L2Device&)            = delete;
    V4L2Device& operator=(const V4L2Device&) = delete;

    // =================================== //

    int getHandle() const;

    void setCallback(const function<void(const Buffer&, const struct v4l2_buffer&)> &);

    // ============== Stream ============== //

    void stopCapturing();

    void startCapturing();

    // ============= State ================ //

    bool isCapturing() const;

    void changeState();

    // ============== Uitls =============== //

    void printInfo();

private:

    /* device's file descriptor */
    int _fd;

    /* capturing state flag, thread safe */
    atomic<bool> _is_capturing;

    /* internal device parameters, capabilities, etc. */
    v4l2_device_param _parameters;
    v4l2_capability   _capability;
    v4l2_format       _format;
    v4l2_streamparm   _stream_parameters;

    /* frames' buffers */
    vector<Buffer> _buffers;

    /* callback function, it's invoked when frame's read */
    function<void(const Buffer&, const struct v4l2_buffer&)> _callback;

    /* multithreading */
    mutex _stream_mutex;

    // ========= Initialization ========== //

    void init_device();

    void open_device();

    void query_capability();

    void query_format();

    void init_buffers();

    void init_mmap();

    void init_fps();

    // =========== Destruction ============ //

    void uninit_device();

    void close_device();

    // ============= Stream =============== //

    int is_stream_readable();

    bool read_frame();

    void stream();
};

#endif // V4L2DEVICE_H
