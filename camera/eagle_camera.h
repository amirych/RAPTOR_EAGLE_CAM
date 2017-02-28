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

class EAGLE_CAMERA_LIBRARY_EXPORT EagleCamera
{
    friend class EagleCamera_StringFeature;
public:

    EagleCamera(const char* epix_video_fmt_filename = nullptr);
    EagleCamera(const std::string &epix_video_fmt_filename);

    virtual ~EagleCamera();


    enum EagleCameraFeatureType {Unknown = -1, IntType, FloatType, StringType};
    typedef std::map<std::string,EagleCameraFeatureType> EagleCameraFeatureNameMap;


protected:

            /*   DECLARATION OF A PROXY CLASS TO ACCESS EAGLE CAMERA FEATURES  */

    class EagleCameraFeature {
    public:
        EagleCameraFeature();

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
        EagleCamera::EagleCameraFeatureType _type;
        std::string _name;

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

    };      /*   END OF EagleCameraFeature DECLARATION   */


public:

        // operator to access camera feature

    EagleCameraFeature & operator[](const char* name);
    EagleCameraFeature & operator[](const std::string & name);


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

#endif // EAGLE_CAMERA_H
