#include "eagle_camera.h"

#include <xcliball.h>
#include <cameralink_defs.h>

#include <chrono>
#include <sstream>
#include <cstring>

#include <iostream>

                        /*********************************************
                        *                                            *
                        *      EagleCamera CLASS IMPLEMENTATION      *
                        *                                            *
                        *********************************************/


                            /*  Auxiliary functions */


int XCLIB_API_CALL(int err_code, const char *context)
{
    if ( err_code < 0 ) {
        throw EagleCameraException(err_code, EagleCamera::Error_OK, context);
    }

    return err_code;
}

int XCLIB_API_CALL(int err_code, const std::string &context)
{
    if ( err_code < 0 ) {
        throw EagleCameraException(err_code, EagleCamera::Error_OK, context);
    }

    return err_code;
}


static std::string time_stamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    char time_stamp[100];

    struct std::tm buff;
    buff = *std::localtime(&now_c);

    std::strftime(time_stamp,sizeof(time_stamp),"%c",&buff);

    return std::string(time_stamp);
}


static std::string pointer_to_str(void* ptr)
{
    char addr[20];
#ifdef _MSC_VER
    int n = _snprintf_s(addr,20,"%p",ptr);
#else
    int n = snprintf(addr,20,"%p",ptr);
#endif
    return std::string(addr);
}



                /*  INIT STATIC PREDEFINED FEATURES MAP  */

extern EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES();

EagleCamera::camera_feature_map_t EagleCamera::PREDEFINED_CAMERA_FEATURES = INIT_CAMERA_FEATURES();
//EagleCamera::camera_feature_map_t EagleCamera::PREDEFINED_CAMERA_FEATURES;

size_t EagleCamera::createdObjects = 0;

                /*  CONSTRUCTORS AND DESTRUCTOR  */

EagleCamera::EagleCamera(const char *epix_video_fmt_filename):
    cameraVideoFormatFilename(""),
    cameraUnitmap(-1),
    logLevel(EagleCamera::LOG_LEVEL_VERBOSE), cameraLog(nullptr),
    cameraFeature(this), currentCameraFeature(nullptr),
    CL_ACK_BIT_ENABLED(CL_DEFAULT_ACK_ENABLED), CL_CHK_SUM_BIT_ENABLED(CL_DEFAULT_CK_SUM_ENABLED),
    _serialNumber(0), _buildDate(), _buildCode(),
    _microVersion(), _FPGAVersion(),
    _ADC_Calib(), ADC_LinearCoeffs(),
    _DAC_Calib(), DAC_LinearCoeffs(),
    PREDEFINED_CAMERA_COMMANDS()
{
    if ( !createdObjects ) {
        if ( epix_video_fmt_filename != nullptr ) {
            cameraVideoFormatFilename = epix_video_fmt_filename;
            std::string log_str = std::string("pxd_PIXCIopen(\"\",NULL,") + cameraVideoFormatFilename + ")";
            XCLIB_API_CALL( pxd_PIXCIopen("",NULL,epix_video_fmt_filename), log_str);
        } else {
            std::string log_str = "pxd_PIXCIopen(\"\",\"DEFAULT\",\"\")";
            XCLIB_API_CALL( pxd_PIXCIopen("","DEFAULT",""), log_str);
        }
    }

    ++createdObjects;

    InitCameraCommands();

    setLogLevel(logLevel);
}


EagleCamera::EagleCamera(const std::string & epix_video_fmt_filename):
    EagleCamera(epix_video_fmt_filename.c_str())
{
}


EagleCamera::~EagleCamera()
{
    --createdObjects;

    if ( !createdObjects ) {
        pxd_PIXCIclose();
    }
}


                            /*  PUBLIC METHODS  */

void EagleCamera::setLogLevel(const EagleCameraLogLevel log_level)
{
    logLevel = log_level;

//    CameraCommand<int, std::ostream*> initCom("INIT",std::bind(
//                         static_cast<void(EagleCamera::*)(const int, const std::ostream*)>
//                         (&EagleCamera::initCamera), this, std::placeholders::_1, std::placeholders::_2
//                                                               )
//                                              );

//    initCom.exec(1,nullptr);

}


