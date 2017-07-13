#include "v4l2device.h"
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{

    v4l2_device_param p;

    p.height = 720;
    p.width = 1280;
    p.n_buffers = 15;

    V4L2Device device(p);

    device.startCapturing();

    std::this_thread::sleep_for(std::chrono::seconds(4));

    device.stopCapturing();

    std::this_thread::sleep_for(std::chrono::seconds(4));

    device.startCapturing();

    std::this_thread::sleep_for(std::chrono::seconds(4));

    std::cout << "end" << std::endl;

    return 0;
}
