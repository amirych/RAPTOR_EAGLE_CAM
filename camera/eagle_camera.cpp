#include "eagle_camera.h"
#include <eagle_camera_config.h>

#include <xcliball.h>
#include <cameralink_defs.h>

#include <future>
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


int CFITSIO_API_CALL(int err_code, const char *context)
{
    if ( err_code ) {
        throw EagleCameraException(err_code, EagleCamera::Error_FITS_ERR, context);
    }

    return err_code;
}


int CFITSIO_API_CALL(int err_code, const std::string &context)
{
    if ( err_code ) {
        throw EagleCameraException(err_code, EagleCamera::Error_FITS_ERR, context);
    }

    return err_code;
}


static std::string time_stamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    char time_stamp[100];

    struct std::tm buff;
//    buff = *std::localtime(&now_c);
    buff = *localtime(&now_c);

//    std::strftime(time_stamp,sizeof(time_stamp),"%c",&buff);
    strftime(time_stamp,sizeof(time_stamp),"%c",&buff);

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

//extern EagleCamera::camera_feature_map_t INIT_CAMERA_FEATURES();

//EagleCamera::camera_feature_map_t EagleCamera::PREDEFINED_CAMERA_FEATURES = INIT_CAMERA_FEATURES();

size_t EagleCamera::createdObjects = 0;

                /*  CONSTRUCTORS AND DESTRUCTOR  */

EagleCamera::EagleCamera(const char *epix_video_fmt_filename):
    cameraVideoFormatFilename(""),
    cameraUnitmap(-1),
    logLevel(EagleCamera::LOG_LEVEL_VERBOSE), cameraLog(nullptr), logMutex(),
    logMessageStream(), logMessageStreamMutex(),

    CL_ACK_BIT_ENABLED(CL_DEFAULT_ACK_ENABLED), CL_CHK_SUM_BIT_ENABLED(CL_DEFAULT_CK_SUM_ENABLED),

    _frameBuffersNumber(0), _frameCounts(1),
    _imageBuffer(), _currentBufferLength(0),
    _copyFramebuffersFuture(),
    _acquisitionProccessPollingInterval(EAGLE_CAMERA_DEFAULT_ACQUISITION_POLL_INTERVAL),
    _stopCapturing(true), _acquiringFinished(true),
    _lastCameraError(EagleCamera::Error_OK), _lastXCLIBError(0),

    _fitsFilePtr(nullptr),
    _fitsFilename(""), _fitsHdrFilename(""), _fitsMultiImageFormat("EXTEN"),

    _ccdDimension(), _bitsPerPixel(0),
    _serialNumber(0), _buildDate(), _buildCode(),
    _microVersion(), _FPGAVersion(),
    _ADC_Calib(), ADC_LinearCoeffs(),
    _DAC_Calib(), DAC_LinearCoeffs(),

    PREDEFINED_CAMERA_FEATURES(),
    PREDEFINED_CAMERA_COMMANDS(),
    cameraFeature(this), currentCameraFeature(nullptr)
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

    InitCameraFeatures();
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


