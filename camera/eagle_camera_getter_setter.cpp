#include <eagle_camera.h>
#include <eagle_camera_init_defs.h>

#include <cmath>

                     /*******************************************
                     *                                          *
                     *    EagleCamera CLASS IMPLEMENTATION:     *
                     *      getter and setter functions         *
                     *                                          *
                     *******************************************/

/*
 *  NOTE:  User pixel coordinates are assumed to be in according
 *         to FITS-format, i.e. it start from 1
 *
 *         In contrast of inner camera representation
 *         BINNING values also start from 1 (not from 0).
 *
 *         The methods do not check user values for valid range!
 *         It is assumed that such procedure already performed in
 *         EagleCamera::CameraFeatureProxy methods! But methods for
 *         setup of ROI recompute and set ROI width and height
 *         if neccessary.
 *
 *         The start position of ROI must be in CCD pixels, while sizes
 *         are in binned pixels!
 *
*/


                            /*   geometry related methods    */

void EagleCamera::setGeometryValue(const EagleCamera::GeometryValueName name, const EagleCamera::IntegerType val)
{
    unsigned char v = val;
    byte_vector_t addr;
    byte_vector_t value;

    switch (name) {
        case EagleCamera::GV_XBIN:
            addr = {0xA1};
            value = {v};
            break;
        case EagleCamera::GV_YBIN:
            addr = {0xA2};
            value = {v};
            break;
        case EagleCamera::GV_ROILEFT:
            addr = {0xB6, 0xB7};
            value = integerToFPGA12Bits(val);
            break;
        case EagleCamera::GV_ROITOP:
            addr = {0xBA, 0xBB};
            value = integerToFPGA12Bits(val);
            break;
        case EagleCamera::GV_ROIWIDTH:
            addr = {0xB4, 0xB5};
            value = integerToFPGA12Bits(val);
            break;
        case EagleCamera::GV_ROIHEIGHT:
            addr = {0xB8, 0xB9};
            value = integerToFPGA12Bits(val);
            break;
        default:
            break;
    }

    writeRegisters(addr,value);
}


EagleCamera::IntegerType EagleCamera::getGeometryValue(const EagleCamera::GeometryValueName name)
{
    byte_vector_t addr;
    byte_vector_t value;

    switch (name) {
        case EagleCamera::GV_XBIN:
            addr = {0xA1};
            value = {0x0};
            break;
        case EagleCamera::GV_YBIN:
            addr = {0xA2};
            value = {0x0};
            break;
        case EagleCamera::GV_ROILEFT:
            addr = {0xB6, 0xB7};
            value = {0x0, 0x0};
            break;
        case EagleCamera::GV_ROITOP:
            addr = {0xBA, 0xBB};
            value = {0x0, 0x0};
            break;
        case EagleCamera::GV_ROIWIDTH:
            addr = {0xB4, 0xB5};
            value = {0x0, 0x0};
            break;
        case EagleCamera::GV_ROIHEIGHT:
            addr = {0xB8, 0xB9};
            value = {0x0, 0x0};
            break;
        default:
            break;
    }

    value = readRegisters(addr);

    return fpga16BitsToInteger(value);
}


void EagleCamera::setXBIN(const EagleCamera::IntegerType val)
{
    setGeometryValue(EagleCamera::GV_XBIN, val-1); // '-1' to convert FITS-format value to inner presentation

    // check and adjust ROI width

    IntegerType startX =  getGeometryValue(EagleCamera::GV_ROILEFT);
    IntegerType width =  getGeometryValue(EagleCamera::GV_ROIWIDTH);

    IntegerType max_w = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[0] - startX)/val));

    if ( max_w < width ) setGeometryValue(EagleCamera::GV_ROIWIDTH,max_w);
}


EagleCamera::IntegerType EagleCamera::getXBIN()
{
    return getGeometryValue(EagleCamera::GV_XBIN) + 1; // '+1' to convert from inner 0-based presentation
}


void EagleCamera::setYBIN(const EagleCamera::IntegerType val)
{
    setGeometryValue(EagleCamera::GV_YBIN, val-1); // '-1' to convert FITS-format to inner presentation

    // check and adjust ROI height

    IntegerType startY =  getGeometryValue(EagleCamera::GV_ROITOP);
    IntegerType height =  getGeometryValue(EagleCamera::GV_ROIHEIGHT);

    IntegerType max_h = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[1] - startY)/val));

    if ( max_h < height ) setGeometryValue(EagleCamera::GV_ROIHEIGHT,max_h);
}


