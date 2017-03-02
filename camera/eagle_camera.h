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

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include <exception>

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

    enum EagleCameraError { Error_OK, Error_Uninitialized, Error_NullPointer, Error_UnknowCommand,
                            Error_UnknowFeature, Error_ReadOnlyFeature, Error_WriteOnlyFeature};

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


        /*  feauture control members  */

    CameraFeatureProxy cameraFeature;
    CameraAbstractFeature *currentCameraFeature;

    static camera_feature_map_t PREDEFINED_CAMERA_FEATURES;

        /*  command control members  */

    camera_command_map_t PREDEFINED_CAMERA_COMMANDS;
    void InitCameraCommands();


        /*  CAMERALINK serial port methods  */

    void resetMicro();
    void resetFPGA();

    void setSystemState(unsigned char state);
    void setCtrlRegister(unsigned char reg);




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
