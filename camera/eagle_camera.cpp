#include "eagle_camera.h"
#include <eagle_camera_config.h>

#include <xcliball.h>
#include <cameralink_defs.h>

#include <cstring>
#include <cmath>
#include <algorithm>

#include <iostream>

                        /*********************************************
                        *                                            *
                        *      EagleCamera CLASS IMPLEMENTATION      *
                        *                                            *
                        *********************************************/


                            /*  Auxiliary functions */

// function deletes leading and trailing whitesaces
std::string trim_spaces(const std::string& s, const std::string& whitespace = " \t")
{
    std::string str = s;

    std::size_t strBegin = str.find_first_not_of(whitespace);

    if (strBegin == std::string::npos) {
        str.clear();
        return str; // empty string
    }

    std::size_t strEnd = str.find_last_not_of(whitespace);
    std::size_t strRange = strEnd - strBegin + 1;

    str.assign(str, strBegin, strRange);

    return str;
}


static std::string get_float_fmt(double value, int num_digits)
{
    double lg = std::log10(value);
    if ( lg < 0 ) lg = 0;

    int n = ( lg < 0 ) ? 1 : std::floor(lg) + 1;

    n += num_digits;

    std::string fmt = "F" + std::to_string(n+1) + "." + std::to_string(num_digits);

    return fmt;
}


static std::string time_stamp(const char* fmt = nullptr, bool utc = false,
                              std::chrono::system_clock::time_point *now_point = nullptr)
{
    auto now = std::chrono::system_clock::now();
    if ( now_point ) *now_point = now;
    auto now_c = std::chrono::system_clock::to_time_t(now);

    char time_stamp[100];
    char time_stamp1[100];

    struct std::tm buff;
    if ( utc ) {
        buff = *gmtime(&now_c);
    } else {
        buff = *localtime(&now_c);
    }

    if ( !fmt ) {
        strftime(time_stamp,sizeof(time_stamp),"%c",&buff);
    } else {
        strftime(time_stamp1,sizeof(time_stamp1),fmt,&buff);
        std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >
                                       (now.time_since_epoch());
        std::chrono::seconds ss = std::chrono::duration_cast< std::chrono::seconds >
                                       (now.time_since_epoch());
//        int64_t milli = ms.count() - ss.count()*1000;
        int64_t tens = (ms.count() - ss.count()*1000)/100;
//        std::cout << "MS: " << milli << "\n";
//        std::snprintf(time_stamp,sizeof(time_stamp),"%s.%03li",time_stamp1,milli);
#ifdef _MSC_VER
        _snprintf(time_stamp,sizeof(time_stamp),"%s.%li",time_stamp1,tens);
#else
        snprintf(time_stamp,sizeof(time_stamp),"%s.%li",time_stamp1,tens);
#endif
    }

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



                /*  INIT STATIC MEMBERS  */

size_t EagleCamera::createdObjects = 0;

                /*  CONSTRUCTORS AND DESTRUCTOR  */

EagleCamera::EagleCamera(const char *epix_video_fmt_filename):
//EagleCamera::EagleCamera():
    cameraVideoFormatFilename(""),
    cameraUnitmap(-1),
    logLevel(EagleCamera::LOG_LEVEL_VERBOSE), cameraLog(nullptr), logMutex(),
    logMessageStream(), logMessageStreamMutex(),

    CL_ACK_BIT_ENABLED(CL_DEFAULT_ACK_ENABLED), CL_CHK_SUM_BIT_ENABLED(CL_DEFAULT_CK_SUM_ENABLED),

    _imageStartX(0), _imageStartY(0), _imageXDim(0), _imageYDim(0),
    _imagePixelsNumber(0),
    _frameBuffersNumber(EAGLE_CAMERA_DEFAULT_NUMBER_OF_BUFFERS),
    _frameCounts(1),
    _startExpTimestamp(), _expTime(0),
    _ccdTemp(), _pcbTemp(),
    _startExpTimepoint(), _stopExpTimepoint(),
    _imageBuffer(), _currentBufferLength(0), _usedBuffersNumber(0),
    _capturingTimeoutGap(EAGLE_CAMERA_DEFAULT_CAPTURING_TIMEOUT_GAP),
    _acquisitionProccessThreadFuture(),

    _acquisitionProccessPollingInterval(EAGLE_CAMERA_DEFAULT_ACQUISITION_POLL_INTERVAL),
    _stopCapturing(true), _acquiringFinished(true),
    _lastCameraError(EagleCamera::Error_OK), _lastXCLIBError(0),

    _fitsWritingTimeout(EAGLE_CAMERA_DEFAULT_FITS_WRITING_TIMEOUT),
    _fitsFilePtr(nullptr),
    _fitsFilename(""), _fitsHdrFilename(""),
    _fitsDataFormat(EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_EXTEN),

    _ccdDimension(), _bitsPerPixel(0),
    _serialNumber(0), _buildDate(), _buildCode(),
    _microVersion(), _FPGAVersion(),
//    _ADC_Calib{0,0}, ADC_LinearCoeffs{0,0},
//    _DAC_Calib{0,0}, DAC_LinearCoeffs{0,0},
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

        } else { // use of default format file for Eagle-V camera
            std::string log_str = "pxd_PIXCIopen(\"\",\"DEFAULT\",\"\")";
            XCLIB_API_CALL( pxd_PIXCIopen("","DEFAULT",""), log_str);

            #include "raptor_eagle-v.fmt"      // exported from XCAP (Linux 64-bit!): bin 1x1, full CCD frame
            pxd_videoFormatAsIncludedInit(0);

            log_str = "pxd_videoFormatAsIncluded(0)";
            XCLIB_API_CALL(  pxd_videoFormatAsIncluded(0), log_str);

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
    if ( !_acquiringFinished ) {
#ifndef NDEBUG
        std::cout << "\nDTOR: WAIT FOR ACQ FINISHED\n";
#endif
        if ( _acquisitionProccessThreadFuture.valid() ) {
            try {
                _acquisitionProccessThreadFuture.get(); // wait for acquiring ...  TODO: stop proccess ???!!!
            } catch ( EagleCameraException &ex ) {
                logToFile(ex);
            }
        }
    }

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

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Try to reset FPGA ...", 1);
        bool status = resetFPGA();
        if ( !status ) throw EagleCameraException(0,EagleCamera::Error_Uninitialized,"");

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
//        XCLIB_API_CALL( _frameBuffersNumber = pxd_imageZdim(), log_str);
        int Nbuff;
        XCLIB_API_CALL( Nbuff = pxd_imageZdim(), log_str);

        _imageBuffer.resize(_frameBuffersNumber);
//        _copyFramebuffersFuture.resize(_frameBuffersNumber);
//        _currentBufferLength = _ccdDimension[0]*_ccdDimension[1];

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "CCD dimensions: [" +
                  std::to_string(_ccdDimension[0]) + ", " + std::to_string(_ccdDimension[1]) + "] pixels", ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "CCD bits per pixel: " + std::to_string(_bitsPerPixel), ntab);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Number of frame buffers: " + std::to_string(Nbuff), ntab);
