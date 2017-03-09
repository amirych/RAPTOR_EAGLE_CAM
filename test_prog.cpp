#include <iostream>

#include <eagle_camera.h>

using namespace std;

int main(int argc, char* argv[])
{
    try {
        EagleCamera cam("/home/timur/raptor_eagle-v.fmt");
        if ( argc > 1 ) cam.setLogLevel(EagleCamera::LOG_LEVEL_ERROR);

        cam.initCamera(1,&std::cout);

        cam["ExposureTime"] = 1.73;
        cout << (double)cam["ExposureTime"] << "\n";

        cam["TECState"] = "OFF";

        EagleCamera_StringFeature tt;
        tt = cam["TECState"];

        cout << tt.value() << "\n";

        cam["FrameCount"] = 2;

        cam["FitsFilename"] = "z.fits";

        cam["ShutterState"] = "CLOSED";

        cam("EXPSTART");

        std::cout << "sleep ...\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    } catch ( EagleCameraException &ex ) {
        cout << "ERROR: " << ex.what() << "\n";
    }

    return 0;
}