EagleCamera::EagleCameraLogLevel EagleCamera::getLogLevel() const
{
    return logLevel;
}


void EagleCamera::initCamera(const int unitmap, const std::ostream *log_file)
{
    std::cout << "CAMERA: initCamera(" << std::to_string(unitmap) << ", " << log_file << ")\n";

    formatLogMessage("pxd_serialConfigure",0,CL_DEFAULT_BAUD_RATE,CL_DEFAULT_DATA_BITS,0,CL_DEFAULT_STOP_BIT,0,0,0);
    XCLIB_API_CALL( pxd_serialConfigure(cameraUnitmap,0,CL_DEFAULT_BAUD_RATE,CL_DEFAULT_DATA_BITS,0,CL_DEFAULT_STOP_BIT,0,0,0),
                    logMessageStream.str());

    if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
        logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logMessageStream.str());
    }

    resetFPGA();

    getManufactureData();
}


void EagleCamera::resetCamera()
{

}


void EagleCamera::startAcquisition()
{

}


void EagleCamera::stopAcquisition()
{

}


EagleCamera::CameraFeatureProxy & EagleCamera::operator [](const std::string & name)
{
    if ( cameraUnitmap <= 0 ) {
        throw EagleCameraException(0,EagleCamera::Error_Uninitialized,"Try to access feature for uninitialized camera!");
    }

    auto search = PREDEFINED_CAMERA_FEATURES.find(name);

    if ( search != PREDEFINED_CAMERA_FEATURES.end() ) {
//        currentCameraFeature = (PREDEFINED_CAMERA_FEATURES[name]).get();
        currentCameraFeature = (search->second).get();
    } else {
        currentCameraFeature = nullptr;
        std::string log_str = "'" + name + "' is unknown camera feature!";
        throw EagleCameraException(0,EagleCamera::Error_UnknowFeature,log_str);
    }

    return cameraFeature;
}


EagleCamera::CameraFeatureProxy & EagleCamera::operator[](const char* name)
{
    operator [](std::string(name));
}



void EagleCamera::logToFile(const EagleCamera::EagleCameraLogIdent ident, const std::string &log_str, const int indent_tabs)
{
    if ( !cameraLog ) return;

    if ( logLevel == EagleCamera::LOG_LEVEL_QUIET ) return;

    std::string tab;
    if ( indent_tabs > 0 ) tab.resize(indent_tabs*EAGLE_CAMERA_DEFAULT_LOG_TAB, ' ');

    std::string str = "[" + time_stamp() + "]";

    switch ( ident ) {
    case EagleCamera::LOG_IDENT_BLANK:
        *cameraLog  << log_str << std::endl << std::flush;
        return;
    case EagleCamera::LOG_IDENT_CAMERA_INFO:
        str += "[CAMERA INFO] ";
        break;
    case EagleCamera::LOG_IDENT_CAMERA_ERROR:
        str += "[CAMERA ERROR] ";
        break;
    case EagleCamera::LOG_IDENT_XCLIB_INFO:
        str += "[XCLIB INFO] ";
        break;
    case EagleCamera::LOG_IDENT_XCLIB_ERROR:
        str += "[XCLIB ERROR] ";
        break;
    default:
        break;
    }


    *cameraLog << str << tab << log_str << std::endl << std::flush;
}