//        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Number of frame buffers: " + std::to_string(_frameBuffersNumber), ntab);

        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "Set initial camera configuration ...", 1);

        setInitialState();

    } catch ( EagleCameraException &ex ) {
        logToFile(ex);
        logToFile(EagleCamera::LOG_IDENT_CAMERA_ERROR, "CANNOT INITIALIZE CAMERA");
        throw;
    }

    logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, "INITIALIZATION COMPLETED SUCCESSFULLY");
}


void EagleCamera::setInitialState()
{
//    setTriggerMode(false, false, false, false, false, false); // IDLE mode
    setTriggerMode(0x0);  // IDLE mode
//    setCtrlRegister(true, false, true); // gain to HIGH, TEC is ON
    setCtrlRegister(true, false, false); // gain to HIGH, TEC is ON

    setXBIN(1);
    setYBIN(1);
    setROILeft(1);
    setROITop(1);

    setROIHeight(0); // mystic command!!!! without this camera does not generate snapshot trigger!!!

//    setTriggerMode(CL_TRIGGER_MODE_ABORT_CURRENT_EXP);
//    setTriggerMode(0x0);  // IDLE mode

    setROIWidth(_ccdDimension[0]);
    setROIHeight(_ccdDimension[1]);

    setReadoutRate(EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST);
//    setReadoutRate("SLOW");
    setReadoutMode(EAGLE_CAMERA_FEATURE_READOUT_MODE_NORMAL);
//    setReadoutMode("NORMAL");

    // set initial upper range for CCD geometry

//    std::cout << "SET INIT RANGE: dim = " << _ccdDimension[0] << ", " << _ccdDimension[1] << "\n";

    CameraFeature<IntegerType> *f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROILeft"].get());
    f->set_range({1,_ccdDimension[0]-1});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROITop"].get());
    f->set_range({1,_ccdDimension[1]-1});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROIWidth"].get());
    f->set_range({1,_ccdDimension[0]});

    f = static_cast<CameraFeature<IntegerType> *>( PREDEFINED_CAMERA_FEATURES["ROIHeight"].get());
    f->set_range({1,_ccdDimension[1]});
}


void EagleCamera::resetCamera() // full reset (microcontroller and FPGA)
{
    if ( cameraUnitmap < 1 ) return; // camera does not initialized. nothing to do.

    stopAcquisition();
    _acquiringFinished = true;

    _lastCameraError = EagleCamera::Error_OK;
    _lastXCLIBError = 0;

    resetMicro();
    resetFPGA();

    setInitialState();
}