EagleCamera::IntegerType EagleCamera::getYBIN()
{
    return getGeometryValue(EagleCamera::GV_YBIN) + 1; // '+1' to convert ifrom nner 0-based presentation
}


void EagleCamera::setROILeft(const EagleCamera::IntegerType val)
{
    setGeometryValue(EagleCamera::GV_ROILEFT, val-1); // '-1' to convert FITS-format value to inner presentation

    // check and adjust ROI width

    IntegerType binX =  getGeometryValue(EagleCamera::GV_XBIN);
    IntegerType width =  getGeometryValue(EagleCamera::GV_ROIWIDTH);

    IntegerType max_w = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[0] - val)/binX));

    if ( max_w < width ) setGeometryValue(EagleCamera::GV_ROIWIDTH,max_w);
}


EagleCamera::IntegerType EagleCamera::getROILeft()
{
    return getGeometryValue(EagleCamera::GV_ROILEFT) + 1; // '+1' to convert inner presentation to FITS-format one
}


void EagleCamera::setROITop(const EagleCamera::IntegerType val)
{
    setGeometryValue(EagleCamera::GV_ROITOP, val-1); // '-1' to convert FITS-format value to inner presentation

    // check and adjust ROI height

    IntegerType binY =  getGeometryValue(EagleCamera::GV_YBIN);
    IntegerType height =  getGeometryValue(EagleCamera::GV_ROIHEIGHT);

    IntegerType max_h = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[1] - val)/binY));

    if ( max_h < height ) setGeometryValue(EagleCamera::GV_ROIHEIGHT,max_h);
}


EagleCamera::IntegerType EagleCamera::getROITop()
{
    return getGeometryValue(EagleCamera::GV_ROITOP) + 1; // '+1' to convert inner presentation to FITS-format one
}


void EagleCamera::setROIWidth(const EagleCamera::IntegerType val)
{
    IntegerType binX = getGeometryValue(EagleCamera::GV_XBIN);
    IntegerType startX = getGeometryValue(EagleCamera::GV_ROILEFT);

    IntegerType max_w = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[0]-startX)/binX));

    if ( max_w < val ) setGeometryValue(EagleCamera::GV_ROIWIDTH, max_w);
    else setGeometryValue(EagleCamera::GV_ROIWIDTH, val);
}


EagleCamera::IntegerType EagleCamera::getROIWidth()
{
    return getGeometryValue(EagleCamera::GV_ROIWIDTH);
}

void EagleCamera::setROIHeight(const EagleCamera::IntegerType val)
{
    IntegerType binY = getGeometryValue(EagleCamera::GV_YBIN);
    IntegerType startY = getGeometryValue(EagleCamera::GV_ROITOP);

    IntegerType max_h = static_cast<IntegerType>(ceil(1.0*(_ccdDimension[1]-startY)/binY));

    if ( max_h < val ) setGeometryValue(EagleCamera::GV_ROIHEIGHT, max_h);
    else setGeometryValue(EagleCamera::GV_ROIHEIGHT, val);
}


EagleCamera::IntegerType EagleCamera::getROIHeight()
{
    return getGeometryValue(EagleCamera::GV_ROIHEIGHT);
}



                                    /*   shutter control related methods    */

#define SHUTTER_DELAY_PERIOD 1.6384E-3 // in seconds

void EagleCamera::setShutterState(const std::string val)
{
    unsigned char v;
    if ( !val.compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_CLOSED) )  {
        v = 0x0;
    } else if ( !val.compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_OPEN) )  {
        v = 0x1;
    } else if ( !val.compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_EXP) )  {
        v = 0x2;
    }

    writeRegisters({0xA5},{v});
}


std::string EagleCamera::getShutterState()
{
    byte_vector_t val(1);
    std::string value;

    readRegisters({0xA5},val);

    switch (val[0]) {
        case 0x0:
            value = EAGLE_CAMERA_FEATURE_SHUTTER_STATE_CLOSED;
            break;
        case 0x1:
            value = EAGLE_CAMERA_FEATURE_SHUTTER_STATE_OPEN;
            break;
        case 0x2:
            value = EAGLE_CAMERA_FEATURE_SHUTTER_STATE_EXP;
            break;
        default:
            logMessageStream.str("");
            logMessageStream << "Unexpected FPGA register value for shutter state (got"  << std::hex
                             << (int)val[0] << std::dec << ")";
            throw EagleCameraException(0,EagleCamera::Error_UnexpectedFPGAValue,logMessageStream.str());
    }

    return value;
}


