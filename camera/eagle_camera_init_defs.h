#ifndef EAGLE_CAMERA_INIT_DEFS_H
#define EAGLE_CAMERA_INIT_DEFS_H


#define EAGLE_CAMERA_FEATURE_HBIN_NAME                  "HBin"
#define EAGLE_CAMERA_FEATURE_VBIN_NAME                  "VBin"
#define EAGLE_CAMERA_FEATURE_ROI_LEFT_NAME              "ROILeft"
#define EAGLE_CAMERA_FEATURE_ROI_TOP_NAME               "ROITop"
#define EAGLE_CAMERA_FEATURE_ROI_WIDTH_NAME             "ROIWidth"
#define EAGLE_CAMERA_FEATURE_ROI_HEIGHT_NAME            "ROIHeight"
#define EAGLE_CAMERA_FEATURE_EXPTIME_NAME               "ExposureTime"
#define EAGLE_CAMERA_FEATURE_FRAME_RATE_NAME            "FrameRate"
#define EAGLE_CAMERA_FEATURE_SHUTTER_OPEN_DELAY_NAME    "ShutterOpenDelay"
#define EAGLE_CAMERA_FEATURE_SHUTTER_CLOSE_DELAY_NAME   "ShutterCloseDelay"
#define EAGLE_CAMERA_FEATURE_TEC_SET_POINT_NAME         "TECSetPoint"

#define EAGLE_CAMERA_FEATURE_CCD_TEMP_NAME   "CCDTemperature"
#define EAGLE_CAMERA_FEATURE_PCB_TEMP_NAME   "PCBTemperature"

#define EAGLE_CAMERA_FEATURE_ADC_CALIB0_NAME   "ADC_CALIB_0"
#define EAGLE_CAMERA_FEATURE_ADC_CALIB1_NAME   "ADC_CALIB_1"
#define EAGLE_CAMERA_FEATURE_DAC_CALIB0_NAME   "DAC_CALIB_0"
#define EAGLE_CAMERA_FEATURE_DAC_CALIB1_NAME   "DAC_CALIB_1"

#define EAGLE_CAMERA_FEATURE_SERIAL_NUMBER_NAME   "SerialNumber"
#define EAGLE_CAMERA_FEATURE_BUILD_DATE_NAME      "BuildDate"
#define EAGLE_CAMERA_FEATURE_BUILD_CODE_NAME      "BuildCode"
#define EAGLE_CAMERA_FEATURE_FPGA_VERSION_NAME    "FPGAVersion"
#define EAGLE_CAMERA_FEATURE_MICRO_VERSION_NAME   "MicroVersion"

#define EAGLE_CAMERA_FEATURE_FRAME_COUNTS_NAME        "FrameCounts"
#define EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME       "FitsFilename"
#define EAGLE_CAMERA_FEATURE_FITS_HDR_FILENAME_NAME   "FitsHdrFilename"



                    /***************************************************
                    *                                                  *
                    *   DEFINITIONS FOR STRING-TYPE CAMERA FEATURES    *
                    *                                                  *
                    ***************************************************/


                    /*    "ShutterState"     */

#define EAGLE_CAMERA_FEATURE_SHUTTER_STATE_NAME   "ShutterState"
#define EAGLE_CAMERA_FEATURE_SHUTTER_STATE_CLOSED "CLOSED"  // permanently closed
#define EAGLE_CAMERA_FEATURE_SHUTTER_STATE_OPEN   "OPEN"    // permanently open
#define EAGLE_CAMERA_FEATURE_SHUTTER_STATE_EXP    "EXP"     // open during exposure, closed otherwise


                    /*     "TECState"     */

#define EAGLE_CAMERA_FEATURE_TEC_STATE_NAME "TECState"
#define EAGLE_CAMERA_FEATURE_TEC_STATE_ON   "ON"     // thermoelectrical cooler is on
#define EAGLE_CAMERA_FEATURE_TEC_STATE_OFF  "OFF"    // thermoelectrical cooler is off


                    /*     "PreAmpGain"     */

#define EAGLE_CAMERA_FEATURE_PREAMP_GAIN_NAME  "PreAmpGain"
#define EAGLE_CAMERA_FEATURE_PREAMP_GAIN_HIGH  "HIGH"
#define EAGLE_CAMERA_FEATURE_PREAMP_GAIN_LOW   "LOW"


                    /*     "ReadoutRate"     */

#define EAGLE_CAMERA_FEATURE_READOUT_RATE_NAME "ReadoutRate"
#define EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST "FAST"
#define EAGLE_CAMERA_FEATURE_READOUT_RATE_SLOW "SLOW"


                    /*     "ReadoutMode"     */

#define EAGLE_CAMERA_FEATURE_READOUT_MODE_NAME    "ReadoutMode"
#define EAGLE_CAMERA_FEATURE_READOUT_MODE_NORMAL  "NORMAL"
#define EAGLE_CAMERA_FEATURE_READOUT_MODE_TEST    "TEST"


                    /*     "FitsDataFormat"     */

#define EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_NAME   "FitsDataFormat"
#define EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_EXTEN  "EXTEN"  // write frames into separate IMAGE-extensions
#define EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_CUBE   "CUBE"   // write frames into primary array as a 3D cube


#endif // EAGLE_CAMERA_INIT_DEFS_H
