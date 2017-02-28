#include <eagle_camera.h>


                        /**************************************************
                        *    RAPTOR EAGLE-V CAMERA FEATURES DEFINITIONS   *
                        **************************************************/

//
//  Names are based on EAGLE V 4240 Instruction Manual (Revision 1.1)
//


EagleCamera::EagleCameraFeatureNameMap EAGLE_CAMERA_DEFAULT_FEATURE_NAME = {
    {"HBIN", EagleCamera::IntType},
    {"VBIN", EagleCamera::IntType},
    {"ROILeft", EagleCamera::IntType}, // x-coordinate of left-top ROI rectangle (in CCD pixels, starting from 1)
    {"ROITop", EagleCamera::IntType}, // y-coordinate of left-top ROI rectangle (in CCD pixels, starting from 1)
    {"ROIWidth", EagleCamera::IntType}, // width of ROI rectangle (in binned pixels)
    {"ROIHeight", EagleCamera::IntType}, // height of ROI rectangle (in binned pixels)

    {"ExposureTime", EagleCamera::FloatType}, // exposure time in seconds
    {"FrameRate", EagleCamera::FloatType}
};