void EagleCamera::setShutterOpenDelay(const double val)
{
    unsigned char v = static_cast<unsigned char>(ceil(val/SHUTTER_DELAY_PERIOD));

    writeRegisters({0xA6},{v});
}


double EagleCamera::getShutterOpenDelay()
{
    byte_vector_t val(1);

    val = readRegisters({0xA6});

    return SHUTTER_DELAY_PERIOD*val[0];
}


void EagleCamera::setShutterCloseDelay(const double val)
{
    unsigned char v = static_cast<unsigned char>(ceil(val/SHUTTER_DELAY_PERIOD));

    writeRegisters({0xA7},{v});
}


double EagleCamera::getShutterCloseDelay()
{
    byte_vector_t val(1);

    readRegisters({0xA7},val);

    return SHUTTER_DELAY_PERIOD*val[0];
}



                        /*  CCD temperature control related methods   */

void EagleCamera::setTECState(const std::string val)
{
    unsigned char ctrl_reg = getCtrlRegister();
    bool flag;

    if ( !val.compare(EAGLE_CAMERA_FEATURE_TEC_STATE_ON) ) {
        flag = true;
    } else {
        flag = false;
    }

    setCtrlRegister(is_high_gain_enabled(ctrl_reg),is_reset_temp_trip(ctrl_reg), flag);
}


std::string EagleCamera::getTECState()
{
    unsigned char ctrl_reg = getCtrlRegister();
    std::string val = EAGLE_CAMERA_FEATURE_TEC_STATE_OFF;

    if ( is_tec_enabled(ctrl_reg) ) val = EAGLE_CAMERA_FEATURE_TEC_STATE_ON;

    return val;
}


void EagleCamera::setTEC_SetPoint(const double temp)
{
    byte_vector_t addr = {0x03, 0x04};
    byte_vector_t value(2);

    IntegerType counts = static_cast<uint16_t>(ceil((temp-ADC_LinearCoeffs[0])/ADC_LinearCoeffs[1]));

    value = integerToFPGA12Bits(counts);

//    value[0] = ( counts & 0x0F00) >> 8;
//    value[1] = counts & 0x00FF;

    writeRegisters(addr,value);
}


double EagleCamera::getTEC_SetPoint()
{
    byte_vector_t addr = {0x03, 0x04};

    byte_vector_t value = readRegisters(addr);

//    uint16_t counts = (value[0] & 0x0F) << 8;
//    counts += value[1];

    IntegerType counts = fpga16BitsToInteger(value);

    return static_cast<double>(ADC_LinearCoeffs[0] + ADC_LinearCoeffs[1]*counts);
}


double EagleCamera::getPCBTemp()
{
    //
    // according to Reference Manual there are unusual addressing for this command (extra 0x00 after address)
    //
    byte_vector_t addr = {0x70, 0x71};
    byte_vector_t comm_addr = {0x53, 0xE0, 0x02, 0x6E, 0x00};

    byte_vector_t value = readRegisters(addr, comm_addr);

//    uint16_t counts = ((value[0] & 0x0F) << 8) + value[1];

    IntegerType counts = fpga16BitsToInteger(value);

    return counts/16.0;
}


double EagleCamera::getCCDTemp()
{
    //
    // according to Reference Manual there are unusual addressing for this command (extra 0x00 after address)
    //
    byte_vector_t addr = {0x6E, 0x6F};
    byte_vector_t comm_addr = {0x53, 0xE0, 0x02, 0x6E, 0x00};

    byte_vector_t value = readRegisters(addr, comm_addr);

//    uint16_t counts = ( value[0] << 8) + value[1];

    IntegerType counts = fpga16BitsToInteger(value);

    double val = ADC_LinearCoeffs[1]*static_cast<double>(counts);

    return ADC_LinearCoeffs[0] + val;
}



                        /*   read out process control methods   */


void EagleCamera::setReadoutRate(const std::string val)
{
    byte_vector_t addr = {0xA3, 0xA4};
    byte_vector_t v = {0x43, 0x80}; // for 75 kHz

    if ( !val.compare(EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST) ) v = {0x02, 0x02};

    writeRegisters(addr, v);
}


