#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cstring>
#include <iostream>
#include <functional>

#include "v4l2device.h"

/* ioctl fucntion */
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

    // start streaming in a new thread
    thread([&](){
        stream();
    }).detach();

    printInfo();
}

V4L2Device::~V4L2Device() {
    stopCapturing();
    uninit_device();
    close_device();
}

// ============================================== //

void V4L2Device::open_device() {

    struct stat st; // see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html

    if (stat(_parameters.dev_name.c_str(), &st) == -1) {
        throw runtime_error(_parameters.dev_name + ": cannot identify! " +
                            to_string(errno) + ": " + strerror(errno));
    }

    if (!S_ISCHR(st.st_mode)) {
        throw runtime_error(_parameters.dev_name + " is not a char device");
    }

    /*
     * NOTE: By default VIDIOC_DQBUF blocks when no buffer is in the outgoing queue.
     * When the O_NONBLOCK flag was given to the open() function system call,
     * VIDIOC_DQBUF returns immediately with an EAGAIN error code when no buffer is available.
     */

    _fd = open(_parameters.dev_name.c_str(), O_RDWR | O_NONBLOCK);

    if (_fd == -1) {
        throw runtime_error(_parameters.dev_name + ": cannot open! " + to_string(errno) + ": " + strerror(errno));
    }
}

void V4L2Device::close_device() {

    if (close(_fd) == -1) {
        throw runtime_error(_parameters.dev_name + " cannot close! " +
                            to_string(errno) + ": " + strerror(errno));
    }

    _fd = -1;
}

// =============================================== //

void V4L2Device::init_device() {
    query_capability();
    query_format();
    init_fps();
    init_buffers();
    init_mmap();
}

void V4L2Device::uninit_device() {
    // unmap buffers
    for (auto &buf : _buffers) {
        if (munmap(buf.data, buf.size) == -1) {
            throw runtime_error(string(strerror(errno)) + ". MUNMAP");
        }
    }
}

// =============================================== //

void V4L2Device::query_capability() {

    struct v4l2_capability capability;

    // query capabilities
    if (v4l2_ioctl(_fd, VIDIOC_QUERYCAP, &capability) == -1) {
        if (errno == EINVAL) {
            throw runtime_error(_parameters.dev_name + " is no V4L2 device");
        } else {
            throw runtime_error("VIDIOC_QUERYCAP");
        }
    }

    // check if v4l2 device can capture video
    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        throw runtime_error(_parameters.dev_name + " is no video capture device");
    }

    // check if v4l2 device can stream
    if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
        throw runtime_error(_parameters.dev_name + " does not support streaming I/O");
    }

    // store received capabilties
    _capability = capability;
}

void V4L2Device::query_format() {

    struct v4l2_format format = {0};

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* See what device supports, i.e. using v4l2-ctl utility */
    format.fmt.pix.width        = _parameters.width;
    format.fmt.pix.height       = _parameters.height;
    format.fmt.pix.field        = _parameters.pix_field;
    format.fmt.pix.pixelformat  = _parameters.pixel_format;

    // query format
    if (v4l2_ioctl(_fd, VIDIOC_S_FMT, &format) == -1) {
        throw runtime_error("VIDIOC_S_FMT");
    }

    if (format.fmt.pix.pixelformat != _parameters.pixel_format) {
        throw runtime_error(_parameters.dev_name + " does not support current format");
    }

    // save received format
    _format = format;

    cout << "bytes per line: " << _format.fmt.pix.bytesperline << endl;
}

void V4L2Device::init_fps() {

    struct v4l2_streamparm stream_param = {0};

    stream_param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* BUG: numerator and denominator are alternated */
    stream_param.parm.capture.timeperframe.numerator   = _parameters.numerator;
    stream_param.parm.capture.timeperframe.denominator = _parameters.denominator;

    // set fps
    if (v4l2_ioctl(_fd, VIDIOC_S_PARM, &stream_param) == -1) {
        /* Never mind */
        cerr << "VIDIOC_S_PARM" << endl;
    }

    // save stream parameters
    _stream_parameters = stream_param;
}

