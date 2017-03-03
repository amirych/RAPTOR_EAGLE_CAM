#include <iostream>

#include <eagle_camera.h>

using namespace std;

int main(int argc, char* argv[])
{
    EagleCamera cam;

//    cam("INIT", 23, nullptr);

    cam.initCamera(1,&std::cout);

//    cout << cam.getSerialNumber() << "\n";
//    cout << cam.getBuildDate() << "\n";

    return 0;
}
