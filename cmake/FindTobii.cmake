# Distributed under the OSI-approved BSD 3-Clause License.  See 
# https://cmake.org/licensing for details.

#.rst:
# FindTobii
# --------
#
# Find the Tobii "Stream Engine" SDK.
#
# This module accepts the following variable for finding the SDK (in addition to ``CMAKE_PREFIX_PATH``)
#
# ::
#
#     TOBII_ROOT - An additional path to search. Set to the root of the extracted SDK zip.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the :prop_tgt:`IMPORTED` target ``Tobii::Tobii``,
# with both `.lib` and `.dll` files associated, if the Tobii Stream
# Engine SDK has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   TOBII_INCLUDE_DIRS - include directories for Tobii Stream SDK
#   TOBII_RUNTIME_LIBRARIES - Runtime libraries (.dll file) needed for execution of an app linked against this SDK, if it differs from TOBII_LIBRARIES.
#   TOBII_RUNTIME_LIBRARY_DIRS - directories to add to runtime library search path.
#   TOBII_LIBRARIES - libraries to link against Tobii Stream SDK
#   TOBII_FOUND - true if Tobii Stream SDK has been found and can be used

# Original Author:
# 2018 Ryan Pavlik for Sensics, Inc.
#
# Copyright Sensics, Inc. 2018.


set(TOBII_ROOT
    "${TOBII_ROOT}"
    CACHE
    PATH
    "Path to search for Tobii Stream SDK: set to the root of the extracted SDK.")

set(_tobii_extra_paths)
set(_tobii_hint)
if(TOBII_ROOT)
    list(APPENT _tobii_extra_paths "${TOBII_ROOT}")
    # In case they were one directory too high
    file(GLOB _tobii_more "${TOBII_ROOT}/*_stream_engine_*")
    if(_tobii_more)
        list(APPEND _tobii_extra_paths ${_tobii_more})
    endif()
endif()


if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_TOBII_LIB_PATH_SUFFIX lib/x64)
else()
    set(_TOBII_LIB_PATH_SUFFIX lib/x86)
endif()

find_path(TOBII_INCLUDE_DIR
    tobii/tobii.h
    PATHS
    ${_tobii_extra_paths})

if(TOBII_INCLUDE_DIR)
    # parent of include dir is useful root
    get_filename_component(_tobii_hint "${TOBII_INCLUDE_DIR}" DIRECTORY)
endif()

find_library(TOBII_STREAM_ENGINE_LIBRARY
    tobii_stream_engine
    PATH_SUFFIXES ${_TOBII_LIB_PATH_SUFFIX}
    HINTS ${_tobii_hint}
    PATHS ${_tobii_extra_paths})

set(_tobii_extra_required)
if(WIN32)
    list(APPEND _tobii_extra_required TOBII_STREAM_ENGINE_RUNTIME_LIBRARY)
endif()
if(WIN32 AND TOBII_STREAM_ENGINE_LIBRARY)
    # directory of link import library is also location for DLL, typically
    get_filename_component(_tobii_libdir "${TOBII_STREAM_ENGINE_LIBRARY}" DIRECTORY)
    find_file(TOBII_STREAM_ENGINE_RUNTIME_LIBRARY
        tobii_stream_engine.dll
        HINTS ${_tobii_libdir} ${_tobii_hint}
        PATH_SUFFIXES ${_TOBII_LIB_PATH_SUFFIX}
        PATHS ${_tobii_extra_paths})
endif()

mark_as_advanced(TOBII_INCLUDE_DIR TOBII_STREAM_ENGINE_LIBRARY TOBII_STREAM_ENGINE_RUNTIME_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tobii
                                  REQUIRED_VARS TOBII_INCLUDE_DIR TOBII_STREAM_ENGINE_LIBRARY ${_tobii_extra_required})
if(TOBII_FOUND)
    set(TOBII_INCLUDE_DIRS "${TOBII_INCLUDE_DIR}")
    set(TOBII_LIBRARIES "${TOBII_STREAM_ENGINE_LIBRARY}")
    if(WIN32)
        set(TOBII_RUNTIME_LIBRARIES "${TOBII_STREAM_ENGINE_RUNTIME_LIBRARY}")
        get_filename_component(TOBII_RUNTIME_LIBRARY_DIRS "${TOBII_STREAM_ENGINE_RUNTIME_LIBRARY}" DIRECTORY)
    else()
        get_filename_component(TOBII_RUNTIME_LIBRARY_DIRS "${TOBII_STREAM_ENGINE_LIBRARY}" DIRECTORY)
    endif()
    if(NOT TARGET Tobii::Tobii)
        add_library(Tobii::Tobii SHARED IMPORTED)
        set_target_properties(Tobii::Tobii PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${TOBII_INCLUDE_DIRS}")
        if(WIN32)
            set_target_properties(Tobii::Tobii PROPERTIES
                IMPORTED_IMPLIB "${TOBII_STREAM_ENGINE_LIBRARY}"
                IMPORTED_LOCATION "${TOBII_STREAM_ENGINE_RUNTIME_LIBRARY}")
        else()
            set_target_properties(Tobii::Tobii PROPERTIES
                IMPORTED_LOCATION "${TOBII_STREAM_ENGINE_LIBRARY}")
        endif()
    endif()
    mark_as_advanced(TOBII_ROOT)
endif()