void EagleCamera::startAcquisition()
{
    // main thread blocking part:

    if ( !_acquiringFinished ) throw EagleCameraException(0,EagleCamera::Error_CameraIsAcquiring,"Camera is acquiring");

    if ( _fitsFilename.empty() ) return;

    _stopCapturing = false;

    _lastCameraError = EagleCamera::Error_OK;
    _lastXCLIBError = 0;

    // get output image dimension

    _imageStartX = (*this)[EAGLE_CAMERA_FEATURE_ROI_LEFT_NAME];
    _imageStartY = (*this)[EAGLE_CAMERA_FEATURE_ROI_TOP_NAME];

    _imageXDim = (*this)[EAGLE_CAMERA_FEATURE_ROI_WIDTH_NAME];
    _imageYDim = (*this)[EAGLE_CAMERA_FEATURE_ROI_HEIGHT_NAME];

    _expTime = (*this)[EAGLE_CAMERA_FEATURE_EXPTIME_NAME];


    _imagePixelsNumber = _imageXDim*_imageYDim;

    // compute number of grabber framebuffer lines one needs to store whole image.
    // This API does not reconfigure grabber in case of changing binning factor or
    // ROI size. Thus, read image will be stored in the same initial (binning 1x1,
    // ROI = full resoluion CCD image) framebuffer. In this case, to compute size of
    // image buffer for 'pxd_readushort' XCLIB API function one needs to know
    // how many whole framebuffer lines read image occupys.

    _frameBufferLines = static_cast<IntegerType>(std::ceil(1.0*_imagePixelsNumber/_ccdDimension[0]));
    IntegerType Nelem = _frameBufferLines*_ccdDimension[0];

#ifndef NDEBUG
    std::cout << "IMAGE DIMENSION: [" << _imageXDim << ", " << _imageYDim << "] ([Width, Height])\n";
    std::cout << "ROI TOP-LEFT COORDINATES: [" << _imageStartX << ", " << _imageStartY << "]\n";
    std::cout << "NUMBER OF PIXELS IN THE IMAGE: " << _imagePixelsNumber << "\n";
    std::cout << "NUMBER OF FRAMEBUFFER LINES: " << _frameBufferLines << "\n";
    std::cout << "NUMBER OF API FRAME BUFFERS: " << _frameBuffersNumber << "\n";
#endif

    size_t Nbuffs = (_frameBuffersNumber <= _frameCounts) ? _frameBuffersNumber : _frameCounts;
    try {
        if ( Nelem != _currentBufferLength ) {
            _currentBufferLength = Nelem;
//            Nbuffs = (_frameBuffersNumber <= _frameCounts) ? _frameBuffersNumber : _frameCounts;
            for ( size_t i = 0; i < Nbuffs; ++i ) {
                _imageBuffer[i] = std::unique_ptr<ushort[]>(new ushort[_currentBufferLength]);
            }
        } else {
            if ( _usedBuffersNumber < Nbuffs ) {
                for ( size_t i = _usedBuffersNumber; i < Nbuffs; ++i ) {
                    _imageBuffer[i] = std::unique_ptr<ushort[]>(new ushort[_currentBufferLength]);
                }
            }
        }
        _usedBuffersNumber = Nbuffs;
    } catch ( std::bad_alloc ) {
        throw EagleCameraException(0, EagleCamera::Error_MemoryAllocation, "Cannot allocate memory for image buffer");
    }


    _acquiringFinished = false;
    _startExpTimestamp.resize(_frameCounts);
    _ccdTemp.resize(_frameCounts);
    _pcbTemp.resize(_frameCounts);

    // start acquisition proccess in separate thread ...

    _acquisitionProccessThreadFuture = std::async(std::launch::async, [&]{
        try {

            double digits_temp_factor = pow(10.0,EAGLE_CAMERA_DEFAULT_TEMP_VALUE_DIGITS);

            // create FITS file

            int status = 0;
            long naxes[3] = {_imageXDim, _imageYDim, 0};
            long naxis = 2;

            double stopFrameExpTime = _expTime;

            bool exten_format = (!_fitsDataFormat.compare(EAGLE_CAMERA_FEATURE_FITS_DATA_FORMAT_EXTEN)) ? true : false;


            if ( !exten_format && (_frameCounts > 1) ) {
                naxis = 3;
                naxes[2] = _frameCounts;
            }

            exten_format = exten_format && (_frameCounts > 1);

            std::string filename = "!" + _fitsFilename; // add '!' to overwrite existing file

            formatFitsLogMessage("fits_create_file",filename,(void*)&status);

            CFITSIO_API_CALL( fits_create_file(&_fitsFilePtr, filename.c_str(), &status), logMessageStream.str() );

            std::string date_str = time_stamp(EAGLE_CAMERA_FITS_DATE_KEYWORD_FORMAT, true);

            if ( exten_format ) { // multi-extension FITS file
                // creating empty primary array
                formatFitsLogMessage("fits_create_img",USHORT_IMG,0,0,(void*)&status);
                CFITSIO_API_CALL( fits_create_img(_fitsFilePtr,USHORT_IMG,0,0,&status), logMessageStream.str());

                // write 'DATE' keyword into primary HDU

                formatFitsLogMessage("fits_update_key", TSTRING, "DATE", date_str,
                                     EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATE, (void*)&status);
                CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, "DATE", (void*)date_str.c_str(),
                                                  EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATE, &status),
                                  logMessageStream.str());

            } else {
                formatFitsLogMessage("fits_create_img",USHORT_IMG,naxis,(void*)naxes,(void*)&status);
                CFITSIO_API_CALL( fits_create_img(_fitsFilePtr,USHORT_IMG,naxis,naxes,&status),
                                  logMessageStream.str());

                // write 'DATE' keyword

                formatFitsLogMessage("fits_update_key", TSTRING, "DATE", date_str,
                                     EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATE, (void*)&status);
                CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, "DATE", (void*)date_str.c_str(),
                                                  EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATE, &status),
                                  logMessageStream.str());
            }

            ulong timeout = (_expTime + _capturingTimeoutGap)*1000; // to milliseconds

            IntegerType Nbuffers;
            if (_frameCounts < _frameBuffersNumber ) {
                Nbuffers = _frameCounts;
            } else {
                Nbuffers = _frameBuffersNumber;
            }


            std::future<void> run_saving;
            std::future<void> run_capture;

            _currentBuffer = 0;
            IntegerType lastSavingBuffer = 0;
            IntegerType i_frameSaving = 0;
            IntegerType i_frame;

            std::chrono::system_clock::time_point startSavingTimepoint;

            for ( i_frame = 0; i_frame < _frameCounts; ++i_frame ) {
                if ( run_capture.valid() ) { // wait for previous capturing&copying thread
                    auto wstatus = run_capture.wait_for(std::chrono::milliseconds(timeout));

                    if ( wstatus == std::future_status::timeout ) { // something is wrong!
                        throw EagleCameraException(0,EagleCamera::Error_AcquisitionProccessError,
                                                   "A timeout occured while waiting for copying of image from framebuffer");
                    } else if ( wstatus == std::future_status::ready ) {
                        run_capture.get();
                    } else { // something is wrong!
                        throw EagleCameraException(0,EagleCamera::Error_AcquisitionProccessError,
                                                   "Acquisition proccess failed");
                    }
                }

                // check for exposure abort signal
                if ( _stopCapturing ) { // recompute exposure duration
#ifndef NDEBUG
                    std::cout << "\n(stop before capturing) i_frame = " << i_frame << "\n";
#endif
                    std::chrono::duration<double> fp_s = _stopExpTimepoint-_startExpTimepoint;
                    stopFrameExpTime = fp_s.count();
                    break; // break cycle
                }

                if ( (i_frame - i_frameSaving) < _imageBuffer.size() ) { // read buffer should not overrun save buffer
                                                                         // more than a circle (number of buffers)
                    // 'arm' grabber, capture image and copy it to my buffer
                    run_capture = std::async(std::launch::async,
                                             &EagleCamera::doSnapAndCopy, this, timeout, i_frame, _currentBuffer);


                    // trigger single exposure
                    _startExpTimestamp[i_frame] = time_stamp(EAGLE_CAMERA_FITS_DATE_KEYWORD_FORMAT, true, &_startExpTimepoint);
                    setTriggerMode(CL_TRIGGER_MODE_SNAPSHOT);
#ifndef NDEBUG
                    std::cout << "\nSTART TRIGGER\n";
#endif

                    // read temperature values

                    double temp = (*this)[EAGLE_CAMERA_FEATURE_CCD_TEMP_NAME];
                    // rounding to required numbers of digits after floating point
                    temp = std::round(temp*digits_temp_factor)/digits_temp_factor;
                    _ccdTemp[i_frame] = temp;


                    temp = (*this)[EAGLE_CAMERA_FEATURE_PCB_TEMP_NAME];
                    // rounding to required numbers of digits after floating point
                    temp = std::round(temp*digits_temp_factor)/digits_temp_factor;
                    _pcbTemp[i_frame] = temp;

                    if ( i_frame == 0 ) run_capture.get(); // wait for the first image

                    ++_currentBuffer;
                    if ( _currentBuffer == _frameBuffersNumber ) _currentBuffer = 0;

                } else --i_frame;

                // save images to FITS file asynchronously

                if ( run_saving.valid() ) { // is saving thread still working?

                    // check for writing timeout
                    std::chrono::duration<double> diff = std::chrono::system_clock::now() - startSavingTimepoint;
                    if ( std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() >= _fitsWritingTimeout ) {
                        throw EagleCameraException(0,EagleCamera::Error_FitsWritingTimeout,
                                                   "A timeout occured while writing image buffer into FITS file");
                    }

                    auto wstatus = run_saving.wait_for(std::chrono::milliseconds(10));

                    // CFITSIO API does not allow parallel writing so just start next capturing
                    if ( wstatus == std::future_status::timeout ) {
                        continue; // capture next image
                    }
                    if ( wstatus != std::future_status::ready ) { // something wrong!
                        throw EagleCameraException(0,EagleCamera::Error_AcquisitionProccessError,
                                                   "Acquisition proccess failed");
                    }

                    // here wstatus == std::future_status::ready
                    run_saving.get();

                    ++lastSavingBuffer; // ready to save next image
                    if ( lastSavingBuffer == _frameBuffersNumber ) lastSavingBuffer = 0;
                    ++i_frameSaving;

                    if ( i_frameSaving >= i_frame ) continue; // capture next image
                }

                startSavingTimepoint = std::chrono::system_clock::now();
                run_saving = std::async(std::launch::async,
                                        &EagleCamera::saveToFitsFile, this, i_frameSaving,
                                        lastSavingBuffer, _expTime, exten_format);

            }

            if ( run_capture.valid() ) {
                run_capture.get(); // wait for the last capturing
            }

            if ( run_saving.valid() ) {
                run_saving.get();
                ++i_frameSaving;

                ++lastSavingBuffer; // ready to save next image
                if ( lastSavingBuffer == _frameBuffersNumber ) lastSavingBuffer = 0;
            }


