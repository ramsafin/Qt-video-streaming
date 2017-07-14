#ifndef V4L2DEVICE_H
#define V4L2DEVICE_H

#include <iostream>
#include <memory>
#include <vector>
#include <linux/videodev2.h>

#define DEV_NAME "/dev/video0"

#define BUFFER_SIZE 10

#define HEIGHT 320
#define WIDTH  240

/* video buffer structure */
typedef struct {
    void *start;
    size_t length;
} Buffer;


/* v4l2 device parameters structure */
typedef struct {

    std::string dev_name = DEV_NAME;

    /* resolution */
    unsigned int width  = WIDTH;
    unsigned int height = HEIGHT;

    /* fps */
    unsigned int numerator   = 1001;
    unsigned int denominator = 30000;

    /* buffer */
    unsigned int n_buffers = BUFFER_SIZE;

    /* format */
    unsigned int pixel_format = V4L2_PIX_FMT_MJPEG;
    unsigned int pix_field    = V4L2_FIELD_ANY;

} v4l2_device_param;


/* This class represents v4l2 device */
class V4L2Device {
public:

    V4L2Device();

    V4L2Device(const v4l2_device_param&);

    ~V4L2Device();

    /* Prohibit copy constructor and assignment operator */
    V4L2Device(const V4L2Device&) = delete;

    V4L2Device& operator=(const V4L2Device&) = delete;

    // ==================================== //

    int getFileDescriptor() const;

    bool isCapturing() const;

    void setIsCapturing(bool);

    // ============== Stream ============== //

    void stopCapturing();

    void startCapturing();

    void stream();

private:

    int _fd;
    bool _is_capturing;

    v4l2_device_param _parameters;
    v4l2_capability   _capability;
    v4l2_format       _format;

    std::vector<Buffer> _buffers;

    // ========= Initialization ========== //

    void init_device();

    void open_device();

    void query_capability();

    void query_format();

    void init_mmap();

    void init_fps();

    void init_buffers();

    // =========== Destruction ============ //

    void uninit_device();

    void close_device();

    // ============= State ================ //

    bool is_stream_readable();

    // =========== Stream data  ============ //

    bool read_frame();
};

#endif // V4L2DEVICE_H