void EagleCamera::logToFile(const EagleCameraException &ex, const int indent_tabs)
{
    if ( ex.XCLIB_Error() < 0 ) {
        std::string log_str = ex.what();
        log_str += " [XCLIB ERROR CODE: " + std::to_string(ex.XCLIB_Error()) + "]";

        logToFile(EagleCamera::LOG_IDENT_XCLIB_ERROR, log_str, indent_tabs);
    }

    if ( ex.Camera_Error() != EagleCamera::Error_OK ) {
        std::string log_str = ex.what();
        log_str += " [CAMERA ERROR CODE: " + std::to_string(ex.Camera_Error()) + "]";

        logToFile(EagleCamera::LOG_IDENT_CAMERA_ERROR, log_str, indent_tabs);
    }
}



                            /*  PROTECTED METHODS  */

    // CAMERALINK serial port related methods

int EagleCamera::cl_read(byte_vector_t &data,  const bool all)
{
    int nbytes;
    std::unique_ptr<char[]> buff;
    char* buff_ptr;

    int info_len = 0;
    if ( CL_ACK_BIT_ENABLED ) ++info_len;
    if ( CL_CHK_SUM_BIT_ENABLED ) ++info_len;

    formatLogMessage("pxd_serialRead",0,NULL,0);

    // how many byte available for reading ...
    XCLIB_API_CALL( nbytes = pxd_serialRead(cameraUnitmap,0,nullptr,0), logMessageStream.str() );
    if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
        logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(),nbytes));
    }

    // special case
    if ( (data.size() == 0) && !info_len ) { // nothing to read
        return nbytes;
    }

    if ( !all ) {
        nbytes = data.size() + info_len;
        buff = std::unique_ptr<char[]>(new char[nbytes]);
    } else { // just read all available bytes in Rx-buffer
        if ( !nbytes ) return nbytes; // no data? do not wait and return immediately!
    }

    buff = std::unique_ptr<char[]>(new char[nbytes]);
    buff_ptr = buff.get();

    formatLogMessage("pxd_serialRead", 0, buff_ptr, nbytes);
    XCLIB_API_CALL( pxd_serialRead(cameraUnitmap, 0, buff_ptr, nbytes), logMessageStream.str() );
    if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
        logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(),buff_ptr,nbytes));
    }

    if ( all ) {
        data.resize(nbytes);
        memcpy(data.data(),buff_ptr,nbytes);
    } else {
        memcpy(data.data(),buff_ptr,nbytes - info_len);  // exclude possible ACK and CHK_SUM bytes

        if ( buff[data.size()] != CL_ETX ) {
            throw EagleCameraException(0,(EagleCamera::EagleCameraError)buff[data.size()],"Last serial read operation failed!");
        }
    }

    return nbytes;
}


int EagleCamera::cl_write(const byte_vector_t val)
{
    int nbytes;

    if ( val.size() == 0 ) { // special case
        formatLogMessage("pxd_serialWrite",0,NULL,0);
        XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, NULL, 0), logMessageStream.str() );

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), nbytes));
        }
    } else {
        int N;

        formatLogMessage("pxd_serialWrite",0,val.data(),val.size());
        XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, (char*)val.data(), val.size()), logMessageStream.str() );

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), nbytes));
        }

        // write mandatory End-of-Transmision byte
        char ack = CL_ETX;

        formatLogMessage("pxd_serialWrite",0,&ack,1);
        XCLIB_API_CALL( N = pxd_serialWrite(cameraUnitmap, 0, &ack, 1), logMessageStream.str() );

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), N));
        }

        nbytes += N;

        if ( CL_CHK_SUM_BIT_ENABLED ) { // compute check sum
            char sum = val[0];
            for ( int i = 1; i < val.size(); ++i ) sum ^= val[i];
            sum ^= CL_ETX;

            formatLogMessage("pxd_serialWrite",0,&sum,1);
            XCLIB_API_CALL( N = pxd_serialWrite(cameraUnitmap, 0, &sum, 1), logMessageStream.str() );

            if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
                logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), N));
            }

            nbytes += N;
        }
    }

    return nbytes;
}


int EagleCamera::cl_exec(const EagleCamera::byte_vector_t command, EagleCamera::byte_vector_t &response, const long timeout)
{
    cl_write(command);
    if ( timeout > 0 ) std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    return cl_read(response);
}