#ifndef NDEBUG
            std::cout << "\nEND OF ACQUISITION LOOP: _currentBuffer = " << _currentBuffer <<
                         ", lastSavingBuffer = " << lastSavingBuffer << "\n" <<
                         "             i_frame = " << i_frame <<
                         ", i_frameSaving = " << i_frameSaving << "\n";
#endif

            if ( i_frameSaving < i_frame ) { // save remainder of buffers list
                if ( _currentBuffer > lastSavingBuffer ) {
                    for ( IntegerType i = lastSavingBuffer; i < _currentBuffer; ++i ) {
                        saveToFitsFile(i_frameSaving++, i, _expTime, exten_format);
                    }
                } else {
                    for ( IntegerType i = lastSavingBuffer; i < _frameBuffersNumber; ++i ) {
                        saveToFitsFile(i_frameSaving++, i, _expTime, exten_format);
                    }
                    for ( IntegerType i = 0; i < _currentBuffer; ++i ) {
                        saveToFitsFile(i_frameSaving++, i, _expTime, exten_format);
                    }
                }
            }

            if ( _stopCapturing && (stopFrameExpTime < _expTime)) { // re-write exposure duration keyword for the last image
                                                                    // if (stopFrameExpTime > _expTime) then
                                                                    // exposure was not active when it was stopped!
                if ( exten_format || i_frame == 1 ) {
                    formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME,
                                         _expTime, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_EXPTIME, (void*)&status);
                    CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME,
                                                      (void*)&stopFrameExpTime, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_EXPTIME,
                                                      &status),
                                      logMessageStream.str());
                }
            }

            if ( !exten_format && _stopCapturing ) { // re-write NAXIS3 value for "CUBE" data format
                long val;
                if ( i_frame > 1 ) { // just re-write
                    val = i_frame ;
                    formatFitsLogMessage("fits_update_key", TLONG ,"NAXIS3", val,NULL,(void*)&status);
                    CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TLONG ,"NAXIS3", &val, NULL, &status),
                                      logMessageStream.str() );
                } else { // delete keyword because of it is now just 2-dim image, and update "NAXIS" keyword
                    if ( _frameCounts > 1 ) {
                        val = 2;
                        formatFitsLogMessage("fits_update_key", TLONG ,"NAXIS", val,NULL,(void*)&status);
                        CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TLONG ,"NAXIS", &val, NULL, &status),
                                          logMessageStream.str() );
                        formatFitsLogMessage("fits_delete_key","NAXIS3",(void*)&status);
                        CFITSIO_API_CALL( fits_delete_key(_fitsFilePtr,"NAXIS3",&status),
                                          logMessageStream.str());
                    }
                }
            }


            // save fits keywords values in separate ASCII-table for "CUBE" data format
            if ( !exten_format && (i_frame > 1) ) {
                int tfields = 4; // number of columns
                const char* ttype[] = {"DATE-OBS", EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME,
                                      EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP, EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP};

                // format for exposure time
                std::string fmt1 = get_float_fmt(_expTime, EAGLE_CAMERA_DEFAULT_EXPTIME_VALUE_DIGITS);

                // format for temperature values
                auto res = std::max_element(_ccdTemp.begin(),_ccdTemp.end(),
                                            [](double &a, double &b){return std::abs(a) < std::abs(b);});
                std::string fmt2 = get_float_fmt(*res,EAGLE_CAMERA_DEFAULT_TEMP_VALUE_DIGITS);

                res = std::max_element(_pcbTemp.begin(),_pcbTemp.end(),
                                       [](double &a, double &b){return std::abs(a) < std::abs(b);});
                std::string fmt3 = get_float_fmt(*res,EAGLE_CAMERA_DEFAULT_TEMP_VALUE_DIGITS);

                const char* tform[] = {"A30",fmt1.c_str(),fmt2.c_str(),fmt3.c_str()};

                formatFitsLogMessage("fits_create_tbl",ASCII_TBL,0,tfields,(void*)ttype,(void*)tform,NULL,
                                     "CUBE INFO",(void*)&status);
                CFITSIO_API_CALL( fits_create_tbl(_fitsFilePtr,ASCII_TBL,i_frame,tfields,(char**)ttype,
                                                  (char**)tform,NULL,"CUBE INFO",&status),
                                  logMessageStream.str());

                const char* str;
                int icol;
                for ( IntegerType i = 0; i < i_frame; ++i ) {
                    str = _startExpTimestamp[i].c_str();
                    icol = 1;
                    formatFitsLogMessage("fits_write_col",TSTRING,icol,i+1,1,1,str,(void*)&status);
                    CFITSIO_API_CALL( fits_write_col(_fitsFilePtr,TSTRING,icol,i+1,1,1,&str,&status),
                                      logMessageStream.str());

                    ++icol;

                    formatFitsLogMessage("fits_write_col",TDOUBLE,icol,i+1,1,1,_expTime,(void*)&status);
                    CFITSIO_API_CALL( fits_write_col(_fitsFilePtr,TDOUBLE,icol,i+1,1,1,&_expTime,&status),
                                      logMessageStream.str());

                    ++icol;

                    formatFitsLogMessage("fits_write_col",TDOUBLE,icol,i+1,1,1,_ccdTemp[i],(void*)&status);
                    CFITSIO_API_CALL( fits_write_col(_fitsFilePtr,TDOUBLE,icol,i+1,1,1,&_ccdTemp[i],&status),
                                      logMessageStream.str());

                    ++icol;

                    formatFitsLogMessage("fits_write_col",TDOUBLE,icol,i+1,1,1,_pcbTemp[i],(void*)&status);
                    CFITSIO_API_CALL( fits_write_col(_fitsFilePtr,TDOUBLE,icol,i+1,1,1,&_pcbTemp[i],&status),
                                      logMessageStream.str());
                }

                if ( _stopCapturing  && (stopFrameExpTime < _expTime)) { // re-write value of EXPTIME if user abort exposure
                    formatFitsLogMessage("fits_write_col",TDOUBLE,2,i_frame,1,1,stopFrameExpTime,(void*)&status);
                    CFITSIO_API_CALL( fits_write_col(_fitsFilePtr,TDOUBLE,2,i_frame,1,1,&stopFrameExpTime,&status),
                                      logMessageStream.str());
                }
            }

