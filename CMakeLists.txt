cmake_minimum_required(VERSION 2.8.12)
project(TobiiPlugin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(vendor)

if(NOT EIGEN3_FOUND)
    list(APPEND CMAKE_PREFIX_PATH "${TOBII_VENDORED_EIGEN_ROOT}")
endif()

# This looks for an osvrConfig.cmake file - most of the time it can be
# autodetected but you might need to create/extend CMAKE_PREFIX_PATH to include something like
# C:/Users/Ryan/Desktop/build/OSVR-Core-vc12 or
# C:/Users/Ryan/Downloads/OSVR-Core-Snapshot-v0.1-406-gaa55515-build54-vs12-32bit
# in the CMake GUI or command line.
find_package(osvr REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Tobii REQUIRED)

# This generates a header file, from the named json file, containing a string literal
# named org_osvr_Tobii_json (not null terminated)
# The file must be added as a source file to some target (as below) to be generated.
osvr_convert_json(org_osvr_Tobii_json
    org_osvr_Tobii.json
    "${CMAKE_CURRENT_BINARY_DIR}/org_osvr_Tobii_json.h")

# Be able to find our generated header file.
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${EIGEN3_INCLUDE_DIR}")
# include_directories("${TOBII_INCLUDE_DIRS}")
include_directories(org_osvr_Tobii)

# This is just a helper function wrapping CMake's add_library command that
# sets up include dirs, libraries, and naming convention (no leading "lib")
# for an OSVR plugin. It also installs the plugin into the right directory.
# Pass as many source files as you need. See osvrAddPlugin.cmake for full docs.
osvr_add_plugin(NAME org_osvr_Tobii
    CPP # indicates we'd like to use the C++ wrapper
    SOURCES
    Win32Includes.h
    TrackerDevice.h
    TrackerDevice.cpp
    HardwareDetection.cpp
    HardwareDetection.h
    org_osvr_Tobii.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/org_osvr_Tobii_json.h")

# If you use other libraries, find them and add a line like:
target_link_libraries(org_osvr_Tobii
    osvr::osvrAnalysisPluginKit
    Tobii::Tobii
	eigen-headers)
#    JsonCpp::JsonCpp)
