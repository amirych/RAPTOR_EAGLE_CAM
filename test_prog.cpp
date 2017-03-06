#include <iostream>

#include <eagle_camera.h>

using namespace std;

int main(int argc, char* argv[])
{
    try {
    EagleCamera cam("/home/timur/raptor_eagle-v.fmt");
    if ( argc > 1 ) cam.setLogLevel(EagleCamera::LOG_LEVEL_ERROR);

//    cam("INIT", 23, nullptr);

    cam.initCamera(1,&std::cout);

//    cout << cam.getSerialNumber() << "\n";
//    cout << cam.getBuildDate() << "\n";

    cam["ExposureTime"] = 17.73;
    cout << (double)cam["ExposureTime"] << "\n";

    cam["TECState"] = "OFF";

    EagleCamera_StringFeature tt = cam["TECState"];

    cout << tt.value() << "\n";

    } catch ( EagleCameraException &ex ) {
        cout << "ERROR: " << ex.what() << "\n";
    }

    return 0;
}
