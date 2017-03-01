#include <eagle_camera.h>

EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES()
{
    EagleCamera::camera_feature_map_t features;


    features["HBIN"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {1, 64}) );

    features["VBIN"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {1, 64}) );

    features["ROILeft"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {1, -1}) );

    features["ROITop"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {1, -1}) );

    features["ROIWidth"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {0, -1}) );

    features["ROIHeight"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EagleCamera::ReadWrite, {0, -1}) );

    return features;
}