std::string EagleCamera::getReadoutRate()
{
    std::string value;
    byte_vector_t addr = {0xA3, 0xA4};

    byte_vector_t v = readRegisters(addr);

    if ( (v[0] == 0x02) && (v[1] == 0x02) ) {
        value = EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST;
    } else if ( (v[0] == 0x43) && (v[1] == 0x80) ) {
        value = EAGLE_CAMERA_FEATURE_READOUT_RATE_SLOW;
    } else {
        logMessageStream.str("");
        logMessageStream << "Unexpected FPGA registers values (got [" << std::hex << (int)v[0] << std::dec << ", "
                         << std::hex << (int)v[1] << std::dec << "])";
        throw EagleCameraException(0,EagleCamera::Error_UnexpectedFPGAValue,logMessageStream.str());
    }

    return value;
}


void EagleCamera::setReadoutMode(const std::string val)
{
    byte_vector_t addr = {0xF7};
    byte_vector_t v = {0x01}; // for normal mode

    if ( !val.compare(EAGLE_CAMERA_FEATURE_READOUT_MODE_TEST) ) v = {0x04};

    writeRegisters(addr,v);
}


std::string EagleCamera::getReadoutMode()
{
    std::string value;
    byte_vector_t addr = {0xF7};

    byte_vector_t v = readRegisters(addr);

    if ( v[0] == 0x01 ) {
        value = EAGLE_CAMERA_FEATURE_READOUT_MODE_NORMAL;
    } else if ( v[0] == 0x04 ) {
        value = EAGLE_CAMERA_FEATURE_READOUT_MODE_TEST;
    } else {
        logMessageStream.str("");
        logMessageStream << "Unexpected FPGA registers values (got " << std::hex << (int)v[0] << std::dec << ")";
        throw EagleCameraException(0,EagleCamera::Error_UnexpectedFPGAValue,logMessageStream.str());
    }

    return value;
}


                                /*   exposure time control methods   */

void EagleCamera::setExpTime(const double val)
{
    IntegerType counts = static_cast<IntegerType>(ceil(val/2.5E-8)); // in FPGA counts, 1 count = 25nsecs = 1/40MHz

    byte_vector_t addr = {0xED, 0xEE, 0xEF, 0xF0, 0xF1};
    byte_vector_t value = countsToFPGA40Bits(counts);

    writeRegisters(addr,value);
}


double EagleCamera::getExpTime()
{
    byte_vector_t addr = {0xED, 0xEE, 0xEF, 0xF0, 0xF1};
    byte_vector_t v = readRegisters(addr);

    IntegerType counts = fpga40BitsToCounts(v);

    return 2.5E-8 * counts;
}


void EagleCamera::setFrameRate(const double val)
{
    byte_vector_t addr = {0xDC, 0xDD, 0xDE, 0xDF, 0xE0};
    IntegerType counts = static_cast<IntegerType>(ceil(4.0E7/val));

    byte_vector_t v = countsToFPGA40Bits(counts);

    writeRegisters(addr,v);
}


double EagleCamera::getFrameRate()
{
    byte_vector_t addr = {0xDC, 0xDD, 0xDE, 0xDF, 0xE0};
    byte_vector_t v = readRegisters(addr);

    IntegerType counts = fpga40BitsToCounts(v);

    return 4.0E7/counts;
}



                            /*   gain control methods   */

void EagleCamera::setPreAmpGain(const std::string val)
{
    unsigned char ctrl_reg = getCtrlRegister();

    bool flag;

    if ( !val.compare(EAGLE_CAMERA_FEATURE_PREAMP_GAIN_HIGH) ) {
        flag = true;
    } else {
        flag = false;
    }

    setCtrlRegister(flag, is_reset_temp_trip(ctrl_reg), is_tec_enabled(ctrl_reg));
}


std::string EagleCamera::getPreAmpGain()
{
    unsigned char ctrl_reg = getCtrlRegister();

    std::string value;

    if ( is_high_gain_enabled(ctrl_reg) ) {
        value = EAGLE_CAMERA_FEATURE_PREAMP_GAIN_HIGH;
    } else {
        value = EAGLE_CAMERA_FEATURE_PREAMP_GAIN_LOW;
    }

    return value;
}
