cmake_minimum_required(VERSION 2.8)

project(RAPTOR_EAGLE_CAM)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

find_package(XCLIB QUIET)

if (XCLIB_FOUND)
    message(STATUS "XCLIB IS FOUND!")
    message(STATUS "XCLIB_INCLUDE_DIR: " ${XCLIB_INCLUDE_DIR})
    message(STATUS "XCLIB_LIBRARIES: " ${XCLIB_LIBRARIES})
endif()


find_package(CFITSIO QUIET)

if ( CFITSIO_FOUND )
    message(STATUS "CFITSIO IS FOUND!")
    message(STATUS "CFITSIO_INCLUDE_DIR: " ${CFITSIO_INCLUDE_DIR})
    message(STATUS "CFITSIO_LIBRARIES: " ${CFITSIO_LIBRARIES})
endif()


set (EAGLE_CAMERA_SOFTWARE_NAME "EagleCam control software")
set (EAGLE_CAMERA_VERSION_MAJOR 0)
set (EAGLE_CAMERA_VERSION_MINOR 1)

configure_file (
  "camera/eagle_camera_config.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/camera/eagle_camera_config.h"
  #  "${PROJECT_BINARY_DIR}/eagle_camera_config.h"
)

# API library
set(EAGLE_CAMERA_LIB eagle_camera)
include_directories(./camera ${XCLIB_INCLUDE_DIR})
include_directories(./camera ${CFITSIO_INCLUDE_DIR})
aux_source_directory(./camera/ EAGLE_CAMERA_SRC)
add_library(${EAGLE_CAMERA_LIB} SHARED ${EAGLE_CAMERA_SRC})
target_link_libraries(${EAGLE_CAMERA_LIB} ${XCLIB_LIBRARIES})
target_link_libraries(${EAGLE_CAMERA_LIB} ${CFITSIO_LIBRARIES})
target_compile_definitions(${EAGLE_CAMERA_LIB} PRIVATE EAGLE_CAMERA_LIBRARY=1)


# commandline client
set(EAGLE_CAMERA_CMD eaglecam_cmd)
add_executable(${EAGLE_CAMERA_CMD} eaglecam_cmd.cpp)
target_link_libraries(${EAGLE_CAMERA_CMD} ${EAGLE_CAMERA_LIB})


# temporary test suite
set(TEST_PROG test_prog)
add_executable(${TEST_PROG} test_prog.cpp)
target_link_libraries(${TEST_PROG} ${EAGLE_CAMERA_LIB})
