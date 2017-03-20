#include <eagle_camera.h>

#include<iostream>
#include<string>
#include<map>
#include <exception>
#include <cstring>
#include <csignal>

std::map<std::string, std::string> cmd_map =
{
    {"-e",EAGLE_CAMERA_FEATURE_EXPTIME_NAME},         // exp time
    {"-s",EAGLE_CAMERA_FEATURE_SHUTTER_STATE_NAME},   // shutter state
    {"-f",EAGLE_CAMERA_FEATURE_FRAME_COUNTS_NAME},
    {"-bx",EAGLE_CAMERA_FEATURE_HBIN_NAME},
    {"-by",EAGLE_CAMERA_FEATURE_VBIN_NAME},
    {"-x",EAGLE_CAMERA_FEATURE_ROI_LEFT_NAME},
    {"-y",EAGLE_CAMERA_FEATURE_ROI_TOP_NAME},
    {"-w",EAGLE_CAMERA_FEATURE_ROI_WIDTH_NAME},
    {"-h",EAGLE_CAMERA_FEATURE_ROI_HEIGHT_NAME},
    {"-g",EAGLE_CAMERA_FEATURE_PREAMP_GAIN_NAME},
    {"-r",EAGLE_CAMERA_FEATURE_READOUT_RATE_NAME},
    {"-fh",EAGLE_CAMERA_FEATURE_FITS_HDR_FILENAME_NAME},
    {"-ff",EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME}
};


static EagleCamera *camera_ptr;

void signal_hndl(int signal)
{
    (*camera_ptr)("EXPSTOP");
}


int main(int argc, char* argv[])
{
    double val;

    std::signal(SIGINT, signal_hndl);

    try {
//        EagleCamera cam("/home/timur/f.fmt");
        EagleCamera cam;
        camera_ptr = &cam;
        cam.setLogLevel(EagleCamera::LOG_LEVEL_ERROR);
        cam.initCamera(1, &std::cout);

        cam[EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME] = "EXTEN";

        if ( argc > 1 ) {
            for ( int i = 1; i < argc; ++i ) {
                auto search = cmd_map.find(argv[i]);
                if ( search != cmd_map.end() ) {
//                    std::cout << "OPT: '" << argv[i] << "'\n";
                    std::cout << "FEATURE: '" << cmd_map[argv[i]] << "' = ";
                    if ( (i+1) > argc ) {
                        std::cerr << "INVALID ARGUMENT!\n";
                        return 1;
                    }
                    std::cout << argv[i+1] << "\n";
                    try {
                        val = std::stod(argv[i+1]);
                        cam[search->second] = val;
                    } catch ( std::invalid_argument &ex ) {
                        cam[search->second] = argv[i+1];
                    }
                    ++i;
                } else {
                    if ( !strcmp(argv[i],"-v") ) {
                        cam.setLogLevel(EagleCamera::LOG_LEVEL_VERBOSE);
                    }

                    if ( !strcmp(argv[i],"-c") ) {
                        cam[EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME] = "CUBE";
                        std::cout << "FEATURE: '" << EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME << "' = CUBE\n";
                    }
                }
            }
        }

        EagleCamera_StringFeature sf = cam[EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME];
        std::cout << "\nFITS FILENAME: " << sf.value() << "\n";
        cam.startAcquisition();
    } catch ( EagleCameraException &ex ) {
        std::cerr << "ERROR: xclib = " << ex.XCLIB_Error() << ", cam_err = " << ex.Camera_Error() << "\n";
        std::cerr << "ERR MSG: " << ex.what() << "\n";
        return ex.Camera_Error();
    } catch (...) {
        std::cerr << "ERROR!\n";
        return -1;
    }

    return 0;
}
