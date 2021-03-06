#ifndef EAGLE_CAMERA_H
#define EAGLE_CAMERA_H


#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) // needs for xcliball.h
    #define _CRT_SECURE_NO_WARNINGS
    #include <windows.h>
    #undef min // to use std::numeric_limits<int>::min()
    #undef max // to use std::numeric_limits<int>::max()
#endif

#ifdef _MSC_VER
    // for MS compilers: disable multiple warnings about DLL-exports for the STL containers
    // (and many others C++11 defined classes)
    #pragma warning( disable: 4251 )
    // for MS compilers: disable warning related to new way of initializing
    // of array-type class members in constructor initializing list
    #pragma warning( disable: 4351 )
#if (_MSC_VER > 1800)
    #define NOEXCEPT_DECL noexcept
    #define MIN_INT_VALUE std::numeric_limits<int>::min()
#else
    #define NOEXCEPT_DECL // empty to compile with VS2013
    #define MIN_INT_VALUE MININT // VS2013 has no constexpr (it is required in enum definition)
#endif
#else
    #define NOEXCEPT_DECL noexcept
    #define MIN_INT_VALUE std::numeric_limits<int>::min()
#endif


#include <xcliball.h>

#include <export_decl.h>

#include <limits>
#include <string>
#include <sstream>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <exception>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>
#include <mutex>
#include <fitsio.h>

#include <iostream>


                    /*********************************************
                    *                                            *
                    *      EagleCamera CLASS DECLARATION         *
                    *                                            *
                    *    An C++ wrapper class for control of     *
                    *  Raptor Photonics Eagle-V 4240 CCD Camera  *
                    *                                            *
                    *********************************************/


                            /*   definitions   */

#define EAGLE_CAMERA_DEFAULT_CAPTURING_TIMEOUT_GAP 120 // in seconds. default timeout portion to be added to exposure time.
                                                       // the final timeout is one for capturing proccess
                                                       // (see doSnap XCLIB function)

#define EAGLE_CAMERA_DEFAULT_FITS_WRITING_TIMEOUT 100000 // in milliseconds (100 secs)
                                                         // default timeout for writing each image buffer into FITS file

#define EAGLE_CAMERA_DEFAULT_NUMBER_OF_BUFFERS 10 // default number of buffers used for FITS file saving

#define EAGLE_CAMERA_DEFAULT_BUFFER_TIMEOUT 10  // default timeout in seconds for captured image buffer copying proccess

#define EAGLE_CAMERA_DEFAULT_ACQUISITION_POLL_INTERVAL 100 // default interval in milliseconds for polling of acquisition
                                                           // proccess
#define EAGLE_CAMERA_DEFAULT_LOG_TAB 3        // default tabulation in symbols for logging

#define EAGLE_CAMERA_DEFAULT_TEMP_VALUE_DIGITS 2 // default number of digits after floating point
                                                 // for temperature values saved in FITS file

#define EAGLE_CAMERA_DEFAULT_EXPTIME_VALUE_DIGITS 2 // default number of digits after floating point
                                                    // for exposure time values saved in FITS file



// FITS keywords name to be written

#define EAGLE_CAMERA_FITS_DATE_KEYWORD_FORMAT "%Y-%m-%dT%H:%M:%S" // format for DATE and DATE-OBS FITS-keywords
                                                                  // Note: this format string is one for 'strftime'
                                                                  // function, but this API add also a ten part of
                                                                  // seconds to the final timestamp string

#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_ORIGIN "Acquisition system"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATEOBS "Start of the exposure in UTC"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATE "Date of the HDU creation in UTC"


