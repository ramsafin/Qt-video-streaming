#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>
#include <iostream>

#include <thread>
#include <chrono>

#include "v4l2device.h"

/* ioctl fucntion v4l2 wrapper */
static int v4l2_ioctl(int fd, unsigned long int request, void *arg) {
    int status_code = 0;

    do {
        status_code = ioctl(fd, request, arg);
        // see https://stackoverflow.com/questions/41474299/checking-if-errno-eintr-what-does-it-mean
    } while (status_code == -1 && errno == EINTR);

    return status_code;
}

// ========= V4L2Device class ========== //

V4L2Device::V4L2Device(const v4l2_device_param &parameters) :
    _is_capturing(false), _parameters(parameters)
{
    open_device();
    init_device();
}

V4L2Device::~V4L2Device() {
    std::cout << "destructor" << std::endl;
    stopCapturing();
    uninit_device();
    close_device();
}

// ============================================== //

void V4L2Device::open_device() {

    std::cout << "open device" << std::endl;

    // see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
    struct stat st;

    if (stat(_parameters.dev_name.c_str(), &st) == -1) {
        throw std::runtime_error(_parameters.dev_name + ": cannot identify! " +
                                 std::to_string(errno) + ": " + strerror(errno));
    }

    if (!S_ISCHR(st.st_mode)) {
        throw std::runtime_error(_parameters.dev_name + " is not a char device");
    }

    // see https://linux.die.net/man/2/fcntl about flags
    _fd = open(_parameters.dev_name.c_str(), O_RDWR | O_NONBLOCK);

    if (_fd == -1) {
        throw std::runtime_error(_parameters.dev_name + ": cannot open! " + std::to_string(errno) + ": " + strerror(errno));
    }

    std::cout << "device handle: " << _fd << std::endl;
}

void V4L2Device::close_device() {

    std::cout << "close device" << std::endl;

    if (close(_fd) == -1) {
        throw std::runtime_error(_parameters.dev_name + " cannot close! " + std::to_string(errno) + ": " + strerror(errno));
    }

    _fd = -1;
}

// =============================================== //

void V4L2Device::init_device() {

    std::cout << "init device" << std::endl;

    query_capability();
    query_format();
    init_fps();
    init_buffers();
    init_mmap();
}

void V4L2Device::uninit_device() {

    std::cout << "uninit device" << std::endl;

    for (auto &buf : _buffers) {
        if (munmap(buf.start, buf.length) == -1) {
            throw std::runtime_error(std::string(strerror(errno)) + ". MUNMAP");
        }
    }
}

// =============================================== //

void V4L2Device::query_capability() {

    struct v4l2_capability capability;

    if (v4l2_ioctl(_fd, VIDIOC_QUERYCAP, &capability) == -1) {
        if (errno == EINVAL) {
            throw std::runtime_error(_parameters.dev_name + " is no V4L2 device");
        } else {
            throw std::runtime_error("VIDIOC_QUERYCAP");
        }
    }

    // check if v4l2 device can capture video
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        throw std::runtime_error(_parameters.dev_name + " is no video capture device");
    }

    // check if v4l2 device can stream video flow
    if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
        throw std::runtime_error(_parameters.dev_name + " does not support streaming I/O");
    }

    // store current capabilties
    _capability = capability;
}

void V4L2Device::query_format() {

    struct v4l2_format format = {0};

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* See what device supports with v4l2-ctl utility for example */
    format.fmt.pix.width        = _parameters.width;
    format.fmt.pix.height       = _parameters.height;
    format.fmt.pix.field        = _parameters.pix_field;
    format.fmt.pix.pixelformat  = _parameters.pixel_format;

    if (v4l2_ioctl(_fd, VIDIOC_S_FMT, &format) == -1) {
        throw std::runtime_error("VIDIOC_S_FMT");
    }

    if (format.fmt.pix.pixelformat != _parameters.pixel_format) {
        throw std::runtime_error(_parameters.dev_name + " does not support current format");
    }

    // TODO find out field format
    // VIDIOC_S_FMT may change width and height
    _parameters.width     = format.fmt.pix.width;
    _parameters.height    = format.fmt.pix.height;
    _parameters.pix_field = format.fmt.pix.field;

    // save format
    _format = format;

    std::cout << "Size: " << format.fmt.pix.width << " x " << format.fmt.pix.height << std::endl;
}

void V4L2Device::init_fps() {

    struct v4l2_streamparm stream_param = {0};

    stream_param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* BUG: numerator and denominator are alternated */
    stream_param.parm.capture.timeperframe.numerator   = _parameters.numerator;
    stream_param.parm.capture.timeperframe.denominator = _parameters.denominator;

    if (v4l2_ioctl(_fd, VIDIOC_S_PARM, &stream_param) == -1) {
        std::cerr << "VIDIOC_S_PARM" << std::endl;
    }

    // save parameters
    _parameters.numerator   = stream_param.parm.capture.timeperframe.numerator;
    _parameters.denominator = stream_param.parm.capture.timeperframe.denominator;

    std::cout << "FPS: " << stream_param.parm.capture.timeperframe.numerator << "/"
              << stream_param.parm.capture.timeperframe.denominator << std::endl;
}

