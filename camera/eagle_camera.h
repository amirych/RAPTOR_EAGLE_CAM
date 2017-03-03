#ifndef EAGLE_CAMERA_H
#define EAGLE_CAMERA_H


#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) // needs for xcliball.h
    #include <windows.h>
#endif

#ifdef _MSC_VER
    // for MS compilers: disable multiple warnings about DLL-exports for the STL containers
    // (and many others C++11 defined classes)
    #pragma warning( disable: 4251 )
#if (_MSC_VER > 1800)
    #define NOEXCEPT_DECL noexcept
#else
    #define NOEXCEPT_DECL // empty to compile with VS2013
#endif
#else
    #define NOEXCEPT_DECL noexcept
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

                    /*********************************************
                    *                                            *
                    *      EagleCamera CLASS DECLARATION         *
                    *                                            *
                    *    An C++ wrapper class for control of     *
                    *  Raptor Photonics Eagle-V 4240 CCD Camera  *
                    *                                            *
                    *********************************************/

// just forward declaration

class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera_StringFeature;
class EAGLE_CAMERA_LIBRARY_EXPORT EagleCameraException;


#define EAGLE_CAMERA_DEFAULT_LOG_TAB 3 // default tabulation in symbols for logging


class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera
{
    friend class EagleCamera_StringFeature;
public:

    typedef int64_t IntegerType;
    typedef double FloatingPointType;

    EagleCamera(const char* epix_video_fmt_filename = nullptr);
    EagleCamera(const std::string &epix_video_fmt_filename);

    virtual ~EagleCamera();


    enum EagleCameraFeatureType {UnknownType = -1, IntType, FloatType, StringType};

    enum EagleCameraFeatureAccess {UnknownAccess = -1, ReadWrite, ReadOnly, WriteOnly};

    enum EagleCameraError { Error_Uninitialized = std::numeric_limits<int>::min(),
                            Error_NullPointer, Error_UnknowCommand,
                            Error_UnknowFeature, Error_ReadOnlyFeature, Error_WriteOnlyFeature,
                            Error_OK = 0,
                            // errors from EAGLE V 4240 Instruction Manual
                            Error_ETX_SER_TIMEOUT = 0x51, Error_ETX_CK_SUM_ERR,
                            Error_ETX_I2C_ERR, Error_ETX_UNKNOWN_CMD, Error_ETX_DONE_LOW
                          };

    enum EagleCameraLogLevel {LOG_LEVEL_QUIET, LOG_LEVEL_ERROR, LOG_LEVEL_VERBOSE};

    enum EagleCameraLogIdent {LOG_IDENT_BLANK, LOG_IDENT_CAMERA_INFO, LOG_IDENT_CAMERA_ERROR,
                              LOG_IDENT_XCLIB_INFO, LOG_IDENT_XCLIB_ERROR};


    void initCamera(const int unitmap = 1, const std::ostream *log_file = nullptr);

    void resetCamera();

    void setLogLevel(const EagleCameraLogLevel log_level);
    EagleCameraLogLevel getLogLevel() const;


    void startAcquisition();
    void stopAcquisition();

    void logToFile(const EagleCamera::EagleCameraLogIdent ident, const std::string &log_str, const int indent_tabs = 0);
    void logToFile(const EagleCameraException &ex, const int indent_tabs = 0);

protected:

            /*   DECLARATION OF BASE CLASS FOR CAMERA FEATURES  */

    class CameraAbstractFeature {
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
        }

        void set(const T val) {
            if ( _setter ) _setter(val);
        }

        std::vector<T> range() const {
            return _range;
        }

    private:
        std::function<T()> _getter;
        std::function<void(const T)> _setter;
        std::vector<T> _range;
    };

            /*   DECLARATION OF A PROXY CLASS TO ACCESS EAGLE CAMERA FEATURES  */

    class CameraFeatureProxy {
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
                   f->set(val);
                   break;
               }
               case EagleCameraFeatureType::FloatType: {
                   CameraFeature<double> *f = static_cast<CameraFeature<double> *>(_camera->currentCameraFeature);
                   f->set(val);
                   break;
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
    std::ostream *cameraLog;

    double ADC_LinearCoeffs[2];
    double DAC_LinearCoeffs[2];


        /*  feauture control members  */

    CameraFeatureProxy cameraFeature;
    CameraAbstractFeature *currentCameraFeature;

    static camera_feature_map_t PREDEFINED_CAMERA_FEATURES;

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

    int cl_exec(const byte_vector_t command, byte_vector_t &response, const long timeout = 500);  // execute 'command' and
                                                                                           // return 'response'
                                                                                           // 'timeout' is a timeout in millisecs
                                                                                           // between cl_write an cl_read commands
    int cl_exec(const byte_vector_t command, const long timeout = 500);

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

    void setTriggerMode();
    unsigned char getTriggerMode();

    inline bool is_high_gain_enabled(const unsigned char state);
    inline bool is_reset_temp_trip(const unsigned char state);
    inline bool is_tec_enabled(const unsigned char state);

    void getManufactureData();

    std::string _buildDate, _buildCode, _FPGAVersion, _microVersion;
    EagleCamera::IntegerType _serialNumber;
    EagleCamera::IntegerType _ADC_Calib[2];
    EagleCamera::IntegerType _DAC_Calib[2];


            /*  setters and getters for camera features   */

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
    double getFrameate();

    void setShutterOpenDelay(const double val);
    double getShutterOpenDelay();

    void setShutterCloseDelay(const double val);
    double getShutterCloseDelay();

    void setTEC_SetPoint(const double val);
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

    std::stringstream logMessageStream;

    // format logging message for call of XCLIB functions
    // (the first argument is XCLIB function name, others - its arguments)
    template<typename... T>
    void formatLogMessage(const char* func_name, T... args);

    // helper methods for logging
    template<typename T1, typename... T2>
    inline void logHelper(T1 first, T2... last);

    template<typename T>
    inline void logHelper(T arg);

    inline void logHelper(const std::string &str);
    inline void logHelper(const char* str);

    // add to logging string (see formatLogMessage) return value of XCLIB functions in form "-> ret_val"
    inline std::string logXCLIB_Info(const std::string & str, const int result);

    // add to logging string (see formatLogMessage) return value of XCLIB functions in form "-> [...]"
    // for pxd_serialRead it return bytes read from Rx-buffer
    std::string logXCLIB_Info(const std::string & str, const char *res, const int res_len);

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

private:
    std::string _name;
    std::string _value;
};



                /*  CLASS FOR CAMERA EXCEPTIONS  */


class EAGLE_CAMERA_LIBRARY_EXPORT EagleCameraException: public std::exception
{
public:
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

#endif // EAGLE_CAMERA_H