#define EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTX "CRVAL1"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTX "Start coordinate along X-axis in CCD pixels"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTY "CRVAL2"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTY "Start coordinate along Y-axis in CCD pixels"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME "EXPTIME"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_EXPTIME "Integration time in seconds"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_BINNING "BINNING"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BINNING "Binning mode (XBINxYBIN)"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_XBIN "XBIN"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_XBIN "Binning mode along X-axis"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_YBIN "YBIN"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_YBIN "Binning mode along Y-axis"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_SHUTTER_STATE "SHUTTER"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_SHUTTER_STATE "Shutter state"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_RATE "READRATE"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_RATE "Camera readout rate"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_MODE "READMODE"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_MODE "Camera readout mode"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_TEC_STATE "TECSTATE"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_TEC_STATE "Thermoelectrial cooler state"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP "CCDTEMP"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_CCD_TEMP "CCD chip temperature in Celsius"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP "PCBTEMP"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_PCB_TEMP "PCB temperature in Celsius"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_SERIAL_NUMBER "SERNUM"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_SERIAL_NUMBER "Camera serial number"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_MICRO_VERSION "MICROVER"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_MICRO_VERSION "Camera microcontroller version"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_FPGA_VERSION  "FPGAVER"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_FPGA_VERSION  "Camera FPGA version"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_DATE  "BUILDDAT"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_DATE  "Camera build date, DD/MM/YY"

#define EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_CODE  "BUILDCOD"
#define EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_CODE  "Camera build code"

// just forward declaration

class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera_StringFeature;
class EAGLE_CAMERA_LIBRARY_EXPORT EagleCameraException;



class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera
{
    friend class EagleCamera_StringFeature;
public:

    typedef int64_t IntegerType;
    typedef double FloatingPointType;

//    EagleCamera();
    EagleCamera(const char* epix_video_fmt_filename = nullptr);
    EagleCamera(const std::string &epix_video_fmt_filename);

    virtual ~EagleCamera();


    enum EagleCameraFeatureType {UnknownType = -1, IntType, FloatType, StringType};

    enum EagleCameraFeatureAccess {UnknownAccess = -1, ReadWrite, ReadOnly, WriteOnly};

    enum EagleCameraError { Error_Uninitialized = MIN_INT_VALUE,
                            Error_NullPointer, Error_MemoryAllocation,
                            Error_InvalidUnitmap,
                            Error_UnknowCommand, Error_UnknowFeature,
                            Error_FeatureGetterIsNull, Error_FeatureSetterIsNull,
                            Error_FeatureTypeMismatch, Error_ReadOnlyFeature, Error_WriteOnlyFeature,
                            Error_FeatureValueIsOutOfRange, Error_InvalidFeatureValue,
                            Error_UnexpectedFPGAValue,
                            Error_AcquisitionProccessError, Error_CopyBufferTimeout,
                            Error_FitsWritingTimeout, Error_CameraIsAcquiring,
                            Error_OK = 0,
                            // errors from EAGLE V 4240 Instruction Manual
                            Error_ETX_SER_TIMEOUT = 0x51, Error_ETX_CK_SUM_ERR,
                            Error_ETX_I2C_ERR, Error_ETX_UNKNOWN_CMD, Error_ETX_DONE_LOW,
                            Error_FITS_ERR
                          };

    enum EagleCameraLogLevel {LOG_LEVEL_QUIET, LOG_LEVEL_ERROR, LOG_LEVEL_VERBOSE};

    enum EagleCameraLogIdent {LOG_IDENT_BLANK, LOG_IDENT_CAMERA_INFO, LOG_IDENT_CAMERA_ERROR,
                              LOG_IDENT_XCLIB_INFO, LOG_IDENT_XCLIB_ERROR};


    void initCamera(const int unitmap = 1, std::ostream *log_file = nullptr);

    void resetCamera();

    void setLogLevel(const EagleCameraLogLevel log_level);
    EagleCameraLogLevel getLogLevel() const;


    void startAcquisition();
    void stopAcquisition();

    // is invoked every time image was captured and
    // copied to buffer pointed by 'image_buffer'.
    // size of the buffer is in 'buff_len'.
    // in 'frame_no' a sequence number of frame for 'image_buffer' will be returned.
    // 'frame_no' starts from 0!!!
    void virtual imageReady(const IntegerType frame_no, const ushort* image_buffer, const size_t buffer_len);

    void logToFile(const EagleCamera::EagleCameraLogIdent ident, const std::string &log_str, const int indent_tabs = 0);
    void logToFile(const EagleCameraException &ex, const int indent_tabs = 0);

protected:

            /*   DECLARATION OF BASE CLASS FOR CAMERA FEATURES  */

    class EAGLE_CAMERA_LIBRARY_EXPORT CameraAbstractFeature {
    public:
        CameraAbstractFeature(const std::string & name,
                              const EagleCamera::EagleCameraFeatureType type,
                              const EagleCamera::EagleCameraFeatureAccess access);