#ifndef NDEBUG
            std::cout << "Save FITS keywords ...\n";
#endif

            // move to primary HDU (needs if multiple extensions format was used)
            formatFitsLogMessage("fits_movabs_hdu", 1, 0, (void*)&status);
            CFITSIO_API_CALL( fits_movabs_hdu(_fitsFilePtr, 1, NULL, &status), logMessageStream.str());

            // write camera info FITS keywords
            EagleCamera_StringFeature str_f;
            std::string str_val;
            int int_val;
            double float_val;
            long long_val;

            // origin
            str_val = std::string(EAGLE_CAMERA_SOFTWARE_NAME) + ", v" + std::to_string(EAGLE_CAMERA_VERSION_MAJOR) + "." +
                    std::to_string(EAGLE_CAMERA_VERSION_MINOR);
            formatFitsLogMessage("fits_update_key", TSTRING, "ORIGIN", str_val,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_ORIGIN, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, "ORIGIN", (void*)str_val.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_ORIGIN, &status),
                              logMessageStream.str() );

            // start pixels coordinates
            formatFitsLogMessage("fits_update_key", TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTX, _imageStartX,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTX, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTX, &_imageStartX,
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTX, &status),
                              logMessageStream.str() );

            formatFitsLogMessage("fits_update_key", TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTY, _imageStartY,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTY, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_STARTY, &_imageStartY,
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_STARTY, &status),
                              logMessageStream.str() );

            // binning
            int_val = (*this)[EAGLE_CAMERA_FEATURE_HBIN_NAME];
            formatFitsLogMessage("fits_update_key", TINT, EAGLE_CAMERA_FITS_KEYWORD_NAME_XBIN, int_val,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_XBIN, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TINT, EAGLE_CAMERA_FITS_KEYWORD_NAME_XBIN,
                                              &int_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_XBIN, &status),
                              logMessageStream.str() );

            str_val = std::to_string(int_val) + "x";
            int_val = (*this)[EAGLE_CAMERA_FEATURE_VBIN_NAME];
            formatFitsLogMessage("fits_update_key", TINT, EAGLE_CAMERA_FITS_KEYWORD_NAME_YBIN, int_val,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_YBIN, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TINT, EAGLE_CAMERA_FITS_KEYWORD_NAME_YBIN,
                                              &int_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_YBIN, &status),
                              logMessageStream.str() );


            str_val += std::to_string(int_val);
            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BINNING, str_val,
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BINNING, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BINNING,
                                              (void*)str_val.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BINNING, &status),
                              logMessageStream.str() );


            // shutter state
            str_f = (*this)[EAGLE_CAMERA_FEATURE_SHUTTER_STATE_NAME];
            str_val = std::string(EAGLE_CAMERA_FITS_KEYWORD_COMMENT_SHUTTER_STATE) + ": ";
            if ( !str_f.value().compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_EXP) ) {
                str_val += "open for duration exposure time";
            } else if ( !str_f.value().compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_CLOSED) ) {
                str_val += "always closed";
            } else if ( !str_f.value().compare(EAGLE_CAMERA_FEATURE_SHUTTER_STATE_OPEN) ) {
                str_val += "always open";
            }
            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_SHUTTER_STATE,
                                 str_f.value(), str_val, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_SHUTTER_STATE,
                                              (void*)str_f.value().c_str(), str_val.c_str(), &status),
                              logMessageStream.str() );

            // readout rate
            str_f = (*this)[EAGLE_CAMERA_FEATURE_READOUT_RATE_NAME];
            str_val = EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_RATE;
            if ( !str_f.value().compare(EAGLE_CAMERA_FEATURE_READOUT_RATE_FAST) ) {
                str_val += " (2 MHz)";
            } else if ( !str_f.value().compare(EAGLE_CAMERA_FEATURE_READOUT_RATE_SLOW) ){
                str_val += " (75 kHz)";
            }
            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_RATE,
                                 str_f.value(), str_val, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_RATE,
                                              (void*)str_f.value().c_str(), str_val.c_str(), &status),
                              logMessageStream.str() );


            // readout mode
            str_f = (*this)[EAGLE_CAMERA_FEATURE_READOUT_MODE_NAME];
            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_MODE,
                                 str_f.value(), EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_MODE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_READOUT_MODE,
                                              (void*)str_f.value().c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_MODE, &status),
                              logMessageStream.str() );


            // TEC state and temperatures
            str_f = (*this)[EAGLE_CAMERA_FEATURE_TEC_STATE_NAME];
            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_TEC_STATE,
                                 str_f.value(), EAGLE_CAMERA_FITS_KEYWORD_COMMENT_TEC_STATE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_TEC_STATE,
                                              (void*)str_f.value().c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_TEC_STATE, &status),
                              logMessageStream.str() );

            float_val = (*this)[EAGLE_CAMERA_FEATURE_CCD_TEMP_NAME];
            float_val = std::round(float_val*100)/100.0; // 2 digits after the floating point
            formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP,
                                 float_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_CCD_TEMP, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP,
                                              &float_val,
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_CCD_TEMP, &status),
                              logMessageStream.str() );

            float_val = (*this)[EAGLE_CAMERA_FEATURE_PCB_TEMP_NAME];
            float_val = std::round(float_val*100)/100.0; // 2 digits after the floating point
            formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP,
                                 float_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_MODE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP,
                                              &float_val,
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_PCB_TEMP, &status),
                              logMessageStream.str() );



            // write user FITS keywords
            if ( !_fitsHdrFilename.empty() ) {
                formatFitsLogMessage("fits_write_key_template", _fitsHdrFilename, (void*)&status);
                CFITSIO_API_CALL( fits_write_key_template(_fitsFilePtr, _fitsHdrFilename.c_str(),&status),
                                  logMessageStream.str());
            }

            // versions info keywords
            long_val = (long)_serialNumber;
            formatFitsLogMessage("fits_update_key", TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_SERIAL_NUMBER,
                                 long_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_SERIAL_NUMBER, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TLONG, EAGLE_CAMERA_FITS_KEYWORD_NAME_SERIAL_NUMBER,
                                              &long_val, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_SERIAL_NUMBER, &status),
                              logMessageStream.str());


            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_MICRO_VERSION,
                                 _microVersion, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_MICRO_VERSION, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_MICRO_VERSION,
                                              (void*)_microVersion.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_MICRO_VERSION, &status),
                              logMessageStream.str() );


            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_FPGA_VERSION,
                                 _FPGAVersion, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_FPGA_VERSION, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_FPGA_VERSION,
                                              (void*)_FPGAVersion.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_FPGA_VERSION, &status),
                              logMessageStream.str() );

            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_DATE,
                                 _buildDate, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_DATE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_DATE,
                                              (void*)_buildDate.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_DATE, &status),
                              logMessageStream.str() );

            formatFitsLogMessage("fits_update_key", TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_CODE,
                                 _buildCode, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_CODE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, EAGLE_CAMERA_FITS_KEYWORD_NAME_BUILD_CODE,
                                              (void*)_buildCode.c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_BUILD_CODE, &status),
                              logMessageStream.str() );


            formatFitsLogMessage("fits_close_file",(void*)&status);
            CFITSIO_API_CALL( fits_close_file(_fitsFilePtr,&status), logMessageStream.str());

