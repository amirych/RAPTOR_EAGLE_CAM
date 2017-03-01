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

    enum EagleCameraError { Error_OK, Error_Uninitialized};

protected:

            /*   DECLARATION OF BASE CLASS FOR CAMERA FEATURES  */

    class CameraAbstractFeature {
    public:
        CameraAbstractFeature(const EagleCamera::EagleCameraFeatureType type,
                              const EagleCamera::EagleCameraFeatureAccess access);

        virtual ~CameraAbstractFeature();
    protected:
        EagleCamera::EagleCameraFeatureType _type;
        EagleCamera::EagleCameraFeatureAccess _access;
    };

    typedef std::map<std::string, std::unique_ptr<CameraAbstractFeature>> camera_feature_map_t;
    friend EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES();

            /*   DECLARATION OF TEMPLATE CLASS FOR CAMERA FEATURES  */

    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value ||
                                                            std::is_same<T,std::string>::value>::type>
    class CameraFeature: public CameraAbstractFeature {
    public:
        CameraFeature(const EagleCamera::EagleCameraFeatureAccess access,
                      const std::vector<T> & range,
                      const std::function<T()> getter = nullptr,
                      const std::function<void(const T)> setter = nullptr):
            CameraAbstractFeature(EagleCamera::UnknownType, access), _range(range),
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

    class EagleCameraFeature {
    public:
        EagleCameraFeature();

        void setType(const EagleCameraFeatureType type);
        EagleCameraFeatureType getType() const;

        void setAccess(const EagleCameraFeatureAccess access);
        EagleCameraFeatureAccess getAccess() const;

        // get feature value operators

        template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
        operator T()
        {
            T ret_val;
            switch ( _type ) {
                case EagleCameraFeatureType::IntType: {
                    getInt();
                    ret_val = T(_int_val);
                    break;
                }
                case EagleCameraFeatureType::FloatType: {
                    getFloat();
                    ret_val = T(_float_val);
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
        EagleCameraFeature & operator=(T &&val)
        {
           switch ( _type ) {
               case EagleCameraFeatureType::IntType: {
                   setInt(val);
                   break;
               }
               case EagleCameraFeatureType::FloatType: {
                   setFloat(val);
                   break;
               }
           }
           return *this;
        }

        EagleCameraFeature & operator = (const char* val);           // for string feature
        EagleCameraFeature & operator = (const std::string & val);

    private:
        std::string _name;
        EagleCamera::EagleCameraFeatureType _type;
        EagleCamera::EagleCameraFeatureAccess _access;

        union {
            int64_t _int_val;
            double _float_val;
        };
        std::string _str_val;

        void getInt();
        void getFloat();
        void getString();

        void setInt(const int64_t val);
        void setFloat(const double val);
        void setString();

        static camera_feature_map_t PREDEFINED_CAMERA_FEATURES; // TODO: !!!

    };      /*   END OF EagleCameraFeature DECLARATION   */

    friend class EagleCameraFeature;


public:

                // operator to access camera feature

    EagleCameraFeature & operator[](const char* name);
    EagleCameraFeature & operator[](const std::string & name);

    // operator to execute command

    EagleCameraFeature & operator()(const char* name);
    EagleCameraFeature & operator()(const std::string & name);


protected:

    EagleCameraFeature cameraFeature;

};



class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera_StringFeature
{
public:
    explicit EagleCamera_StringFeature();
    EagleCamera_StringFeature(EagleCamera::EagleCameraFeature &feature);

    std::string value();
    std::string name();

private:
    std::string _name;
    std::string _value;
};



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