        virtual ~CameraAbstractFeature();

        std::string name() const;
        EagleCamera::EagleCameraFeatureType type() const;
        EagleCamera::EagleCameraFeatureAccess access() const;

    protected:
        std::string _name;
        EagleCamera::EagleCameraFeatureType _type;
        EagleCamera::EagleCameraFeatureAccess _access;
    };

    // TYPE TO HOLD PREDEFINED FEATURES DESCRIPTION
    typedef std::map<std::string, std::unique_ptr<CameraAbstractFeature>> camera_feature_map_t;

    friend EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES();

            /*   DECLARATION OF WORKING TEMPLATE CLASS FOR CAMERA FEATURES  */

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value ||
                                                            std::is_same<T,std::string>::value>::type>
    class CameraFeature: public CameraAbstractFeature {
    public:
        CameraFeature(const std::string & name,
                      const EagleCamera::EagleCameraFeatureAccess access,
                      const std::vector<T> & range,
                      const std::function<T()> getter = nullptr,
                      const std::function<void(const T)> setter = nullptr):
            CameraAbstractFeature(name, EagleCamera::UnknownType, access), _range(range),
            _getter(getter), _setter(setter)
        {
            if ( std::is_integral<T>::value ) {
                _type = EagleCamera::IntType;
            } else if ( std::is_floating_point<T>::value ) {
                _type = EagleCamera::FloatType;
            } else if ( std::is_same<T,std::string>::value ) {
                _type = EagleCamera::StringType;
            };
        }

        T get() {
            if ( _getter ) return _getter();
            throw EagleCameraException(0,EagleCamera::Error_FeatureGetterIsNull,"Feature getter function pointer is null");
        }

        void set(const T val) {
            if ( _setter ) _setter(val);
            else {
                throw EagleCameraException(0,EagleCamera::Error_FeatureSetterIsNull,"Feature setter function pointer is null");
            }
        }

        std::vector<T> range() const {
            return _range;
        }

        void set_range(const std::vector<T> &r) {
            _range = r;
//            std::cout << "NAME = " << _name << "        SET RANGE: [" << _range[0] << ", " << _range[1] << "]\n";
        }

    private:
        std::function<T()> _getter;
        std::function<void(const T)> _setter;
        std::vector<T> _range;
    };

            /*   DECLARATION OF A PROXY CLASS TO ACCESS EAGLE CAMERA FEATURES  */

    class EAGLE_CAMERA_LIBRARY_EXPORT CameraFeatureProxy {
        friend class EagleCamera_StringFeature;
    public:
        CameraFeatureProxy(EagleCamera *camera);

                        // get feature value operators

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        operator T()
        {
            if ( !_camera ) { // strange, but should be handled
                throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera object is null!");
            }

            if ( !_camera->currentCameraFeature ) {
                throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera feature object is null!");
            }

            if ( _camera->currentCameraFeature->type() == EagleCamera::StringType ) {
                std::string log_str = "Feature type mismatch: ('" +
                        _camera->currentCameraFeature->name() + "' has string type)";
                throw EagleCameraException(0,EagleCamera::Error_FeatureTypeMismatch,log_str);
            }

            if ( _camera->currentCameraFeature->access() == EagleCamera::WriteOnly ) {
                std::string log_str = "Try to get value of write-only feature '" +
                        _camera->currentCameraFeature->name() + "'!";
                throw EagleCameraException(0,EagleCamera::Error_WriteOnlyFeature,log_str);
            }

            T ret_val;
            switch ( _camera->currentCameraFeature->type() ) {
                case EagleCameraFeatureType::IntType: {
                    CameraFeature<EagleCamera::IntegerType> *f =
                            static_cast<CameraFeature<EagleCamera::IntegerType> *>(_camera->currentCameraFeature);
                    ret_val = T(f->get());
                    break;
                }
                case EagleCameraFeatureType::FloatType: {
                    CameraFeature<double> *f = static_cast<CameraFeature<double> *>(_camera->currentCameraFeature);
                    ret_val = T(f->get());
                    break;
                }
            }
            return ret_val;
        }

        explicit operator EagleCamera_StringFeature();

        // set feature value operators

