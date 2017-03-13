#include <iostream>

#include <eagle_camera.h>

using namespace std;

int main(int argc, char* argv[])
{
    try {
        EagleCamera cam("/home/timur/raptor_eagle-v.fmt");
        if ( argc > 1 ) {
            int fl = atoi(argv[1]);
            if ( fl == 0 ) {
                cam.setLogLevel(EagleCamera::LOG_LEVEL_ERROR);
            } else {
                cam.setLogLevel(EagleCamera::LOG_LEVEL_VERBOSE);
            }
        }

        cam.initCamera(1,&std::cout);

        cam["ExposureTime"] = 1.73;
        cout << (double)cam["ExposureTime"] << "\n";

        cam["TECState"] = "OFF";

        EagleCamera_StringFeature tt;
        tt = cam["TECState"];

        cout << tt.value() << "\n";

        if ( argc > 2 ) {
            cam["FrameCount"] = atoi(argv[2]);
        } else cam["FrameCount"] = 1;

        cam["FitsFilename"] = "z.fits";
        cam["FitsHdrFilename"] = "z.hdr";

        cam["ShutterState"] = "EXP";

        if ( argc > 3 ) {
            cam["FitsDataFormat"] = "CUBE";
        } else {
            cam["FitsDataFormat"] = "EXTEN";
        }

        cam("EXPSTART");
//        std::this_thread::sleep_for(std::chrono::milliseconds(6134));
//        cam("EXPSTOP");

    } catch ( EagleCameraException &ex ) {
        cout << "ERROR: " << ex.what() << "\n";
    }

    return 0;
}
