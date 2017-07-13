#ifndef V4L2DEVICE_H
#define V4L2DEVICE_H

#include <iostream>
#include <memory>
#include <vector>
#include <linux/videodev2.h>

#define BUFFER_SIZE 10          /* default buffer size */
#define DEV_NAME "/dev/video0"  /* default device name */

#define HEIGHT 640
#define WIDTH 480

/* Video buffer structure */
typedef struct {
    void *start = nullptr;    // pointer to the buffer data
    size_t length = 0;        // data size
} Buffer;


/*  */
typedef struct {
    std::string dev_name = DEV_NAME;

    /* resolution */
    unsigned int width = WIDTH;
    unsigned int height = HEIGHT;

    /* fps */
    unsigned int numerator = 1001;
    unsigned int denominator = 30000;

    unsigned int n_buffers = BUFFER_SIZE;

    /* format */
    unsigned int pixel_format = V4L2_PIX_FMT_MJPEG;
    unsigned int pix_field = V4L2_FIELD_ANY;

    bool force_format = false;

} v4l2_device_param;

/* This class represents v4l2 device */
class V4L2Device {
public:

    V4L2Device(const v4l2_device_param& = v4l2_device_param());

    ~V4L2Device();

    int getFileDescriptor() const;

    bool isCapturing() const;

    // ============== Stream ============== //

    void stop_capturing();

    void start_capturing();

private:

    int _fd;
    bool _is_capturing;

    v4l2_device_param _parameters;
    v4l2_capability   _capability;
    v4l2_format       _format;

    std::vector<Buffer> _buffers;

    /* Prohibit copy constructor and assignment operator */
    V4L2Device(const V4L2Device&) = delete;

    V4L2Device& operator=(const V4L2Device&) = delete;

    // ========= Initialization ========== //

    void init_device();

    void open_device();

    void query_capability();

    void query_format();

    void query_crop();

    void init_mmap();

    void init_fps();

    void init_buffers();

    // =========== Destruction ============ //

    void uninit_device();

    void close_device();

    // ============== Stream ============== //

    void stream();

    // ============= State ================ //

    bool is_stream_readable();

    // =========== Stream data  ============ //

    bool read_frame();
};

#endif // V4L2DEVICE_H
