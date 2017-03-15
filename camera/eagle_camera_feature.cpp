#include <eagle_camera.h>

            /****************************************
            *                                       *
            *   IMPLEMENTATION OF FEATURE CLASSES   *
                                                    *
            ****************************************/

EagleCamera::CameraAbstractFeature::CameraAbstractFeature(const std::string &name,
                                                          const EagleCamera::EagleCameraFeatureType type,
                                                          const EagleCamera::EagleCameraFeatureAccess access):
    _name(name), _type(type), _access(access)
{
}

EagleCamera::CameraAbstractFeature::~CameraAbstractFeature()
{
}


std::string EagleCamera::CameraAbstractFeature::name() const
{
    return _name;
}

EagleCamera::EagleCameraFeatureType EagleCamera::CameraAbstractFeature::type() const
{
    return _type;
}

EagleCamera::EagleCameraFeatureAccess EagleCamera::CameraAbstractFeature::access() const
{
    return _access;
}




             /***********************************************
             *                                              *
             *   IMPLEMENTATION OF A POXY CLASS TO ACCESS   *
             *               EAGLE CAMERA FEATURES          *
             *                                              *
             * *********************************************/



EagleCamera::CameraFeatureProxy::CameraFeatureProxy(EagleCamera *camera):
    _camera(camera)
{
}



EagleCamera::CameraFeatureProxy::operator EagleCamera_StringFeature()
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

    EagleCamera::CameraFeature<std::string> *f =
            static_cast<EagleCamera::CameraFeature<std::string>*>(_camera->currentCameraFeature);

    EagleCamera_StringFeature sf;

    sf._name = _camera->currentCameraFeature->name();
    sf._value = f->get();
    sf._range = f->range();

    return sf;
}


EagleCamera::CameraFeatureProxy & EagleCamera::CameraFeatureProxy::operator = (const std::string & val)
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

    EagleCamera::CameraFeature<std::string> *f = static_cast<EagleCamera::CameraFeature<std::string> *>
                                                            (_camera->currentCameraFeature);

    // check for valid value
    if ( f->range().size() ) {
//        auto it = f->range().begin();
//        for ( ; it != f->range().end(); ++it ) {
//            printf("*IT = %s\n",*it.c_str());
//            if ( !val.compare(*it) ) break;
//        }
        size_t i;
        for ( i = 0; i < f->range().size(); ++i ) {
            if ( !val.compare(f->range()[i]) ) break;
        }
//        if ( it == f->range().end() ) {
        if ( i == f->range().size() ) {
            throw EagleCameraException(0,EagleCamera::Error_InvalidFeatureValue, "Invalid value for String Feature!");
        }
    }

    f->set(val);

    return *this;
}


EagleCamera::CameraFeatureProxy & EagleCamera::CameraFeatureProxy::operator = (const char* val)
{
    return operator = (std::string(val));
}



                    /******************************************
                    *                                         *
                    *   IMPLEMENTATION OF A CLASS TO ACCESS   *
                    *       EAGLE CAMERA STRING FEATURES      *
                    *                                         *
                    * ****************************************/

EagleCamera_StringFeature::EagleCamera_StringFeature():
    _name(), _value(), _range()
{
}


EagleCamera_StringFeature::EagleCamera_StringFeature(EagleCamera_StringFeature &&other):
    EagleCamera_StringFeature()
{
    swap(_name,other._name);
    swap(_value,other._value);
    swap(_range,other._range);
}


EagleCamera_StringFeature::EagleCamera_StringFeature(const EagleCamera_StringFeature &other):
    _name(other._name), _value(other._value), _range(other._range)
{

}


EagleCamera_StringFeature::EagleCamera_StringFeature(EagleCamera::CameraFeatureProxy &feature):
    EagleCamera_StringFeature()
{
    if ( !feature._camera ) { // strange, but should be handled
        throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera object is null");
    }

    if ( !feature._camera->currentCameraFeature ) {
        throw EagleCameraException(0,EagleCamera::Error_NullPointer,"Pointer to camera feature object is null");
    }

    if ( feature._camera->currentCameraFeature->access() == EagleCamera::WriteOnly ) {
        std::string log_str = "Try to get value of write-only feature '" +
                feature._camera->currentCameraFeature->name() + "'!";
        throw EagleCameraException(0,EagleCamera::Error_WriteOnlyFeature,log_str);
    }

    EagleCamera::CameraFeature<std::string> *f = static_cast<EagleCamera::CameraFeature<std::string> *>
                                                             (feature._camera->currentCameraFeature);

    if ( f->type() != EagleCamera::StringType ) {
        throw EagleCameraException(0,EagleCamera::Error_FeatureTypeMismatch,
                                   "Try to get string value of non-string type feature")    ;
    }


    _name = f->name();
    _value = f->get();
    _range = f->range();
}


std::string EagleCamera_StringFeature::name() const
{
    return _name;
}


std::string EagleCamera_StringFeature::value() const
{
    return _value;
}


std::vector<std::string> EagleCamera_StringFeature::range() const
{
    return _range;
}


EagleCamera_StringFeature &EagleCamera_StringFeature::operator =(EagleCamera_StringFeature other)
{
    swap(_name,other._name);
    swap(_value,other._value);
    swap(_range,other._range);

    return *this;
}