#ifndef NDEBUG
            std::cout << "  OK (FITS keywords)\n";
#endif
        } catch ( EagleCameraException ex ) {
#ifndef NDEBUG
            std::cout << "ACQ PROCCESS ERROR: " << ex.XCLIB_Error() << ", " << ex.Camera_Error() << "\n";
            std::cout << "ACQ PROCCESS ERROR: " << ex.what() << "\n";
#endif
            _acquiringFinished = true;
            throw ex;
        }

#ifndef NDEBUG
        std::cout << "END OF ACQUSITION\n";
#endif
        _acquiringFinished = true;
    });


    // waiting for 100 millisecond and check for possible errors ...

    auto wstatus = _acquisitionProccessThreadFuture.wait_for(std::chrono::milliseconds(100));
    if ( wstatus != std::future_status::timeout ) {
        _acquisitionProccessThreadFuture.get(); // if an error occured an exception should be thrown here
    }

}


void EagleCamera::stopAcquisition()
{
#ifndef NDEBUG
    std::cout << "\nABORT EXPOSURE!!!\n";
#endif
    _stopExpTimepoint = std::chrono::system_clock::now();
    setTriggerMode(CL_TRIGGER_MODE_ABORT_CURRENT_EXP); // set abort exp bit
    _stopCapturing = true;
}


void EagleCamera::imageReady(const IntegerType frame_no, const ushort *image_buffer, const size_t buffer_len)
{
}