void V4L2Device::init_buffers() {

    _buffers.reserve(_parameters.n_buffers);

    for (unsigned int i = 0; i < _parameters.n_buffers; ++i) {
        _buffers.push_back(Buffer{0});
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
            throw runtime_error(_parameters.dev_name + " doesn't support memory mapping");
        } else {
            throw runtime_error("VIDIOC_REQBUFS");
        }
    }

    /* driver might allocate less number of buffers */
    if (req_buffers.count != _parameters.n_buffers) {
        throw runtime_error("Invalid requested buffers number");
    }

    /* put application buffers into driver's incoming queue in order to get raw data */
    for (unsigned int buffer_idx = 0; buffer_idx < req_buffers.count; ++buffer_idx) {

        struct v4l2_buffer buffer_info = {0};

        buffer_info.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer_info.memory = V4L2_MEMORY_MMAP;
        buffer_info.index  = buffer_idx;

        if (v4l2_ioctl(_fd, VIDIOC_QUERYBUF, &buffer_info)) {
            throw runtime_error("VIDIOC_QUERYBUF");
        }

        _buffers[buffer_idx].size = buffer_info.length;

        /* vulnarable place, use smart pointer -> less effective, ownership */
        _buffers[buffer_idx].data = mmap(nullptr, buffer_info.length, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, _fd, buffer_info.m.offset);

        /* couldn't map memory. See mmap spec */
        if (MAP_FAILED == _buffers[buffer_idx].data) {
            throw runtime_error("MMAP");
        }
    }
}

// =============================================== //

void V4L2Device::startCapturing() {

    lock_guard<mutex> lock(_stream_mutex);

    if (!_is_capturing) { // we're not capturing yet

        struct v4l2_buffer buffer_info = {0};

        buffer_info.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer_info.memory = V4L2_MEMORY_MMAP;

        /*
         * NOTE: some devices will refuse to get into streaming mode
         * if there aren't already buffers queued
         */
        for (unsigned int i = 0; i < _parameters.n_buffers; ++i) {

            buffer_info.index  = i;

            if (v4l2_ioctl(_fd, VIDIOC_QBUF, &buffer_info) == -1) {
                throw runtime_error("VIDIOC_QBUF");
            }
        }

        // enable streaming
        if (v4l2_ioctl(_fd, VIDIOC_STREAMON, &buffer_info.type) == -1) {
            throw runtime_error("VIDIOC_STREAMON");
        }

        // set the flag
        _is_capturing = true;
    }
}

void V4L2Device::stopCapturing() {

    lock_guard<mutex> lock(_stream_mutex);

    if (_is_capturing) { // we're capturing

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // disable streaming
        if (v4l2_ioctl(_fd, VIDIOC_STREAMOFF, &type) == -1) {
            throw runtime_error("VIDIOC_STREAMOFF");
        }

        // set the flag
        _is_capturing = false;
    }
}

bool V4L2Device::isCapturing() const {
    return _is_capturing;
}

void V4L2Device::changeState() {
    if (_is_capturing) {
        stopCapturing();
    } else {
        startCapturing();
    }
}

int V4L2Device::is_stream_readable() {

    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_fd, &fds);

    struct timeval tval = {0};
    tval.tv_sec = 2; // timeout in seconds

    /*
     * check whether specified device with file descriptor is ready for
     * reading/writing operation or has an error condition
     */
    return select(_fd + 1, &fds, nullptr, nullptr, &tval);
}