void V4L2Device::init_buffers() {

    _buffers.reserve(_parameters.n_buffers);

    for (unsigned int i = 0; i < _parameters.n_buffers; ++i) {
        _buffers.push_back(Buffer{});
    }
}

void V4L2Device::init_mmap() {

    struct v4l2_requestbuffers req_buffers = {0};

    req_buffers.count  = _parameters.n_buffers;
    req_buffers.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_buffers.memory = V4L2_MEMORY_MMAP;

    /* establish a set of buffers between the application and video driver */
    if (v4l2_ioctl(_fd, VIDIOC_REQBUFS, &req_buffers) == -1) {
        if (errno == EINVAL) {
            throw std::runtime_error(_parameters.dev_name + " doesn't support memory mapping");
        } else {
            throw std::runtime_error("VIDIOC_REQBUFS");
        }
    }

    /* driver could allocate less number of buffers */
    if (req_buffers.count != _parameters.n_buffers) {
        throw std::runtime_error("Invalid requested buffers number");
    }

    std::cout << "Buffers number: " << _parameters.n_buffers << std::endl;

    /* put application buffers into driver's incoming queue */
    for (unsigned int buffer_idx = 0; buffer_idx < req_buffers.count; ++buffer_idx) {

        struct v4l2_buffer buf = {0};

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = buffer_idx;

        if (v4l2_ioctl(_fd, VIDIOC_QUERYBUF, &buf)) {
            throw std::runtime_error("VIDIOC_QUERYBUF");
        }

        _buffers[buffer_idx].length = buf.length;

        /* vulnarable place, use smart pointer -> less effective */
        _buffers[buffer_idx].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                                          MAP_SHARED, _fd, buf.m.offset);

        /* couldn't map memory. See mmap spec */
        if (MAP_FAILED == _buffers[buffer_idx].start) {
            throw std::runtime_error("MMAP");
        }
    }
}

// =============================================== //

void V4L2Device::startCapturing() {

    if (!_is_capturing) { // we're not capturing yet

        std::cout << "start capturing" << std::endl;

        enum v4l2_buf_type type;

        // query buffers
        for (unsigned int i = 0; i < _parameters.n_buffers; ++i) {

            struct v4l2_buffer buf = {0};

            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;

            if (v4l2_ioctl(_fd, VIDIOC_QBUF, &buf) == -1) {
                throw std::runtime_error("VIDIOC_QBUF");
            }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // start capture
        if (v4l2_ioctl(_fd, VIDIOC_STREAMON, &type) == -1) {
            throw std::runtime_error("VIDIOC_STREAMON");
        }

        // set the flag
        _is_capturing = true;
    }
}

void V4L2Device::stopCapturing() {

    if (_is_capturing) { // we're capturing

        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        std::cout << "stop capturing" << std::endl;

        if (v4l2_ioctl(_fd, VIDIOC_STREAMOFF, &type) == -1) {
            throw std::runtime_error("VIDIOC_STREAMOFF");
        }

        // set the flag
        _is_capturing = false;
    }
}

bool V4L2Device::isCapturing() const {
    return _is_capturing;
}

void V4L2Device::setIsCapturing(bool is_capturing) {
    _is_capturing = is_capturing;
}

int V4L2Device::getFileDescriptor() const {
    return _fd;
}

// =============================================== //

bool V4L2Device::is_stream_readable() {

    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_fd, &fds);

    struct timeval tval = {0};
    tval.tv_sec = 2;

    return select(_fd + 1, &fds, nullptr, nullptr, &tval);
}

// TODO return frame in some way (structure, or something else)
bool V4L2Device::read_frame() {

    if (!_is_capturing) return true;

    struct v4l2_buffer buf = {0};

    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // get frame from driver's outgoing queue
    if (v4l2_ioctl(_fd, VIDIOC_DQBUF, &buf) == -1) {
        switch (errno) {
            case EAGAIN:
                return false;
            case EIO:
                std::cerr << "EIO: " <<  strerror(errno) << std::endl;
                /* Could ignore EIO see spec */
                /* fall through */
            default:
                std::cerr << "EIO: " <<  strerror(errno) << std::endl;
                throw std::runtime_error("VIDIOC_DQBUF_1");
        }
    }

    /*
     * See https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/buffer.html
     * for buffer structure information
     */

    // TODO process image


    if (v4l2_ioctl(_fd, VIDIOC_QBUF, &buf) == -1) {
        throw std::runtime_error("VIDIOC_QBUF");
    }

    return true;
}

void V4L2Device::stream() {

    while (true) {

        while (_is_capturing) {

            int r = is_stream_readable();

            if (r == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error("Waiting for frame exception");
            }

            if (r == 0) {
                throw std::runtime_error("Select timeout exception");
            }

            if (read_frame()) {
                break;
            }
        }

    }


}