EagleCamera::CameraFeatureProxy & EagleCamera::operator [](const std::string & name)
{
    if ( cameraUnitmap <= 0 ) {
        throw EagleCameraException(0,EagleCamera::Error_Uninitialized,"Try to access feature for uninitialized camera!");
    }

    auto search = PREDEFINED_CAMERA_FEATURES.find(name);

    if ( search != PREDEFINED_CAMERA_FEATURES.end() ) {
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
    if ( ex.XCLIB_Error() < 0 ) { // XCLIB errors
        std::string log_str = ex.what();
        log_str += " [XCLIB ERROR CODE: " + std::to_string(ex.XCLIB_Error()) + "]";

        logToFile(EagleCamera::LOG_IDENT_XCLIB_ERROR, log_str, indent_tabs);
    }

    if ( ex.Camera_Error() != EagleCamera::Error_OK ) {
        std::string log_str = ex.what();
        if ( ex.Camera_Error() == EagleCamera::Error_FITS_ERR ) {
            log_str += " [CFITSIO ERROR CODE: " + std::to_string(ex.XCLIB_Error()) + "]";
        } else {
            log_str += " [CAMERA ERROR CODE: " + std::to_string(ex.Camera_Error()) + "]";
        }

        logToFile(EagleCamera::LOG_IDENT_CAMERA_ERROR, log_str, indent_tabs);
    }
}



                            /*  PROTECTED METHODS  */

void EagleCamera::doSnapAndCopy(const ulong timeout, const IntegerType frame_no, const IntegerType buff_no)
{
    char col[] = "Gray";

    try {
#ifndef NDEBUG
        std::cout << "\nCAPTURE (frame_no = " << frame_no << ", buff_no = " << buff_no << ") ";
#endif

        formatLogMessage("pxd_doSnap", 1, timeout);
        XCLIB_API_CALL( pxd_doSnap(cameraUnitmap, 1, timeout), logMessageStream.str());


        formatLogMessage("pxd_readushort",1, 0, 0, -1, _frameBufferLines,
                         (void*)_imageBuffer[buff_no].get(),_currentBufferLength,col);

#ifndef NDEBUG
        std::cout << "AND READ IMAGE TO BUFFER ...";
#endif

        XCLIB_API_CALL(pxd_readushort(cameraUnitmap, 1, 0, 0, -1, _frameBufferLines, _imageBuffer[buff_no].get(),
                                      _currentBufferLength, (char*)col),
                logMessageStream.str());

        imageReady(frame_no,_imageBuffer[buff_no].get(), _imagePixelsNumber);

#ifndef NDEBUG
        std::cout << "OK CAPTURE & READ\n";
#endif
    } catch ( EagleCameraException &ex ) {
        throw;
    }
}


void EagleCamera::saveToFitsFile(const IntegerType frame_no, const IntegerType buff_no,
                                 const double exp_time, bool as_extension)
{
    int status = 0;

    try {
#ifndef NDEBUG
        std::cout << "\nSave FITS (i_frameSaving = " << frame_no <<
                     ", lastBufferSaving = " << buff_no << ")  ...";
#endif

        if ( as_extension ) {
            long naxes[2] = {_imageXDim, _imageYDim};

            formatFitsLogMessage("fits_create_img",USHORT_IMG,2,(void*)naxes,&status);
            CFITSIO_API_CALL( fits_create_img(_fitsFilePtr,USHORT_IMG,2,naxes,&status),
                              logMessageStream.str());

            // write 'DATE-OBS'

            formatFitsLogMessage("fits_update_key", TSTRING, "DATE-OBS", _startExpTimestamp[frame_no],
                                 EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATEOBS, (void*)&status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, "DATE-OBS",
                                              (void*)_startExpTimestamp[frame_no].c_str(),
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATEOBS, &status),
                              logMessageStream.str());

            // write temperatures keywords

            formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP,
                                 _ccdTemp[frame_no], EAGLE_CAMERA_FITS_KEYWORD_COMMENT_CCD_TEMP, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_CCD_TEMP,
                                              &_ccdTemp[frame_no],
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_CCD_TEMP, &status),
                              logMessageStream.str() );

            formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP,
                                 _pcbTemp[frame_no], EAGLE_CAMERA_FITS_KEYWORD_COMMENT_READOUT_MODE, &status);
            CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_PCB_TEMP,
                                              &_pcbTemp[frame_no],
                                              EAGLE_CAMERA_FITS_KEYWORD_COMMENT_PCB_TEMP, &status),
                              logMessageStream.str() );

            // write image
            formatFitsLogMessage("fits_write_img", TUSHORT, 1, _imagePixelsNumber,
                                 (void*)_imageBuffer[buff_no].get(), (void*)&status);
            CFITSIO_API_CALL( fits_write_img(_fitsFilePtr, TUSHORT, 1, _imagePixelsNumber,
                                             (void*)_imageBuffer[buff_no].get(), &status),
                              logMessageStream.str() );
        } else {
            if ( frame_no == 0 ) {
                // write 'DATE-OBS'
                formatFitsLogMessage("fits_update_key", TSTRING, "DATE-OBS", _startExpTimestamp[0],
                                     EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATEOBS, (void*)&status);
                CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TSTRING, "DATE-OBS",
                                                  (void*)_startExpTimestamp[0].c_str(),
                                                  EAGLE_CAMERA_FITS_KEYWORD_COMMENT_DATEOBS, &status),
                                  logMessageStream.str());
            }
            long first_pix = frame_no*_imagePixelsNumber + 1;
            formatFitsLogMessage("fits_write_img", TUSHORT, first_pix, _imagePixelsNumber,
                                 (void*)_imageBuffer[buff_no].get(), (void*)&status);
            CFITSIO_API_CALL( fits_write_img(_fitsFilePtr, TUSHORT, first_pix, _imagePixelsNumber,
                                             (void*)_imageBuffer[buff_no].get(), &status),
                              logMessageStream.str() );
        }
        // write exposure duration keyword

        formatFitsLogMessage("fits_update_key", TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME,
                             _expTime, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_EXPTIME, (void*)&status);
        CFITSIO_API_CALL( fits_update_key(_fitsFilePtr, TDOUBLE, EAGLE_CAMERA_FITS_KEYWORD_NAME_EXPTIME,
                                          (void*)&exp_time, EAGLE_CAMERA_FITS_KEYWORD_COMMENT_EXPTIME, &status),
                          logMessageStream.str());

#ifndef NDEBUG
        std::cout << "  OK (Save FITS)\n";
