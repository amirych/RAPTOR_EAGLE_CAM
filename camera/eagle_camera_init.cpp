#include <eagle_camera.h>
#include <eagle_camera_config.h>


                        /*****************************************
                        *                                        *
                        *   EagleCamera CLASS IMPLEMENTATION:    *
                        *      initialization of camera          *
                        *        features and commands           *
                        *                                        *
                        *****************************************/



void EagleCamera::InitCameraFeatures()
{
    PREDEFINED_CAMERA_FEATURES["HBin"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "HBin", EagleCamera::ReadWrite, {1, EAGLE_CAMERA_MAX_XBIN},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getXBIN), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setXBIN), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES["VBin"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "VBin", EagleCamera::ReadWrite, {1, EAGLE_CAMERA_MAX_YBIN},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getYBIN), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setYBIN), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES["ROILeft"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROILeft",
                    EagleCamera::ReadWrite, {1, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROILeft), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROILeft), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES["ROITop"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROITop",
                    EagleCamera::ReadWrite, {1, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROITop), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROITop), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES["ROIWidth"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROIWidth",
                    EagleCamera::ReadWrite, {0, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROIWidth), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROIWidth), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES["ROIHeight"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ROIHeight",
                    EagleCamera::ReadWrite, {0, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROIHeight), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROIHeight), this, std::placeholders::_1)
                ));

    // exposure time in seconds. min is 25nsec (1 count in FPGA), but it is no real min!!!
    PREDEFINED_CAMERA_FEATURES["ExposureTime"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ExposureTime",
                    EagleCamera::ReadWrite, {2.5E-8, 27487.7906944},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getExpTime), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setExpTime), this, std::placeholders::_1)
                ));

    // in frames per second (formal range!!!)
    PREDEFINED_CAMERA_FEATURES["FrameRate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "FrameRate",
                    EagleCamera::ReadWrite, {3.63797880709e-05, 4.0E7},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getFrameRate), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setFrameRate), this, std::placeholders::_1)
                ));

    // in milliseconds
    PREDEFINED_CAMERA_FEATURES["ShutterOpenDelay"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ShutterOpenDelay",
                    EagleCamera::ReadWrite, {0.0, 419.43},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getShutterOpenDelay), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setShutterOpenDelay), this, std::placeholders::_1)
                ));

    // in milliseconds
    PREDEFINED_CAMERA_FEATURES["ShutterCloseDelay"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "ShutterCloseDelay",
                    EagleCamera::ReadWrite, {0.0, 419.43},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getShutterCloseDelay), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setShutterCloseDelay), this, std::placeholders::_1)
                ));

    // in Celsius degrees (formal range)
    PREDEFINED_CAMERA_FEATURES["TECSetPoint"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "TECSetPoint",
                    EagleCamera::ReadWrite, {-110.0, 100.0},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getTEC_SetPoint), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setTEC_SetPoint), this, std::placeholders::_1)
                ));


    PREDEFINED_CAMERA_FEATURES["ShutterState"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ShutterState",
                    EagleCamera::ReadWrite, {"CLOSED", "OPEN", "EXP"},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getShutterState), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setShutterState), this, std::placeholders::_1)
               ));


    PREDEFINED_CAMERA_FEATURES["TECState"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "TECState",
                    EagleCamera::ReadWrite, {"OFF", "ON"},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getTECState), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setTECState), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES["PreAmpGain"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "PreAmpGain",
                    EagleCamera::ReadWrite, {"HIGH", "LOW"},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getPreAmpGain), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setPreAmpGain), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES["ReadoutRate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ReadoutRate",
                    EagleCamera::ReadWrite, {"FAST", "SLOW"},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getReadoutRate), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setReadoutRate), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES["ReadoutMode"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "ReadoutMode",
                    EagleCamera::ReadWrite, {"NORMAL", "TEST"},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getReadoutMode), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setReadoutMode), this, std::placeholders::_1)
               ));


                    // read-only features (no range)

    PREDEFINED_CAMERA_FEATURES["CCDTemperature"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "CCDTemperature",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["PCBTemperature"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( "PCBTemperature",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["ADC_CALIB_0"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ADC_CALIB_0",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["ADC_CALIB_1"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "ADC_CALIB_1",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["DAC_CALIB_0"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "DAC_CALIB_0",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["DAC_CALIB_1"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "DAC_CALIB_1",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["SerialNumber"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( "SerialNumber",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["BuildDate"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "BuildDate",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["BuildCode"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "BuildCode",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["FPGAVersion"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "FPGAVersion",
                    EagleCamera::ReadOnly, {}) );

    PREDEFINED_CAMERA_FEATURES["MicroVersion"] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( "MicroVersion",
                    EagleCamera::ReadOnly, {}) );

}


void EagleCamera::InitCameraCommands()
{
    PREDEFINED_CAMERA_COMMANDS["INIT"] = std::unique_ptr<CameraAbstractCommand>(
                new CameraCommand<int, std::ostream*>( "INIT",
                                 std::bind(static_cast<void(EagleCamera::*)(const int, std::ostream*)>
                                 (&EagleCamera::initCamera), this, std::placeholders::_1, std::placeholders::_2))
                                                                               );
    PREDEFINED_CAMERA_COMMANDS["RESET"] = std::unique_ptr<CameraAbstractCommand>(
                new CameraCommand<>( "RESET", std::bind(static_cast<void(EagleCamera::*)()>
                                 (&EagleCamera::resetCamera), this)) );


    PREDEFINED_CAMERA_COMMANDS["EXPSTART"] = std::unique_ptr<CameraAbstractCommand>(
                new CameraCommand<>( "EXPSTART", std::bind(static_cast<void(EagleCamera::*)()>
                                 (&EagleCamera::startAcquisition), this)) );


    PREDEFINED_CAMERA_COMMANDS["EXPSTOP"] = std::unique_ptr<CameraAbstractCommand>(
                new CameraCommand<>( "EXPSTOP", std::bind(static_cast<void(EagleCamera::*)()>
                                 (&EagleCamera::stopAcquisition), this)) );

}