bool V4L2Device::read_frame() {

    lock_guard<mutex> lock(_stream_mutex);

    // additional check of the streaming flag
    if (!_is_capturing) return false;

    struct v4l2_buffer buffer_info = {0};

    buffer_info.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_info.memory = V4L2_MEMORY_MMAP;

    // get frame from driver's outgoing queue
    if (v4l2_ioctl(_fd, VIDIOC_DQBUF, &buffer_info) == -1) {
        switch (errno) {
            case EAGAIN:
                return false;
            case EIO:
                cerr << "I/O ERROR: " <<  strerror(errno) << endl;
                /* Could ignore EIO, see spec */
                /* fall through */
            default:
                throw runtime_error("VIDIOC_DQBUF");
        }
    }

    if (_callback) { // callback
        _callback(_buffers[buffer_info.index], buffer_info);
    }


    if (v4l2_ioctl(_fd, VIDIOC_QBUF, &buffer_info) == -1) {
        throw runtime_error("VIDIOC_QBUF");
    }

    return true;
}

void V4L2Device::stream() {

    while (true) {

        if (_is_capturing) {

            int r = is_stream_readable();

            if (r == -1) {
                if (errno == EINTR) continue;
                throw runtime_error("Waiting for frame exception");
            }

            if (r == 0) {
                throw runtime_error("Select timeout exception");
            }

            read_frame(); // read frame (dequeue and queue)
        }
        else
        {
            /*
             * NOTE: This is used for the purpose of less CPU consuming
             */
            this_thread::sleep_for(chrono::milliseconds(10));
        }

    }

}

// ======================================= //

int V4L2Device::getHandle() const {
    return _fd;
}

string V4L2Device::getDevice() const {
    return _parameters.dev_name;
}

const v4l2_capability& V4L2Device::getCapability() const {
    return _capability;
}

const v4l2_format& V4L2Device::getFormat() const {
    return _format;
}

const v4l2_streamparm& V4L2Device::getStreamParameters() const {
    return _stream_parameters;
}

unsigned int V4L2Device::getWidth() const {
    return _format.fmt.pix.width;
}

unsigned int V4L2Device::getHeight() const {
    return _format.fmt.pix.height;
}

unsigned int V4L2Device::getStride() const {
    return _format.fmt.pix.bytesperline;
}

unsigned int V4L2Device::getImageSize() const {
    return _format.fmt.pix.sizeimage;
}

unsigned int V4L2Device::getPixelField() const {
    return _format.fmt.pix.field;
}

void V4L2Device::setCallback(const function<void (const Buffer&, const struct v4l2_buffer&)> &callback) {
    _callback = callback;
}

void V4L2Device::printInfo() {

    cout << "===============" << _parameters.dev_name << "==================" << endl;

    printf( "Driver Caps:\n"
            "  Driver: \"%s\"\n"
            "  Card: \"%s\"\n"
            "  Bus: \"%s\"\n"
            "  Version: %d.%d\n"
            "  Capabilities: %08x\n",
            _capability.driver,
            _capability.card,
            _capability.bus_info,
            (_capability.version >> 16) && 0xff,
            (_capability.version >> 24) && 0xff,
            _capability.capabilities);

    cout << "=================================" << endl;

    struct v4l2_fmtdesc fmtdesc = {0};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    char fourcc[5] = {0};
    char c, e;

    printf("FMT : CE Desc\n--------------------\n");

    while(v4l2_ioctl(_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0)
    {
        strncpy(fourcc, (char*)&fmtdesc.pixelformat, 4);
        c = fmtdesc.flags & 1? 'C' : ' ';
        e = fmtdesc.flags & 2? 'E' : ' ';
        printf("\t%s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
        fmtdesc.index++;
    }

    cout << "=================================" << endl;

    strncpy(fourcc, (char *)&_format.fmt.pix.pixelformat, 4);

    printf( "Selected Camera Mode:\n"
            "  Width: %d\n"
            "  Height: %d\n"
            "  PixFmt: %s\n"
            "  Field: %d\n",
            _format.fmt.pix.width,
            _format.fmt.pix.height,
            fourcc,
            _format.fmt.pix.field);

    cout << "=================================" << endl;

    printf("Camera's fps: %d/%d\n", _stream_parameters.parm.capture.timeperframe.denominator,
           _stream_parameters.parm.capture.timeperframe.numerator);

    cout << "=================================" << endl;

    printf("Buffers number: %d\n", _parameters.n_buffers);
}