        template<typename T>
        using delRef = typename std::remove_reference<T>::type;

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<delRef<T>>::value, delRef<T>>::type>
        CameraFeatureProxy & operator=(T &&val)
        {
            if ( !_camera ) { // strange, but should be handled
                throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera object is null!");
            }

            if ( !_camera->currentCameraFeature ) {
                throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera feature object is null!");
            }

            if ( _camera->currentCameraFeature->access() == EagleCamera::ReadOnly ) {
                std::string log_str = "Try to set value to read-only feature '" +
                        _camera->currentCameraFeature->name() + "'!";
                throw EagleCameraException(0,EagleCamera::Error_ReadOnlyFeature,log_str);
            }
            switch ( _camera->currentCameraFeature->type() ) {
                case EagleCameraFeatureType::IntType: {
                    CameraFeature<EagleCamera::IntegerType> *f =
                            static_cast<CameraFeature<EagleCamera::IntegerType> *>(_camera->currentCameraFeature);
                    if ( (val < f->range()[0]) || (val > f->range()[1]) ) {
                        throw EagleCameraException(0,EagleCamera::Error_FeatureValueIsOutOfRange,
                                                   "User value for Integer feature is out of valid range!");
                    }
                    f->set(val);
                    break;
                }
                case EagleCameraFeatureType::FloatType: {
                    CameraFeature<double> *f = static_cast<CameraFeature<double> *>(_camera->currentCameraFeature);
//                    printf("RANGE: [%f %f], set val: %f\n",f->range()[0], f->range()[1], val);
                    if ( (val < f->range()[0]) || (val > f->range()[1]) ) {
                        throw EagleCameraException(0,EagleCamera::Error_FeatureValueIsOutOfRange,
                                                   "User value for FloatingPoint feature is out of valid range!");
                    }
                    f->set(val);
                    break;
                }
                case EagleCamera::StringType: {
                        std::string log_str = "Feature type mismatch: ('" +
                                _camera->currentCameraFeature->name() + "' has string type)";
                        throw EagleCameraException(0,EagleCamera::Error_FeatureTypeMismatch,log_str);
                }
            }
            return *this;
        }

        CameraFeatureProxy & operator = (const char* val);           // for string feature
        CameraFeatureProxy & operator = (const std::string & val);

    private:
        EagleCamera *_camera;

    };
                        /*   END OF CameraFeatureProxy DECLARATION   */

    friend class CameraFeatureProxy;


                        /*   DECLARATION OF BASE CLASS FOR CAMERA COMMANDS   */

    class CameraAbstractCommand {
    public:
        CameraAbstractCommand(const std::string name): _name(name){}
        virtual ~CameraAbstractCommand(){}
    private:
        std::string _name;
    };

    // TYPE TO HOLD PREDEFINED CAMERA COMMAND DESCRIPTION
    typedef std::map<std::string, std::unique_ptr<CameraAbstractCommand>> camera_command_map_t;

    friend camera_command_map_t INIT_CAMERA_COMMANDS();

                        /*   DECLARATION OF WORKING TEMPLATE CLASS FOR CAMERA COMMANDS   */

    template<typename ... Types>
    class CameraCommand: public CameraAbstractCommand {
    public:
        typedef std::function<void(Types ...args)> camera_command_func_t;

        CameraCommand(const std::string name, const camera_command_func_t &exec_func):
            CameraAbstractCommand(name), execFunc(exec_func), numberArgs(sizeof...(Types))
        {
        }

        void exec(Types ...args){
            if ( execFunc ) execFunc(std::forward<Types>(args)...);
        }

        size_t n_args() const
        {
            return numberArgs;
        }

    private:
        size_t numberArgs;
        camera_command_func_t execFunc;
    };

            /*   END OF CAMERA COMMANDS CLASS DECLARATION  */

public:

                // operator to access camera feature

    CameraFeatureProxy & operator[](const char* name);
    CameraFeatureProxy & operator[](const std::string & name);

                // operator to execute command

    template<typename ... Types>
    void operator()(const std::string name, Types ...args)
    {
        if ( cameraUnitmap <= 0 ) {
            throw EagleCameraException(0,EagleCamera::Error_Uninitialized,"Try to invoke command for uninitialized camera!");
        }

        auto search = PREDEFINED_CAMERA_COMMANDS.find(name);

        if ( search != PREDEFINED_CAMERA_COMMANDS.end() ) {
            CameraCommand<Types...> *op = static_cast<CameraCommand<Types...> *>( (search->second).get());
            op->exec(args...);
        } else {
            std::string log_str = "'" + name + "' is unknown camera command!";
            throw EagleCameraException(0,EagleCamera::Error_UnknowCommand,log_str);
        }
    }



protected:

