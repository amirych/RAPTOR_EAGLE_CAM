#include "eagle_camera.h"
#include <eagle_camera_feature_defs.h>

extern EagleCameraFeatureNameMap EAGLE_CAMERA_DEFINED_FEATURE;


EagleCamera::EagleCamera(const char *epix_video_fmt_filename)
{

}


EagleCamera::EagleCamera(const std::string & epix_video_fmt_filename)
{
    EAGLE_CAMERA_DEFINED_FEATURE["HBIN"]._float_range[2] = 10.0;
    EAGLE_CAMERA_DEFINED_FEATURE["HBIN"]._float_range[2] = 100.0;

}


EagleCamera::~EagleCamera()
{

}


EagleCamera::EagleCameraFeature & EagleCamera::operator [](const std::string & name)
{
    auto search = EAGLE_CAMERA_DEFINED_FEATURE.find(name);

    if ( search != EAGLE_CAMERA_DEFINED_FEATURE.end() ) {
        cameraFeature.setType(EAGLE_CAMERA_DEFINED_FEATURE[name]._type);
        cameraFeature.setAccess(EAGLE_CAMERA_DEFINED_FEATURE[name]._access);
    }

    return cameraFeature;
}


EagleCamera::EagleCameraFeature & EagleCamera::operator[](const char* name)
{
    operator [](std::string(name));
}