int EagleCamera::cl_exec(const EagleCamera::byte_vector_t command, const long timeout)
{
    byte_vector_t empty;
    return cl_exec(command,empty,timeout);
}


void EagleCamera::writeRegisters(const byte_vector_t addr, const byte_vector_t values)
{
    size_t N = ( addr.size() < values.size() ) ? addr.size() : values.size();
    if ( !N ) return;

    byte_vector_t comm = CL_COMMAND_WRITE_VALUE;

    size_t i = 0;
    for ( auto address: addr ) {
        comm[3] = address;
        comm[4] = values[i++];
        cl_exec(comm);
        if ( i == N ) return; // just protection from out-of-range error ...
    }
}


EagleCamera::byte_vector_t EagleCamera::readRegisters(const byte_vector_t addr, const byte_vector_t addr_comm)
{
    if ( !addr.size() ) return byte_vector_t();

    byte_vector_t comm = CL_COMMAND_READ_VALUE;
    byte_vector_t a_comm;
    byte_vector_t res(addr.size());
    byte_vector_t value(1);

    if ( !addr_comm.size() ) a_comm = CL_COMMAND_SET_ADDRESS; else a_comm = addr_comm;

    size_t i = 0;
    for (auto address: addr ) {
        a_comm[3] = address;
        cl_exec(a_comm);
        cl_exec(comm,value);
        res[i++] = value[0];
    }

    return res;
}


unsigned char EagleCamera::getSystemState()
{
    byte_vector_t comm = {0x49};
    byte_vector_t state(1);

    cl_exec(comm,state);

    CL_ACK_BIT_ENABLED = is_ack_enabled(state[0]);
    CL_CHK_SUM_BIT_ENABLED = is_chk_sum_enabled(state[0]);

    return state[0];
}


inline bool EagleCamera::is_chk_sum_enabled(const unsigned char state)
{
    return state & CL_SYSTEM_STATE_CK_SUM;
}


inline bool EagleCamera::is_ack_enabled(const unsigned char state)
{
    return state & CL_SYSTEM_STATE_ACK;
}

inline bool EagleCamera::is_fpga_boot_ok(const unsigned char state)
{
    return state & CL_SYSTEM_STATE_FPGA_BOOT_OK;
}


inline bool EagleCamera::is_fpga_in_reset(const unsigned char state)
{
    return !(state & CL_SYSTEM_STATE_FPGA_RST_HOLD);
}


inline bool EagleCamera::is_fpga_eprom_enabled(const unsigned char state)
{
    return state & CL_SYSTEM_STATE_FPGA_EPROM_COMMS;
}


void EagleCamera::setSystemState(bool check_sum_mode_enabled,
                                 bool comm_ack_enabled,
                                 bool hold_fpga_in_reset,
                                 bool fpga_eprom_enabled)
{
    byte_vector_t comm = {0x4F, 0x0};

    if ( check_sum_mode_enabled ) comm[1] |= CL_SYSTEM_STATE_CK_SUM;
    if ( comm_ack_enabled ) comm[1] |= CL_SYSTEM_STATE_ACK;
    if ( !hold_fpga_in_reset ) comm[1] |= CL_SYSTEM_STATE_FPGA_RST_HOLD;
    if ( fpga_eprom_enabled ) comm[1] |= CL_SYSTEM_STATE_FPGA_EPROM_COMMS;

    cl_exec(comm); // if ACK_BIT_ENABLED then camera response will be checked in 'cl_read'!!!

    CL_ACK_BIT_ENABLED = comm_ack_enabled;
    CL_CHK_SUM_BIT_ENABLED = check_sum_mode_enabled;
}


