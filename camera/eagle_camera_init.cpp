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
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_HBIN_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_HBIN_NAME,
                    EagleCamera::ReadWrite, {1, EAGLE_CAMERA_MAX_XBIN},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getXBIN), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setXBIN), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_VBIN_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_VBIN_NAME,
                    EagleCamera::ReadWrite, {1, EAGLE_CAMERA_MAX_YBIN},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getYBIN), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setYBIN), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ROI_LEFT_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ROI_LEFT_NAME,
                    EagleCamera::ReadWrite, {1, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROILeft), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROILeft), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ROI_TOP_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ROI_TOP_NAME,
                    EagleCamera::ReadWrite, {1, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROITop), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROITop), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ROI_WIDTH_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ROI_WIDTH_NAME,
                    EagleCamera::ReadWrite, {0, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROIWidth), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROIWidth), this, std::placeholders::_1)
                ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ROI_HEIGHT_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ROI_HEIGHT_NAME,
                    EagleCamera::ReadWrite, {0, -1},
                    std::bind(static_cast<IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getROIHeight), this),
                    std::bind(static_cast<void(EagleCamera::*)(const IntegerType)>
                    (&EagleCamera::setROIHeight), this, std::placeholders::_1)
                ));

    // exposure time in seconds. min is 25nsec (1 count in FPGA), but it is no real min!!!
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_EXPTIME_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_EXPTIME_NAME,
                    EagleCamera::ReadWrite, {2.5E-8, 27487.7906944},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getExpTime), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setExpTime), this, std::placeholders::_1)
                ));

    // in frames per second (formal range!!!)
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FRAME_RATE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_FRAME_RATE_NAME,
                    EagleCamera::ReadWrite, {3.63797880709e-05, 4.0E7},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getFrameRate), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setFrameRate), this, std::placeholders::_1)
                ));

    // in milliseconds
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_SHUTTER_OPEN_DELAY_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_SHUTTER_OPEN_DELAY_NAME,
                    EagleCamera::ReadWrite, {0.0, 419.43},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getShutterOpenDelay), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setShutterOpenDelay), this, std::placeholders::_1)
                ));

    // in milliseconds
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_SHUTTER_CLOSE_DELAY_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_SHUTTER_CLOSE_DELAY_NAME,
                    EagleCamera::ReadWrite, {0.0, 419.43},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getShutterCloseDelay), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setShutterCloseDelay), this, std::placeholders::_1)
                ));

    // in Celsius degrees (formal range)
    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_TEC_SET_POINT_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_TEC_SET_POINT_NAME,
                    EagleCamera::ReadWrite, {-110.0, 100.0},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getTEC_SetPoint), this),
                    std::bind(static_cast<void(EagleCamera::*)(const double)>
                    (&EagleCamera::setTEC_SetPoint), this, std::placeholders::_1)
                ));


    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_SHUTTER_STATE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_SHUTTER_STATE_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_SHUTTER_STATE_CLOSED,
                                             EAGLE_CAMERA_FEATURE_SHUTTER_STATE_OPEN,
                                             EAGLE_CAMERA_FEATURE_SHUTTER_STATE_EXP},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getShutterState), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setShutterState), this, std::placeholders::_1)
               ));


    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_TEC_STATE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_TEC_STATE_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_TEC_STATE_OFF, EAGLE_CAMERA_FEATURE_TEC_STATE_ON},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getTECState), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setTECState), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_PREAMP_GAIN_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_PREAMP_GAIN_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_PREAMP_GAIN_HIGH, EAGLE_CAMERA_FEATURE_PREAMP_GAIN_LOW},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getPreAmpGain), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setPreAmpGain), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_READOUT_RATE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_READOUT_RATE_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST, EAGLE_CAMERA_FEATURE_READOUT_RATE_SLOW},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getReadoutRate), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setReadoutRate), this, std::placeholders::_1)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_READOUT_MODE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_READOUT_MODE_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_READOUT_MODE_NORMAL, EAGLE_CAMERA_FEATURE_READOUT_MODE_TEST},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getReadoutMode), this),
                    std::bind(static_cast<void(EagleCamera::*)(const std::string)>
                    (&EagleCamera::setReadoutMode), this, std::placeholders::_1)
               ));


                    // read-only features (no range)

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_CCD_TEMP_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_CCD_TEMP_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getCCDTemp), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_PCB_TEMP_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<double>( EAGLE_CAMERA_FEATURE_PCB_TEMP_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<double(EagleCamera::*)()>
                    (&EagleCamera::getPCBTemp), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ADC_CALIB0_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ADC_CALIB0_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<EagleCamera::IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getADC_CALIB_0), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_ADC_CALIB1_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_ADC_CALIB1_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<EagleCamera::IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getADC_CALIB_1), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_DAC_CALIB0_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_DAC_CALIB0_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<EagleCamera::IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getDAC_CALIB_0), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_DAC_CALIB1_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_DAC_CALIB1_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<EagleCamera::IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getDAC_CALIB_1), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_SERIAL_NUMBER_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_SERIAL_NUMBER_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<EagleCamera::IntegerType(EagleCamera::*)()>
                    (&EagleCamera::getSerialNumber), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_BUILD_DATE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_BUILD_DATE_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getBuildDate), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_BUILD_CODE_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_BUILD_CODE_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getBuildCode), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FPGA_VERSION_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_FPGA_VERSION_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getFPGAVersion), this)
               ));

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_MICRO_VERSION_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_MICRO_VERSION_NAME,
                    EagleCamera::ReadOnly, {},
                    std::bind(static_cast<std::string(EagleCamera::*)()>
                    (&EagleCamera::getMicroVersion), this)
               ));

                    // non-hardware features

    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FRAME_COUNTS_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<EagleCamera::IntegerType>( EAGLE_CAMERA_FEATURE_FRAME_COUNTS_NAME,
                    EagleCamera::ReadWrite, {1,std::numeric_limits<IntegerType>::max()},
                    [this]() {return _frameCounts;},
                    [this](const EagleCamera::IntegerType fc){_frameCounts = fc;}
               ));


    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME,
                    EagleCamera::ReadWrite, {},
                    [this]() {return _fitsFilename;},
                    [this](const std::string fn){_fitsFilename = fn;}
               ));


    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FITS_HDR_FILENAME_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_FITS_HDR_FILENAME_NAME,
                    EagleCamera::ReadWrite, {},
                    [this]() {return _fitsHdrFilename;},
                    [this](const std::string fhn){_fitsHdrFilename = fhn;}
               ));


    PREDEFINED_CAMERA_FEATURES[EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME] = std::unique_ptr<EagleCamera::CameraAbstractFeature>(
            new EagleCamera::CameraFeature<std::string>( EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME,
                    EagleCamera::ReadWrite, {EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_EXTEN, EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_CUBE},
                    [this]() {return _fitsDataFormat;},
                    [this](const std::string ff){_fitsDataFormat = ff;}
               ));


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