#endif
    } catch ( EagleCameraException &ex ) {
        throw;
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
    XCLIB_API_CALL( nbytes = pxd_serialRead(cameraUnitmap,0,NULL,0), logMessageStream.str() );

    // special case
    if ( (data.size() == 0) && !info_len ) { // nothing to read
        return nbytes;
    }

    int N = nbytes;

    if ( !all ) {
        nbytes = data.size() + info_len;
    } else { // just read all available bytes in Rx-buffer
        if ( !nbytes ) return nbytes; // no data? do not wait and return immediately!
    }

    buff = std::unique_ptr<char[]>(new char[nbytes]);
    buff_ptr = buff.get();

    if ( all ) {
        formatLogMessage("pxd_serialRead", 0, (void*)buff_ptr, nbytes);
        XCLIB_API_CALL( pxd_serialRead(cameraUnitmap, 0, buff_ptr, nbytes), logMessageStream.str(),
                        buff_ptr, nbytes);
    } else {
        std::chrono::milliseconds timeout{10000};
        int64_t timeout_count = timeout.count();

        auto start = std::chrono::system_clock::now();

        while ( N < nbytes ) {
            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = now-start;
            if ( std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() >= timeout_count ) {
                throw EagleCameraException(PXERTIMEOUT,EagleCamera::Error_OK, logMessageStream.str());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // how many byte available for reading ...
            XCLIB_API_CALL( N = pxd_serialRead(cameraUnitmap,0,NULL,0), logMessageStream.str() );
        }

        formatLogMessage("pxd_serialRead", 0, (void*)buff_ptr, nbytes);
        XCLIB_API_CALL( pxd_serialRead(cameraUnitmap, 0, buff_ptr, nbytes), logMessageStream.str(),
                        buff_ptr, nbytes);
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
    int nbytes = 0;

    formatLogMessage("pxd_serialWrite",0,NULL,0);
    XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, NULL, 0), logMessageStream.str() );

    if ( val.size() == 0 ) { // special case
        return nbytes;
    } else {
//        int N;

        int UART_len = val.size() + 1; // '+1' for mandatory ETX-byte
        if ( CL_CHK_SUM_BIT_ENABLED ) ++UART_len;

        // make UART
        std::unique_ptr<unsigned char[]> buff = std::unique_ptr<unsigned char[]>(new unsigned char[UART_len]);
        unsigned char* buff_ptr = buff.get();
        memcpy(buff_ptr, (void*)val.data(), val.size());
        buff[val.size()] = CL_ETX; // mandatory ETX byte

        if ( CL_CHK_SUM_BIT_ENABLED ) { // compute check sum
            buff[val.size()+1] = val[0];
            for ( int i = 1; i < val.size(); ++i ) buff[val.size()+1] ^= val[i];
            buff[val.size()+1] ^= CL_ETX; // ETX-byte is also included in check sum computation
        }

        std::chrono::milliseconds timeout{10000};
        int64_t timeout_count = timeout.count();

        auto start = std::chrono::system_clock::now();

        while ( nbytes < UART_len ) {
            auto now = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = now-start;
            if ( std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() >= timeout_count ) {
                throw EagleCameraException(PXERTIMEOUT,EagleCamera::Error_OK, logMessageStream.str());
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

//            formatLogMessage("pxd_serialWrite",0,NULL,0);
            XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, NULL, 0), logMessageStream.str() );
        }

        formatLogMessage("pxd_serialWrite",0,(void*)buff_ptr,UART_len);
        XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, (char*)buff_ptr, UART_len),
                        logMessageStream.str(), (char*)buff.get(), UART_len);

        /*
        formatLogMessage("pxd_serialWrite",0,(void*)val.data(),val.size());
        XCLIB_API_CALL( nbytes = pxd_serialWrite(cameraUnitmap, 0, (char*)val.data(), val.size()),
                        logMessageStream.str(), (char*)val.data(), val.size());

        // write mandatory End-of-Transmision byte
        char ack = CL_ETX;

        formatLogMessage("pxd_serialWrite",0,(void*)&ack,1);
        XCLIB_API_CALL( N = pxd_serialWrite(cameraUnitmap, 0, &ack, 1), logMessageStream.str(),
                        &ack, 1);

        nbytes += N;

        if ( CL_CHK_SUM_BIT_ENABLED ) { // compute check sum
            char sum = val[0];
            for ( int i = 1; i < val.size(); ++i ) sum ^= val[i];
            sum ^= CL_ETX;

            formatLogMessage("pxd_serialWrite",0,(void*)&sum,1);
            XCLIB_API_CALL( N = pxd_serialWrite(cameraUnitmap, 0, &sum, 1),
                            logMessageStream.str(), &sum, 1 );
            nbytes += N;
        }
        */
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

    int64_t timeout_counts = 0;
    if ( timeout > 0 ) {
        std::chrono::milliseconds readTimeout{timeout};
        timeout_counts = readTimeout.count();
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

        if ( timeout_counts > 0 ) {
            auto now = std::chrono::system_clock::now();
            if ( std::chrono::duration_cast<std::chrono::milliseconds>(start-now).count() >= timeout_counts ) return false;
        }
    }

    return true;
}


bool EagleCamera::resetFPGA(const long timeout)
{
    int64_t timeout_counts = 0;
    if ( timeout > 0 ) {
        std::chrono::milliseconds readTimeout{timeout};
        timeout_counts = readTimeout.count();
    }

    byte_vector_t comm = {0x4F, 0x50};
    CL_ACK_BIT_ENABLED = true;
    CL_CHK_SUM_BIT_ENABLED = true;

    byte_vector_t poll_comm = {0x49};
    byte_vector_t ack(1);

    cl_exec(comm);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    comm = {0x4F, 0x52};
    cl_exec(comm);

    auto start = std::chrono::system_clock::now();

    for (;;) {
        cl_exec(poll_comm,ack);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if ( is_fpga_boot_ok(ack[0]) ) {
            return true;
        }

        if ( timeout_counts > 0 ) {
            auto now = std::chrono::system_clock::now();
            if ( std::chrono::duration_cast<std::chrono::milliseconds>(start-now).count() >= timeout_counts ) {
                return false;
            }
        }
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


EagleCamera::IntegerType EagleCamera::fpga12BitsToInteger(const byte_vector_t &vals)
{
    IntegerType v = 0;

    v = ((vals[0] & 0x0F) << 8) + vals[1];

//    std::cout << "FPGA12: vals[0] = " << (int)vals[0] << ", vals[1] = " << (int)vals[1] << "\n";
//    std::cout << "FPGA12: v = " << v << "\n";
    return v;
}


EagleCamera::byte_vector_t EagleCamera::integerToFPGA12Bits(const EagleCamera::IntegerType val)
{
    byte_vector_t value(2);

    value[0] = (val & 0x0F00) >> 8; // MM
    value[1] = val & 0xFF;          // LL

//    std::cout << "FPGA12: vals[0] = " << (int)value[0] << ", vals[1] = " << (int)value[1] << "\n";
//    std::cout << "FPGA12: val = " << val << "\n";
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


                        /*  LOGGING METHODS  */

int EagleCamera::XCLIB_API_CALL(int err_code, const char *context)
{
    return XCLIB_API_CALL(err_code, std::string(context),nullptr,0);
}


int EagleCamera::XCLIB_API_CALL(int err_code, const std::string &context, const char *res, const int res_len)
{
    if ( err_code < 0 ) {
        throw EagleCameraException(err_code, EagleCamera::Error_OK, context);
    }

    if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
        if ( (res == nullptr) || (res_len == 0) ) {
            logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(context, err_code));
        } else {
            logToFile(EagleCamera::LOG_IDENT_XCLIB_INFO, logXCLIB_Info(context, res, res_len));
        }
    }

    return err_code;

}


int EagleCamera::CFITSIO_API_CALL(int err_code, const char *context)
{
    return CFITSIO_API_CALL(err_code, std::string(context));
}


int EagleCamera::CFITSIO_API_CALL(int err_code, const std::string &context)
{
    if ( err_code ) {
        throw EagleCameraException(err_code, EagleCamera::Error_FITS_ERR, context);
    }

    if ( logLevel == EagleCamera::LOG_LEVEL_VERBOSE ) {
        logToFile(EagleCamera::LOG_IDENT_CAMERA_INFO, context);
    }

    return err_code;
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


void EagleCamera::logHelper()
{

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

    ss << " -> [" << std::hex << ((uint16_t)res[0] & 0x00FF) << std::dec;
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