void EagleCamera::setCtrlRegister(bool high_gain_enabled, bool reset_temp_trip, bool enable_tec)
{
    byte_vector_t comm = CL_COMMAND_WRITE_VALUE;


    if ( high_gain_enabled ) comm[4] |= CL_FPGA_CTRL_REG_HIGH_GAIN;
    if ( reset_temp_trip ) comm[4] |= CL_FPGA_CTRL_REG_TMP_TRIP_RST;
    if ( enable_tec ) comm[4] |= CL_FPGA_CTRL_REG_ENABLE_TEC;

    cl_exec(comm); // if ACK_BIT_ENABLED then camera response will be checked in 'cl_read'!!!
}


unsigned char EagleCamera::getCtrlRegister()
{
    byte_vector_t val = readRegisters({0x0});

    return val[0];
}


bool EagleCamera::resetMicro(const long timeout)
{
    byte_vector_t comm = {0x55, 0x99, 0x66, 0x11};
    byte_vector_t poll_comm = {0x4F, 0x51};
    byte_vector_t ack;

    cl_write(comm); // here there is no response from camera

    size_t timeout_counts;
    if ( timeout > 0 ) {
        std::chrono::milliseconds readTimeout{timeout};
        timeout_counts = readTimeout.count();
    } else { // no timeout
        timeout_counts = std::numeric_limits<size_t>::max();
    }

    auto start = std::chrono::system_clock::now();

    for (;;) {
        cl_write(poll_comm);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        cl_read(ack,true);

        if ( ack.size() ) {
            if ( ack[0] == CL_ETX ) return true;
            else return false; // if all is ok it should be equal to CL_ETX
        }

        auto now = std::chrono::system_clock::now();
        if ( std::chrono::duration_cast<std::chrono::milliseconds>(start-now).count() >= timeout_counts ) return false;
    }

    return true;
}


bool EagleCamera::resetFPGA(const long timeout)
{
    size_t timeout_counts;
    if ( timeout > 0 ) {
        std::chrono::milliseconds readTimeout{timeout};
        timeout_counts = readTimeout.count();
    } else { // no timeout
        timeout_counts = std::numeric_limits<size_t>::max();
    }

    byte_vector_t comm = {0x4F, 0x52};
    byte_vector_t poll_comm = {0x49};
    byte_vector_t ack(1);

    auto start = std::chrono::system_clock::now();

    for (;;) {
        cl_exec(poll_comm,ack);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if ( is_fpga_boot_ok(ack[0]) ) {
            return true;
        }

        auto now = std::chrono::system_clock::now();
        if ( std::chrono::duration_cast<std::chrono::milliseconds>(start-now).count() >= timeout_counts ) return false;
    }

    return false;
}


void EagleCamera::getManufactureData()
{
    byte_vector_t comm = CL_COMMAND_GET_MANUFACTURER_DATA_1;
    cl_exec(comm);

    comm = CL_COMMAND_GET_MANUFACTURER_DATA_2;
    byte_vector_t value(18);
    cl_exec(comm,value);

    _serialNumber = (value[0] << 8) + value[1];

    _buildDate = std::to_string(value[2]) + "/" + std::to_string(value[3]) + "/" + std::to_string(value[4]); // DD/MM/YY

    char buff[6];
    _buildCode = (char*)memcpy(buff, value.data() + 5, 5);

    _ADC_Calib[0] = (value[11] << 8) + value[10];
    _ADC_Calib[1] = (value[13] << 8) + value[12];

    // y = ADC_LinearCoeffs[0] + ADC_LinearCoeffs[1]*x
    // temp(Celcius) = ADC_LinearCoeffs[0] + ADC_LinearCoeffs[1]*ADC(counts)
    ADC_LinearCoeffs[1] = (ADC_CALIBRATION_POINT_2 - ADC_CALIBRATION_POINT_1)/(_ADC_Calib[1] - _ADC_Calib[0]);
    ADC_LinearCoeffs[0] = ADC_CALIBRATION_POINT_1 - ADC_LinearCoeffs[1]*_ADC_Calib[0];

    _DAC_Calib[0] = (value[15] << 8) + value[14];
    _DAC_Calib[1] = (value[17] << 8) + value[16];

    // y = DAC_LinearCoeffs[0] + DAC_LinearCoeffs[1]*x
    // DAC(counts) = DAC_LinearCoeffs[0] + DAC_LinearCoeffs[1]*temp
    DAC_LinearCoeffs[1] = (_DAC_Calib[1] - _DAC_Calib[0])/(DAC_CALIBRATION_POINT_2 - DAC_CALIBRATION_POINT_1);
    DAC_LinearCoeffs[0] = _DAC_Calib[0] - DAC_LinearCoeffs[1]*DAC_CALIBRATION_POINT_1;
}


