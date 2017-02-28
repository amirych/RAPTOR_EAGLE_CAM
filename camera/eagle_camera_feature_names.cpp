#include <eagle_camera_feature_defs.h>


                        /**************************************************
                        *    RAPTOR EAGLE-V CAMERA FEATURES DEFINITIONS   *
                        **************************************************/

//
//  Names are based on EAGLE V 4240 Instruction Manual (Revision 1.1)
//

EagleCameraFeatureNameMap zz = {
    { "ExposureTime", {EagleCamera::FloatType, EagleCamera::ReadWrite,
                       {0,0}, {10.0,20.0}, {"ONE", "TWO"}} },

};

EagleCameraFeatureNameMap EAGLE_CAMERA_DEFINED_FEATURE = {
    { "HBIN", {EagleCamera::IntType, EagleCamera::ReadWrite, {1,64}} },
    { "VBIN", {EagleCamera::IntType, EagleCamera::ReadWrite, {1,64}} },

        // actuall range for ROI coordinates and size will be defined later
    { "ROILeft", {EagleCamera::IntType, EagleCamera::ReadWrite} }, // x-coordinate of left-top ROI rectangle
                                                                   // (in CCD pixels, starting from 1)

    { "ROITop", {EagleCamera::IntType, EagleCamera::ReadWrite} },   // y-coordinate of left-top ROI rectangle
                                                                    // (in CCD pixels, starting from 1)

    { "ROIWidth", {EagleCamera::IntType, EagleCamera::ReadWrite} }, // width of ROI rectangle (in binned pixels)
    { "ROIHeight", {EagleCamera::IntType, EagleCamera::ReadWrite} }, // height of ROI rectangle (in binned pixels)

    { "ExposureTime", {EagleCamera::FloatType, EagleCamera::ReadWrite, {0,0},
                       {2.5E-8, 27487.7906944} }
    }, // exposure time in seconds. min is 25nsec (1 count in FPGA), but it is no real min!!!

    { "FrameRate", {EagleCamera::FloatType, EagleCamera::ReadWrite, {0,0},
                    {3.63797880709e-05, 4.0E7} }
    }, // in frames per second (formal range!!!)

    { "ShutterOpenDelay", {EagleCamera::FloatType, EagleCamera::ReadWrite, {0,0},
                           {0.0, 419.43} }
    },  // in milliseconds

    { "ShutterCloseDelay", {EagleCamera::FloatType, EagleCamera::ReadWrite, {0,0},
                            {0.0, 419.43} }
    }, // in milliseconds

    { "TECSetPoint", {EagleCamera::FloatType, EagleCamera::ReadWrite, {0,0},
                      {-110.0, 100.0} }
    }, // in Celsius degrees (formal range)

    { "ShutterState", {EagleCamera::StringType, EagleCamera::ReadWrite, {0,0}, {0,0},
                       {"CLOSED", "OPEN", "EXP"} }
    },

    { "TECState", {EagleCamera::StringType, EagleCamera::ReadWrite, {0,0}, {0,0},
                   {"OFF", "ON"} }
    },

    { "PreAmpGain", {EagleCamera::StringType, EagleCamera::ReadWrite, {0,0}, {0,0},
                     {"HIGH", "LOW"} }
    },

    { "ReadoutRate", {EagleCamera::StringType, EagleCamera::ReadWrite, {0,0}, {0,0},
                      {"FAST", "SLOW"} }
    },

    { "ReadoutMode", {EagleCamera::StringType, EagleCamera::ReadWrite, {0,0}, {0,0},
                      {"NORMAL", "TEST"} }
    },

                        // read-only features (no range)

    { "CCDTemperature", {EagleCamera::FloatType, EagleCamera::ReadOnly} },    // in Celsius degrees
    { "PCBTemperature", {EagleCamera::FloatType, EagleCamera::ReadOnly} },    // in Celsius degrees

    { "ADC_CALIB_0", {EagleCamera::IntType, EagleCamera::ReadOnly} }, // ADC calibration data at 0 degrees
    { "ADC_CALIB_1", {EagleCamera::IntType, EagleCamera::ReadOnly} }, // ADC calibration data at 40 degrees
    { "DAC_CALIB_0", {EagleCamera::IntType, EagleCamera::ReadOnly} }, // DAC calibration data at 0 degrees
    { "DAC_CALIB_1", {EagleCamera::IntType, EagleCamera::ReadOnly} }, // DAC calibration data at 40 degrees

    { "SerialNumber", {EagleCamera::StringType, EagleCamera::ReadOnly} },
    { "BuildDate", {EagleCamera::StringType, EagleCamera::ReadOnly} },
    { "BuildCode", {EagleCamera::StringType, EagleCamera::ReadOnly} },
    { "FPGAVersion", {EagleCamera::StringType, EagleCamera::ReadOnly} },
    { "MicroVersion", {EagleCamera::StringType, EagleCamera::ReadOnly} }
};