void EagleCamera::initCamera(const int unitmap, std::ostream *log_file)
{
    std::string log_str;

    cameraLog = log_file;



    // print logging header (ignore logLevel!)
    if ( cameraLog ) {
        for (int i = 0; i < 5; ++i) *cameraLog << std::endl;
        std::string line;
        line.resize(89,'*');
        *cameraLog << line << std::endl;
        *cameraLog << "   " << time_stamp() << std::endl;
        *cameraLog << "   'EAGLE CAMERA' v. " << EAGLE_CAMERA_VERSION_MAJOR << "." << EAGLE_CAMERA_VERSION_MINOR <<
                      " CONTROL SOFTWARE FOR RAPTOR PHOTONICS EAGLE-V 4240 CCD CAMERA" << std::endl;
        *cameraLog << line << std::endl;
        *cameraLog << std::endl << std::flush;
    }

    logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "INITIALIZATION OF CCD CAMERA ...");
    logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Try to configure CameraLink serial connection ...", 1);

    try {
        if ( unitmap <= 0 ) {
            log_str = "Unitmap must be greater than 0! (trying to set to " + std::to_string(unitmap) + ")";
            throw EagleCameraException(0,EagleCamera::Error_InvalidUnitmap, log_str);
        }

        cameraUnitmap = unitmap;

        formatLogMessage("pxd_serialConfigure",0,CL_DEFAULT_BAUD_RATE,CL_DEFAULT_DATA_BITS,0,CL_DEFAULT_STOP_BIT,0,0,0);
        XCLIB_API_CALL( pxd_serialConfigure(cameraUnitmap,0,CL_DEFAULT_BAUD_RATE,CL_DEFAULT_DATA_BITS,0,CL_DEFAULT_STOP_BIT,0,0,0),
                        logMessageStream.str());

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logMessageStream.str());
        }

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Try to reset FPGA ...", 1);
        resetFPGA();

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Try to get manufacturer data ...", 1);
        getManufactureData();

        int ntab = 3;

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Found Raptor CCD camera: ", 1);
        log_str = "Serial number: " + std::to_string(_serialNumber);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, log_str, ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Build date (DD/MM/YY): " + _buildDate, ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Build code: " + _buildCode, ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Micro version: " + _microVersion, ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "FPGA version: " + _FPGAVersion, ntab);



        // get framebuffer dimensions

        log_str = "pxd_imageYdim()";
        XCLIB_API_CALL( _ccdDimension[0] = pxd_imageXdim(), log_str );
        log_str = "pxd_imageYdim()";
        XCLIB_API_CALL( _ccdDimension[1] = pxd_imageYdim(), log_str );
        log_str = "pxd_imageBdim()";
        XCLIB_API_CALL( _bitsPerPixel = pxd_imageBdim(), log_str ); // Eagle-V is non-color camera. do not read number of colors
//        log_str = "pxd_imageCdim()";
//        XCLIB_API_CALL( cc = pxd_imageCdim(), log_str );

        log_str = "pxd_imageZdim()";
        XCLIB_API_CALL( _frameBuffersNumber = pxd_imageZdim(), log_str);

        _imageBuffer.resize(_frameBuffersNumber);
        _copyFramebuffersFuture.resize(_frameBuffersNumber);

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "CCD dimensions: [" +
                  std::to_string(_ccdDimension[0]) + ", " + std::to_string(_ccdDimension[1]) + "] pixels", ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "CCD bits per pixel: " + std::to_string(_bitsPerPixel), ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Number of frame buffers: " + std::to_string(_frameBuffersNumber), ntab);

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Set initial camera configuration ...", 1);

        setInitialState();

    } catch ( EagleCameraException &ex ) {
        logToFile(ex);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_ERROR, "CANNOT INITIALIZE CAMERA");
        return;
    }

    logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "INITIALIZATION COMPLETED SUCCESSFULLY");
}


void EagleCamera::setInitialState()
{
//    setTriggerMode(false, false, false, false, false, false); // IDLE mode
    setTriggerMode(0x0);  // IDLE mode
    setCtrlRegister(true, false, true); // gain to HIGH, TEC is ON

    setXBIN(1);
    setYBIN(1);
    setROIWidth(_ccdDimension[0]);
    setROIHeight(_ccdDimension[1]);

    setReadoutRate("SLOW");
    setReadoutMode("NORMAL");

    // set initial upper range for CCD geometry

    CameraFeature<IntegerType> *f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROILeft"].get());
    f->set_range({1,_ccdDimension[0]});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROITop"].get());
    f->set_range({1,_ccdDimension[1]});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROIWidth"].get());
    f->set_range({1,_ccdDimension[0]});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROIHeight"].get());
    f->set_range({1,_ccdDimension[1]});
}


void EagleCamera::resetCamera() // full reset (microcontroller and FPGA)
{
    stopAcquisition();
    _acquiringFinished = true;

    _lastCameraError = EagleCamera::Error_OK;
    _lastXCLIBError = 0;

    resetMicro();
    resetFPGA();
}


