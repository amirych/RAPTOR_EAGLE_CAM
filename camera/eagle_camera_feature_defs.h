#ifndef EAGLE_CAMERA_FEATURE_DEFS_H
#define EAGLE_CAMERA_FEATURE_DEFS_H

#include <eagle_camera.h>
#include <list>
#include <string>
#include <functional>

struct EagleCameraFeatureInfo {
    EagleCamera::EagleCameraFeatureType _type;
    EagleCamera::EagleCameraFeatureAccess _access;
//    union {
        int64_t _int_range[2];
        double _float_range[2];
//    };
    std::list<std::string> _string_range;
};

typedef std::map<std::string,EagleCameraFeatureInfo> EagleCameraFeatureNameMap;


//typedef std::function<> get_func_t;
//typedef std::function<> set_func_t;

class EagleCameraFeature
{
public:
    EagleCameraFeature(const EagleCamera::EagleCameraFeatureType type,
                       const EagleCamera::EagleCameraFeatureAccess access);
protected:
    EagleCamera::EagleCameraFeatureType _type;
    EagleCamera::EagleCameraFeatureAccess _access;
};

#endif // EAGLE_CAMERA_FEATURE_DEFS_H
