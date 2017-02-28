#include "eagle_camera.h"
#include <eagle_camera_feature_defs.h>

extern EagleCameraFeatureNameMap EAGLE_CAMERA_DEFAULT_FEATURE_NAME;


EagleCamera::EagleCamera(const char *epix_video_fmt_filename)
{

}


EagleCamera::EagleCamera(const std::string & epix_video_fmt_filename)
{
    EAGLE_CAMERA_DEFAULT_FEATURE_NAME["HBIN"]._float_range[2] = 10.0;
    EAGLE_CAMERA_DEFAULT_FEATURE_NAME["HBIN"]._float_range[2] = 100.0;

}