EagleCamera::IntegerType EagleCamera::getSerialNumber()
{
    return _serialNumber;
}

EagleCamera::IntegerType EagleCamera::getADC_CALIB_0()
{
    return _ADC_Calib[0];
}

EagleCamera::IntegerType EagleCamera::getADC_CALIB_1()
{
    return _ADC_Calib[1];
}

EagleCamera::IntegerType EagleCamera::getDAC_CALIB_0()
{
    return _DAC_Calib[0];
}

EagleCamera::IntegerType EagleCamera::getDAC_CALIB_1()
{
    return _DAC_Calib[1];
}

std::string EagleCamera::getBuildDate()
{
    return _buildDate;
}

std::string EagleCamera::getBuildCode()
{
    return _buildCode;
}


    // format logging message

template<typename ...T>
void EagleCamera::formatLogMessage(const char* func_name, T ...args)
{

    logMessageStream.str("");
    logMessageStream << func_name << "(" << cameraUnitmap << ", ";
    logHelper(args...);
    logMessageStream << ")";
}


template<typename T1, typename... T2>
inline void EagleCamera::logHelper(T1 first, T2... last)
{
    logHelper(first);
    logMessageStream << ", ";
    logHelper(last ...);
}


template<typename T>
void EagleCamera::logHelper(T arg)
{
    logMessageStream << arg;
}

void EagleCamera::logHelper(const char *str)
{
    logMessageStream << "\"" << str << "\"";
}

void EagleCamera::logHelper(const std::string &str)
{
    logHelper(str.c_str());
}


inline std::string EagleCamera::logXCLIB_Info(const std::string &str, const int result)
{
    return str + " -> " + std::to_string(result);
}


std::string EagleCamera::logXCLIB_Info(const std::string &str, const char *res, const int res_len)
{
    std::stringstream ss;

    if ( !res ) return str + " -> []";
    if ( res_len <= 0 )  return str + " -> []";

    ss << " -> [" << std::hex << (int)res[0] << std::dec;
    for ( int i = 1; i < res_len; +i ) ss << ", " << std::hex << (int)res[i] << std::dec;
    ss << "]";

    return str + ss.str();
}










void EagleCamera::InitCameraCommands()
{
    PREDEFINED_CAMERA_COMMANDS["INIT"] = std::unique_ptr<CameraAbstractCommand>(
                new CameraCommand<int, std::ostream*>( "INIT",
                                 std::bind(static_cast<void(EagleCamera::*)(const int, const std::ostream*)>
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




                 /***************************************************
                 *                                                  *
                 *   IMPLEMENTATION OF EagleCameraException CLASS   *
                 *                                                  *
                 ***************************************************/


EagleCameraException::EagleCameraException(const int xclib_err, const EagleCamera::EagleCameraError camera_err,
                                           const std::string &context):
    std::exception(), _xclib_err(xclib_err), _camera_err(camera_err), _context(context)
{
}

EagleCameraException::EagleCameraException(const int xclib_err, const EagleCamera::EagleCameraError camera_err,
                                           const char *context):
    EagleCameraException(xclib_err, camera_err, std::string(context))
{
}


const char* EagleCameraException::what() const NOEXCEPT_DECL
{
    return _context.c_str();
}


int EagleCameraException::XCLIB_Error() const
{
    return _xclib_err;
}


EagleCamera::EagleCameraError EagleCameraException::Camera_Error() const
{
    return _camera_err;
}