        /*  camera configuration  */

    int cameraUnitmap;
    std::string cameraVideoFormatFilename;

    EagleCamera::EagleCameraLogLevel logLevel;
    std::ostream* cameraLog;
    std::mutex logMutex;

    IntegerType _ccdDimension[2];
    IntegerType _bitsPerPixel;
    IntegerType _frameBuffersNumber;

    double ADC_LinearCoeffs[2];
    double DAC_LinearCoeffs[2];

    void setInitialState();


        /*  acqusition control   */

    void saveToFitsFile(const IntegerType frame_no, const IntegerType buff_no, const double exp_time, bool as_extension);

    void doSnapAndCopy(const ulong timeout, const IntegerType frame_no, const IntegerType buff_no);

    IntegerType _frameCounts; // number of frames per acquisition proccess
    IntegerType _currentBuffer;

    IntegerType _frameBufferLines;
    long _imageXDim;
    long _imageYDim;
    long _imageStartX;
    long _imageStartY;
    long _imagePixelsNumber;
    double _expTime;
    std::chrono::system_clock::time_point _startExpTimepoint;
    std::chrono::system_clock::time_point _stopExpTimepoint;
    std::vector<std::string> _startExpTimestamp;
    std::vector<double> _ccdTemp;
    std::vector<double> _pcbTemp;
    std::vector<std::unique_ptr<ushort[]>> _imageBuffer; // image buffers addresses
    size_t _currentBufferLength;
    size_t _usedBuffersNumber;

    fitsfile* _fitsFilePtr;
    std::string _fitsFilename;
    std::string _fitsHdrFilename;
    std::string _fitsDataFormat;
    long _fitsWritingTimeout; // timeout in milliseconds for writing each image buffer into FITS file

    EagleCamera::EagleCameraError _lastCameraError;
    int _lastXCLIBError;

    std::atomic<bool> _acquiringFinished;
    long _acquisitionProccessPollingInterval; // in milliseconds

    std::future<void> _acquisitionProccessThreadFuture;

    long _capturingTimeoutGap;

//    std::vector<std::future<int>> _copyFramebuffersFuture;
    std::atomic<bool> _stopCapturing;


        /*  feauture control members  */

    CameraFeatureProxy cameraFeature;
    CameraAbstractFeature *currentCameraFeature;

    camera_feature_map_t PREDEFINED_CAMERA_FEATURES;
    void InitCameraFeatures();

        /*  command control members  */

    camera_command_map_t PREDEFINED_CAMERA_COMMANDS;
    void InitCameraCommands();


        /*  CAMERALINK serial port methods and member */

    typedef std::vector<unsigned char> byte_vector_t;

    bool CL_ACK_BIT_ENABLED;
    bool CL_CHK_SUM_BIT_ENABLED;

    int cl_read(byte_vector_t &data, const bool all = false); // all == true:  read all bytes from Rx-buffer,
                                                              // data.size():
                                                              //   0 and no (ACK and CHK_SUM):
                                                              //     return number of available bytes in Rx-buffer,
                                                              //   N: read a least N bytes or throw timeout
                                                              // the method returns number of bytes read

    int cl_write(const byte_vector_t val = byte_vector_t()); // if val.size() == 0 return allowed number of bytes in Tx-buffer

    int cl_exec(const byte_vector_t command, byte_vector_t &response, const long timeout = 10);  // execute 'command' and
                                                                                           // return 'response'
                                                                                           // 'timeout' is a timeout in millisecs
                                                                                           // between cl_write an cl_read commands
    int cl_exec(const byte_vector_t command, const long timeout = 10);

    byte_vector_t readRegisters(const byte_vector_t addr, const byte_vector_t addr_comm = byte_vector_t());
    void writeRegisters(const byte_vector_t addr, const byte_vector_t values);

    bool resetMicro(const long timeout = 10000); // timeout in microsecs
    bool resetFPGA(const long timeout = 10000);  // timeout in microsecs