void EagleCamera::startAcquisition()
{
    _stopCapturing = false;

    _lastCameraError = EagleCamera::Error_OK;
    _lastXCLIBError = 0;

    IntegerType roi_width = (*this)["ROIWidth"];
    IntegerType roi_height = (*this)["ROIHeight"];
    double expTime = (*this)["ExposureTime"];

    double timeout = expTime + 180.0;

    IntegerType Nelem = roi_width*roi_height;
    size_t Nbuffs;
    if ( Nelem != _currentBufferLength ) {
        _currentBufferLength = Nelem;
        Nbuffs = (_frameBuffersNumber <= _frameCounts) ? _frameBuffersNumber : _frameCounts;
        for ( size_t i = 0; i < Nbuffs; ++i ) {
            _imageBuffer[i] = std::unique_ptr<ushort[]>(new ushort[_currentBufferLength]);
        }
    }

//    std::vector<std::future<void>> copy_buffer(Nbuffs);

    acquisitionIsAboutToStart();

    _acquiringFinished = false;

    // start acquisition proccess

    auto run_exp = std::async(std::launch::async, [&]{

        std::function<void()> waitCopyThreads = [&]{
            for (size_t i = 0; i < _copyFramebuffersFuture.size(); ++i)
                _copyFramebuffersFuture[i].wait_for(std::chrono::seconds(EAGLE_CAMERA_DEFAULT_BUFFER_TIMEOUT));
        };

        _currentBuffer = 1;

        for ( size_t i_frame = 0; i_frame < _frameCounts; ++i_frame ) {
            if ( _stopCapturing ) { // user abort acquiring process
                waitCopyThreads();
                break;
            }

            captureAndCopyImage(i_frame,timeout);     // 'arm' grabber to capture image and, after that, copy
                                                      // image asynchronously from grabber framebuffer

            setTriggerMode(CL_TRIGGER_MODE_SNAPSHOT); // start single exposure

            if ( _currentBuffer < _frameBuffersNumber ) ++_currentBuffer;
            else _currentBuffer = 1;

            if ( _stopCapturing ) { // user abort acquiring process
                waitCopyThreads();
                break;
            }

            // wait for end of copy operation of previous (in circular manner) buffer with the same ID
            auto status = _copyFramebuffersFuture[_currentBuffer].wait_for(
                        std::chrono::seconds(EAGLE_CAMERA_DEFAULT_BUFFER_TIMEOUT));

            if ( status == std::future_status::timeout ) {
                _lastCameraError = EagleCamera::Error_CopyBufferTimeout;
                break;
            }
        }

        _acquiringFinished = true;
    });


    // start polling a state of acquisition proccess

    auto pollAcqProccess = std::async(std::launch::async, [&] {
        while ( !_acquiringFinished ) {
            std::this_thread::sleep_for(std::chrono::milliseconds(_acquisitionProccessPollingInterval));
        }
        acquisitionIsAboutToStop();
    });
}


void EagleCamera::stopAcquisition()
{
    setTriggerMode(CL_TRIGGER_MODE_ABORT_CURRENT_EXP); // set abort exp bit
    _stopCapturing = true;
}


void EagleCamera::acquisitionIsAboutToStart()
{
    if ( !_fitsFilename.compare("") ) return;

    // create FITS file
    try {
        int status = 0;
        std::unique_ptr<long[]> naxes;
        long naxis = 2;

        IntegerType xsize = (*this)["ROIWidth"];
        IntegerType ysize = (*this)["ROIHeight"];

        bool exten_format = (!_fitsMultiImageFormat.compare("EXTEN")) ? true : false;

        if ( _frameCounts > 1 ) { // multi image
            if ( exten_format ) { // FITS file is sequence of 2-dim IMAGE extentions
                naxes = std::unique_ptr<long[]>(new long[2]);
            } else { // save image as a primary array cube
                naxis = 3;
                naxes = std::unique_ptr<long[]>(new long[3]);
                naxes[2] = _frameCounts;
            }
        } else {
            naxes = std::unique_ptr<long[]>(new long[2]); // FITS file is just 2-dim primary array.
        }

        naxes[0] = xsize;
        naxes[1] = ysize;


        std::string filename = "!" + _fitsFilename; // add '!' to overwrite existing file

        formatFitsLogMessage("fits_create_file",filename,(void*)&status);

        CFITSIO_API_CALL( fits_create_file(&_fitsFilePtr, filename.c_str(), &status), logMessageStream.str() );
        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, logMessageStream.str());
        }

        if ( exten_format ) { // creating empty primary array
            formatFitsLogMessage("fits_create_img",USHORT_IMG,0,0,(void*)&status);
            CFITSIO_API_CALL( fits_create_img(_fitsFilePtr,USHORT_IMG,0,0,&status), logMessageStream.str());

            if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
                logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, logMessageStream.str());
            }
        } else {
            formatFitsLogMessage("fits_create_img",USHORT_IMG,naxis,naxes.get(),(void*)&status);
            CFITSIO_API_CALL( fits_create_img(_fitsFilePtr,USHORT_IMG,naxis,naxes.get(),&status),
                              logMessageStream.str());

            if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
                logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, logMessageStream.str());
            }
        }
    } catch ( EagleCameraException &ex ) {
        if ( ex.Camera_Error() == EagleCamera::Error_FITS_ERR ) {
            fits_close_file(_fitsFilePtr,nullptr);
            _fitsFilePtr = nullptr;
        }
        throw ex;
    } catch ( std::bad_alloc &ex ) {
        throw EagleCameraException(0,EagleCamera::Error_MemoryAllocation, "Cannot allocate memory");
    }
}


