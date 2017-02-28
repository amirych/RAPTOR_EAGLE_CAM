

             /***********************************************
             *                                              *
             *   IMPLEMENTATION OF A POXY CLASS TO ACCESS   *
             *               EAGLE CAMERA FEATURES          *
             *                                              *
             * *********************************************/


#include <eagle_camera.h>

EagleCamera::EagleCameraFeature::EagleCameraFeature()
{

}


void EagleCamera::EagleCameraFeature::setType(const EagleCamera::EagleCameraFeatureType type)
{
    _type = type;
}


EagleCamera::EagleCameraFeatureType EagleCamera::EagleCameraFeature::getType() const
{
    return _type;
}


void EagleCamera::EagleCameraFeature::setAccess(const EagleCamera::EagleCameraFeatureAccess access)
{
    _access = access;
}


EagleCamera::EagleCameraFeatureAccess EagleCamera::EagleCameraFeature::getAccess() const
{
    return _access;
}


                        /*  PRIVATE METHODS  */

void EagleCamera::EagleCameraFeature::setInt(const int64_t val)
{
    if ( _type != EagleCamera::IntType ) { // TODO: exception!!!

    }

    if ( _access == EagleCamera::ReadOnly ) { // TODO: exception!!!

    }

    if ( !_name.compare("HBIN") ) {

    } else if ( !_name.compare("VBIN") ) {

    } else if ( !_name.compare("ROILeft") ) {

    } else if ( !_name.compare("ROITop") ) {

    } else if ( !_name.compare("ROIWidth") ) {

    } else if ( !_name.compare("ROIHeight") ) {

    }
}


void EagleCamera::EagleCameraFeature::setFloat(const double val)
{
    if ( _type != EagleCamera::FloatType ) { // TODO: exception!!!

    }

    if ( _access == EagleCamera::ReadOnly ) { // TODO: exception!!!

    }

    if ( !_name.compare("ExposureTime") ) {

    } else if ( !_name.compare("FrameRate") ) {

    } else if ( !_name.compare("ShutterOpenDelay") ) {

    } else if ( !_name.compare("ShutterCloseDelay") ) {

    } else if ( !_name.compare("TECSetPoint") ) {

    }

}