    void setSystemState(bool check_sum_mode_enabled,
                        bool comm_ack_enabled,
                        bool hold_fpga_in_reset,  // if true then bit will be set set 0!!!
                        bool fpga_eprom_enabled);

    unsigned char getSystemState();

    inline bool is_chk_sum_enabled(const unsigned char state);
    inline bool is_ack_enabled(const unsigned char state);
    inline bool is_fpga_boot_ok(const unsigned char state);
    inline bool is_fpga_in_reset(const unsigned char state);   // if bit is 0 then return true!!!
    inline bool is_fpga_eprom_enabled(const unsigned char state);

    void setCtrlRegister(bool high_gain_enabled, bool reset_temp_trip, bool enable_tec);

    unsigned char getCtrlRegister();

    bool is_high_gain_enabled(const unsigned char state);
    bool is_reset_temp_trip(const unsigned char state);
    bool is_tec_enabled(const unsigned char state);

    void setTriggerMode(unsigned char mode);

    void setTriggerMode(bool snapshot, bool enable_fixed_frame_rate, bool start_cont_seq,
                        bool abort_exp, bool enable_extern_trigger, bool enable_rising_edge);

    unsigned char getTriggerMode();

    bool is_fixed_frame_rate(unsigned char mode);
    bool is_cont_seq(unsigned char mode);
    bool is_ext_trigger(unsigned char mode);
    bool is_rising_edge(unsigned char mode);

    void getManufactureData();

    std::string _buildDate, _buildCode, _FPGAVersion, _microVersion;
    EagleCamera::IntegerType _serialNumber;
    EagleCamera::IntegerType _ADC_Calib[2];
    EagleCamera::IntegerType _DAC_Calib[2];


            /*  setters and getters for camera features   */

    enum GeometryValueName {GV_XBIN, GV_YBIN, GV_ROILEFT, GV_ROITOP, GV_ROIWIDTH, GV_ROIHEIGHT};