void EagleCamera::imageReady(IntegerType *frame_no, const ushort *image_buffer)
{

}


void EagleCamera::acquisitionIsAboutToStop()
{
    if ( (_lastCameraError != EagleCamera::Error_OK) || (_lastXCLIBError != 0) ) {
        throw EagleCameraException(_lastXCLIBError, _lastCameraError, "Acquisition proccess failed");
    }

    if ( !_fitsFilename.compare("") ) return;

    if ( _fitsFilePtr ) { // wait for all capture-and-copy-image threads finished and close FITS file
        for (size_t i = 0; i < _copyFramebuffersFuture.size(); ++i)
            _copyFramebuffersFuture[i].wait_for(std::chrono::seconds(EAGLE_CAMERA_DEFAULT_BUFFER_TIMEOUT));

        formatFitsLogMessage("fits_close_file",0);
        CFITSIO_API_CALL( fits_close_file(_fitsFilePtr,nullptr), logMessageStream.str());

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, logMessageStream.str());
        }
    }
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
//        std::cout << "[] access = " << currentCameraFeature->access() << "\n";
//        std::cout << "[] name = " << currentCameraFeature->name() << "\n";
//        std::cout << "[] type = " << currentCameraFeature->type() << "\n";
    } else {
        currentCameraFeature = nullptr;
        std::string log_str = "'" + name + "' is unknown camera feature!";
        throw EagleCameraException(0,EagleCamera::Error_UnknowFeature,log_str);
    }

    return cameraFeature;
}


EagleCamera::CameraFeatureProxy & EagleCamera::operator[](const char* name)
{
    return operator [](std::string(name));
}



void EagleCamera::logToFile(const EagleCamera::EagleCameraLogIdent ident, const std::string &log_str, const int indent_tabs)
{
    if ( !cameraLog ) return;

    if ( logLevel == EagleCamera::LOG_LEVEL_QUIET ) return;

    logMutex.lock();

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

    logMutex.unlock();
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


// Capturing control

void EagleCamera::captureAndCopyImage(IntegerType frame_no, const double timeout) NOEXCEPT_DECL
{
    ulong timeout_msecs = static_cast<ulong>(1000.0*timeout);

    char col[] = "Gray";

    ushort* buff_ptr = (_imageBuffer[_currentBuffer-1]).get();

    try {
        formatLogMessage("pxd_doSnap", _currentBuffer, timeout);

        XCLIB_API_CALL( pxd_doSnap(cameraUnitmap, _currentBuffer, timeout_msecs), logMessageStream.str());
        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logMessageStream.str());
        }

        // ran asynchronously
        IntegerType i_buff = _currentBuffer;
        _copyFramebuffersFuture[_currentBuffer] = std::async(std::launch::async, [&] {
            //                std::string log_str = "pxd_readushort(" + std::to_string(cameraUnitmap) + ", 0, 0, -1, -1, " +
            //                        pointer_to_str(buff_ptr) + ", " + std::to_string(_currentBufferLength) + ", '" + col + "')";
            formatLogMessage("pxd_readushort",i_buff, 0,0,-1,-1,(void*)buff_ptr,_currentBufferLength,col);
            XCLIB_API_CALL( pxd_readushort(cameraUnitmap, i_buff, 0, 0, -1, -1,
                                           buff_ptr, _currentBufferLength, col), logMessageStream.str() );

            imageReady(&frame_no, buff_ptr); // invoke user's function to process buffer
        });
    } catch ( EagleCameraException &ex ) {
        _lastCameraError = ex.Camera_Error();
        _lastXCLIBError = ex.XCLIB_Error();
        _stopCapturing = true;
    }
}



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
//        buff = std::unique_ptr<char[]>(new char[nbytes]);
    } else { // just read all available bytes in Rx-buffer
        if ( !nbytes ) return nbytes; // no data? do not wait and return immediately!
    }

    buff = std::unique_ptr<char[]>(new char[nbytes]);
    buff_ptr = buff.get();

    formatLogMessage("pxd_serialRead", 0, (void*)buff_ptr, nbytes);
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

        formatLogMessage("pxd_serialWrite",0,(void*)val.data(),val.size());
        XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, (char*)val.data(), val.size()), logMessageStream.str() );

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), nbytes));
        }

        // write mandatory End-of-Transmision byte
        char ack = CL_ETX;

        formatLogMessage("pxd_serialWrite",0,(void*)&ack,1);
        XCLIB_API_CALL( N = pxd_serialWrite(cameraUnitmap, 0, &ack, 1), logMessageStream.str() );

        if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
            logToFile( EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(logMessageStream.str(), N));
        }

        nbytes += N;

        if ( CL_CHK_SUM_BIT_ENABLED ) { // compute check sum
            char sum = val[0];
            for ( int i = 1; i < val.size(); ++i ) sum ^= val[i];
            sum ^= CL_ETX;

            formatLogMessage("pxd_serialWrite",0,(void*)&sum,1);
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
    return ( state & CL_SYSTEM_STATE_CK_SUM ) ? true : false;
}


