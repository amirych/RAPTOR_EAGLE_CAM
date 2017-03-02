#include <eagle_camera.h>

EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES()
{
    EagleCamera::camera_feature_map_t features;


    features["HBIN"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "HBIN", EagleCamera::ReadWrite, {1, 64}) );

    features["VBIN"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "VBIN", EagleCamera::ReadWrite, {1, 64}) );

    features["ROILeft"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROILeft",
                    EagleCamera::ReadWrite, {1, -1}) );

    features["ROITop"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROITop",
                    EagleCamera::ReadWrite, {1, -1}) );

    features["ROIWidth"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROIWidth",
                    EagleCamera::ReadWrite, {0, -1}) );

    features["ROIHeight"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROIHeight",
                    EagleCamera::ReadWrite, {0, -1}) );

    // exposure time in seconds. min is 25nsec (1 count in FPGA), but it is no real min!!!
    features["ExposureTime"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ExposureTime",
                    EagleCamera::ReadWrite, {2.5E-8, 27487.7906944}) );

    // in frames per second (formal range!!!)
    features["FrameRate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "FrameRate",
                    EagleCamera::ReadWrite, {3.63797880709e-05, 4.0E7}) );

    // in milliseconds
    features["ShutterOpenDelay"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ShutterOpenDelay",
                    EagleCamera::ReadWrite, {0.0, 419.43}) );

    // in milliseconds
    features["ShutterCloseDelay"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ShutterCloseDelay",
                    EagleCamera::ReadWrite, {0.0, 419.43}) );

    // in Celsius degrees (formal range)
    features["TECSetPoint"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "TECSetPoint",
                    EagleCamera::ReadWrite, {-110.0, 100.0}) );


    features["ShutterState"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ShutterState",
                    EagleCamera::ReadWrite, {"CLOSED", "OPEN", "EXP"}) );


    features["TECState"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "TECState",
                    EagleCamera::ReadWrite, {"OFF", "ON"}) );

    features["PreAmpGain"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "PreAmpGain",
                    EagleCamera::ReadWrite, {"HIGH", "LOW"}) );

    features["ReadoutRate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ReadoutRate",
                    EagleCamera::ReadWrite, {"FAST", "SLOW"}) );

    features["ReadoutMode"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ReadoutMode",
                    EagleCamera::ReadWrite, {"NORMAL", "TEST"}) );


                    // read-only features (no range)

    features["CCDTemperature"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "CCDTemperature",
                    EagleCamera::ReadOnly, {}) );

    features["PCBTemperature"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "PCBTemperature",
                    EagleCamera::ReadOnly, {}) );

    features["ADC_CALIB_0"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ADC_CALIB_0",
                    EagleCamera::ReadOnly, {}) );

    features["ADC_CALIB_1"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ADC_CALIB_1",
                    EagleCamera::ReadOnly, {}) );

    features["DAC_CALIB_0"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "DAC_CALIB_0",
                    EagleCamera::ReadOnly, {}) );

    features["DAD_CALIB_1"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "DAC_CALIB_1",
                    EagleCamera::ReadOnly, {}) );

    features["SerialNumber"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "SerialNumber",
                    EagleCamera::ReadOnly, {}) );

    features["BuildDate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "BuildDate",
                    EagleCamera::ReadOnly, {}) );

    features["BuildCode"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "BuildCode",
                    EagleCamera::ReadOnly, {}) );

    features["FPGAVersion"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "FPGAVersion",
                    EagleCamera::ReadOnly, {}) );

    features["MicroVersion"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "MicroVersion",
                    EagleCamera::ReadOnly, {}) );


    return features;
}
