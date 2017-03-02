#include "eagle_camera.h"

#include <xcliball.h>

#include <chrono>

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
    cameraUnitmap(1),
    logLevel(EagleCamera::LOG_LEVEL_VERBOSE), cameraLog(nullptr),
    cameraFeature(this), currentCameraFeature(nullptr),
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
