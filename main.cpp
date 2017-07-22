#include "v4l2device.h"

int main(int argc, char *argv[])
{
    V4L2Device dev;

//    dev.startCapturing();

    dev.printInfo();

    std::string token{0};

    while (true) {

        std::cin >> token;

        switch(std::atoi(token.c_str())) {
            case 1:
                dev.startCapturing();
                break;
            default:
                dev.stopCapturing();
        }
    }

    return 0;
}
