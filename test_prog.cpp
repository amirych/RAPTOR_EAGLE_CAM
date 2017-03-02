#include <iostream>

#include <eagle_camera.h>

int main(int argc, char* argv[])
{
    EagleCamera cam;

    cam("INIT", 23, nullptr);

    return 0;
}