    void setGeometryValue(const EagleCamera::GeometryValueName name, const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getGeometryValue(const EagleCamera::GeometryValueName name);

    void setXBIN(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getXBIN();

    void setYBIN(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getYBIN();

    void setROILeft(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getROILeft();

    void setROITop(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getROITop();

    void setROIWidth(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getROIWidth();

    void setROIHeight(const EagleCamera::IntegerType val);
    EagleCamera::IntegerType getROIHeight();

    void setExpTime(const double val);
    double getExpTime();

    void setFrameRate(const double val);
    double getFrameRate();

    void setShutterOpenDelay(const double val);
    double getShutterOpenDelay();

    void setShutterCloseDelay(const double val);
    double getShutterCloseDelay();

    void setTEC_SetPoint(const double temp);
    double getTEC_SetPoint();

    void setShutterState(const std::string val);
    std::string getShutterState();

    void setTECState(const std::string val);
    std::string getTECState();

    void setPreAmpGain(const std::string val);
    std::string getPreAmpGain();

    void setReadoutRate(const std::string val);
    std::string getReadoutRate();

    void setReadoutMode(const std::string val);
    std::string getReadoutMode();

    double getCCDTemp();
    double getPCBTemp();

    EagleCamera::IntegerType getADC_CALIB_0();
    EagleCamera::IntegerType getADC_CALIB_1();

    EagleCamera::IntegerType getDAC_CALIB_0();
    EagleCamera::IntegerType getDAC_CALIB_1();

    EagleCamera::IntegerType getSerialNumber();

    std::string getBuildDate();
    std::string getBuildCode();

    std::string getFPGAVersion();
    std::string getMicroVersion();


        /*  auxiliary methods  */

    int XCLIB_API_CALL(int err_code, const char *context);
//    int XCLIB_API_CALL(int err_code, const std::string &context);
    int XCLIB_API_CALL(int err_code, const std::string &context, const char *res = nullptr, const int res_len = 0);

    int CFITSIO_API_CALL(int err_code, const char *context);
    int CFITSIO_API_CALL(int err_code, const std::string &context);



    std::stringstream logMessageStream;
    std::mutex logMessageStreamMutex;

    // format logging message for call of CFITSIO functions
    // (the first argument is CFITSIO function name, others - its arguments
    // except the first one (pointer to FITS structure) - it is added automatically)
    template<typename... T>
    void formatFitsLogMessage(const char* func_name, T... args);

    // format logging message for call of XCLIB functions
    // (the first argument is XCLIB function name, others - its arguments
    // except the first one (unitmap) - it is added automatically)
    template<typename... T>
    void formatLogMessage(const char* func_name, T... args);

    // helper methods for logging
    template<typename T1, typename... T2>
    inline void logHelper(T1 first, T2... last);

    template<typename T>
    inline void logHelper(T arg);

    inline void logHelper(const std::string &str);
    inline void logHelper(const char* str);
    inline void logHelper(const void* addr);
    inline void logHelper();

    // add to logging string (see formatLogMessage) return value of XCLIB functions in form "-> ret_val"
    inline std::string logXCLIB_Info(const std::string & str, const int result);

    // add to logging string (see formatLogMessage) return value of XCLIB functions in form "-> [...]"
    // for pxd_serialRead it return bytes read from Rx-buffer
    std::string logXCLIB_Info(const std::string & str, const char *res, const int res_len);


    EagleCamera::IntegerType fpga40BitsToCounts(const byte_vector_t &vals);
    byte_vector_t countsToFPGA40Bits(const EagleCamera::IntegerType counts);

    EagleCamera::IntegerType fpga12BitsToInteger(const byte_vector_t &vals);
    byte_vector_t integerToFPGA12Bits(const EagleCamera::IntegerType val);


        /*  static members and methods  */

    static size_t createdObjects;
};


                /*  CLASS TO GET VALUE OF STRING FEATURES  */

class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera_StringFeature
{
    friend class EagleCamera::CameraFeatureProxy;
public:
    explicit EagleCamera_StringFeature();
    EagleCamera_StringFeature(EagleCamera_StringFeature &&other);
    EagleCamera_StringFeature(const EagleCamera_StringFeature &other);
    EagleCamera_StringFeature(EagleCamera::CameraFeatureProxy &feature);

    EagleCamera_StringFeature & operator =(EagleCamera_StringFeature other);

    std::string value() const;
    std::string name() const;
    std::vector<std::string> range() const;


private:
    std::string _name;
    std::string _value;
    std::vector<std::string> _range;
};



                /*  CLASS FOR CAMERA EXCEPTIONS  */


class EAGLE_CAMERA_LIBRARY_EXPORT EagleCameraException: public std::exception
{
public:
    EagleCameraException(): std::exception(), _xclib_err(0), _camera_err(EagleCamera::Error_OK), _context(""){}
    EagleCameraException(const int xclib_err, const EagleCamera::EagleCameraError camera_err, const char* context);
    EagleCameraException(const int xclib_err, const EagleCamera::EagleCameraError camera_err, const std::string & context);

    int XCLIB_Error() const;
    EagleCamera::EagleCameraError Camera_Error() const;

    const char* what() const NOEXCEPT_DECL;
private:
    int _xclib_err;
    EagleCamera::EagleCameraError _camera_err;

    std::string _context;
};




                    /**************************************
                    *                                     *
                    *   DEFINITIONS FOR CAMERA COMMANDS   *
                    *                                     *
                    **************************************/

#define EAGLE_CAMERA_COMMAND_INIT "INIT"
#define EAGLE_CAMERA_COMMAND_RESET "RESET"
#define EAGLE_CAMERA_COMMAND_EXPSTART "EXPSTART"
#define EAGLE_CAMERA_COMMAND_EXPSTOP  "EXPSTOP"


                    /*******************************************************
                    *                                                      *
                    *   DEFINITIONS FOR INTEGER AND FLOATING-POINT TYPES   *
                    *                   CAMERA FEATURES                    *
                    *                                                      *
                    *******************************************************/

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

#define EAGLE_CAMERA_FEATURE_FRAME_COUNTS_NAME         "FrameCount"
#define EAGLE_CAMERA_FEATURE_FITS_FILENAME_NAME        "FitsFilename"
#define EAGLE_CAMERA_FEATURE_FITS_HDR_FILENAME_NAME    "FitsHdrFilename"
#define EAGLE_CAMERA_FEATURE_FRAME_BUFFERS_NUMBER_NAME "FrameBuffers"


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


#endif // EAGLE_CAMERA_H