inline bool EagleCamera::is_ack_enabled(const unsigned char state)
{
    return ( state & CL_SYSTEM_STATE_ACK ) ? true : false;
}

inline bool EagleCamera::is_fpga_boot_ok(const unsigned char state)
{
    return (state & CL_SYSTEM_STATE_FPGA_BOOT_OK) ? true : false;
}


inline bool EagleCamera::is_fpga_in_reset(const unsigned char state)
{
    return !(state & CL_SYSTEM_STATE_FPGA_RST_HOLD) ? true : false;
}


inline bool EagleCamera::is_fpga_eprom_enabled(const unsigned char state)
{
    return (state & CL_SYSTEM_STATE_FPGA_EPROM_COMMS) ? true : false;
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


bool EagleCamera::is_high_gain_enabled(const unsigned char state)
{
    return !(state & CL_FPGA_CTRL_REG_HIGH_GAIN) ? true : false;
}


bool EagleCamera::is_reset_temp_trip(const unsigned char state)
{
    return (state & CL_FPGA_CTRL_REG_TMP_TRIP_RST) ? true : false;
}


bool EagleCamera::is_tec_enabled(const unsigned char state)
{
    return (state & CL_FPGA_CTRL_REG_ENABLE_TEC) ? true : false;
}


void EagleCamera::setTriggerMode(unsigned char mode)
{
    byte_vector_t comm = CL_COMMAND_WRITE_VALUE;
    comm[3] = 0xD4;
    comm[4] = mode;

    cl_exec(comm);
}


void EagleCamera::setTriggerMode(bool snapshot, bool enable_fixed_frame_rate, bool start_cont_seq,
                                 bool abort_exp, bool enable_extern_trigger, bool enable_rising_edge)
{
    unsigned char mode = 0;

    if ( snapshot ) mode |= CL_TRIGGER_MODE_SNAPSHOT;
    if ( enable_fixed_frame_rate ) mode |= CL_TRIGGER_MODE_FIXED_FRAME_RATE;
    if ( start_cont_seq ) mode |= CL_TRIGGER_MODE_CONTINUOUS_SEQ;
    if ( abort_exp ) mode |= CL_TRIGGER_MODE_ABORT_CURRENT_EXP;
    if ( enable_extern_trigger ) mode |= CL_TRIGGER_MODE_EXT_TRIGGER;
    if ( enable_rising_edge ) mode |= CL_TRIGGER_MODE_ENABLE_RISING_EDGE;

    byte_vector_t comm = CL_COMMAND_WRITE_VALUE;
    comm[3] = 0xD4;
    comm[4] = mode;

    cl_exec(comm);
}


unsigned char EagleCamera::getTriggerMode()
{
    byte_vector_t addr_comm = CL_COMMAND_SET_ADDRESS;
    byte_vector_t comm = CL_COMMAND_READ_VALUE;
    byte_vector_t val(1);

    addr_comm[3] = 0xD4;

    cl_exec(addr_comm);
    cl_exec(comm,val);

    return val[0];
}


bool EagleCamera::is_fixed_frame_rate(unsigned char mode)
{
    return (mode & CL_TRIGGER_MODE_FIXED_FRAME_RATE) ? true : false;
}


bool EagleCamera::is_cont_seq(unsigned char mode)
{
    return (mode & CL_TRIGGER_MODE_CONTINUOUS_SEQ) ? true : false;
}


bool EagleCamera::is_ext_trigger(unsigned char mode)
{
    return (mode & CL_TRIGGER_MODE_EXT_TRIGGER) ? true : false;
}


bool EagleCamera::is_rising_edge(unsigned char mode)
{
    return (mode & CL_TRIGGER_MODE_ENABLE_RISING_EDGE) ? true : false;
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
    unsigned char state = getSystemState();
    setSystemState(is_chk_sum_enabled(state), is_ack_enabled(state), is_fpga_in_reset(state), true);

    byte_vector_t comm = CL_COMMAND_GET_MANUFACTURER_DATA_1;
    cl_exec(comm);

    comm = CL_COMMAND_GET_MANUFACTURER_DATA_2;
    byte_vector_t value(18);
    cl_exec(comm,value);

    setSystemState(is_chk_sum_enabled(state), is_ack_enabled(state), is_fpga_in_reset(state), false);

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


    // get microcontroller version

    comm = {0x56};

    byte_vector_t val(2);

    cl_exec(comm,val);

    _microVersion = std::to_string(val[0]) + "." + std::to_string(val[1]);


    // get FPGA version

    byte_vector_t addr = {0x7E, 0x7F};

    val = readRegisters(addr);

    _FPGAVersion = std::to_string(val[0]) + "." + std::to_string(val[1]);
}


// convert 5-bytes (40 bit) FPGA registers values to number of counts
EagleCamera::IntegerType EagleCamera::fpga40BitsToCounts(const byte_vector_t &vals)
{
    int64_t counts = 0;

    counts |= vals[0];
    for ( int i = 1; i < 5 ; ++i ) {
        counts <<= 8;
        counts |= vals[i];
    }

    return counts;
}


// convert number of counts to 5-bytes (40 bit) FPGA registers values
EagleCamera::byte_vector_t EagleCamera::countsToFPGA40Bits(const EagleCamera::IntegerType counts)
{
    size_t N = 5;
    byte_vector_t value(N);
    int64_t cnts = counts;

    for ( size_t i = 1; i <= N ; ++i ) {
        value[N-i] = (cnts & 0xFF);
        cnts >>= 8;
    }

    return value;
}


EagleCamera::IntegerType EagleCamera::fpga16BitsToInteger(const byte_vector_t &vals)
{
    IntegerType v = 0;

    v = ((vals[0] & 0x0F) << 8) + vals[1];

    return v;
}


EagleCamera::byte_vector_t EagleCamera::integerToFPGA12Bits(const EagleCamera::IntegerType val)
{
    byte_vector_t value(2);

    value[0] = (val & 0x0F00) >> 8; // MM
    value[1] = val & 0xFF;          // LL

    return value;
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


std::string EagleCamera::getMicroVersion()
{
    return _microVersion;
}


std::string EagleCamera::getFPGAVersion()
{
    return _FPGAVersion;
}


    // format logging message

template<typename ...T>
void EagleCamera::formatLogMessage(const char* func_name, T ...args)
{

    logMessageStreamMutex.lock();

    logMessageStream.str("");
    logMessageStream << func_name << "(" << cameraUnitmap << ", ";
    logHelper(args...);
    logMessageStream << ")";

    logMessageStreamMutex.unlock();
}


template<typename ...T>
void EagleCamera::formatFitsLogMessage(const char* func_name, T ...args)
{

    logMessageStreamMutex.lock();

    logMessageStream.str("");
    logMessageStream << func_name << "(" << (void*)_fitsFilePtr << ", ";
    logHelper(args...);
    logMessageStream << ")";

    logMessageStreamMutex.unlock();
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


void EagleCamera::logHelper(const void *addr)
{
    logMessageStream << std::hex << addr << std::dec;
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
    for ( int i = 1; i < res_len; ++i ) ss << ", " << std::hex << ((uint16_t)res[i] & 0x00FF) << std::dec;
    ss << "]";

    return str + ss.str();
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
