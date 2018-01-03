/** @file
    @brief Header

    @date 2018

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2018 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDED_HardwareDetection_h_GUID_338514CB_A4A5_4284_A427_12C72A9B7F3E
#define INCLUDED_HardwareDetection_h_GUID_338514CB_A4A5_4284_A427_12C72A9B7F3E


// Internal Includes
#include "TrackerDevice.h"
#include "TobiiLoggerNames.h"

// Library/third-party includes
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/Util/Log.h>

// Standard includes
// - none



namespace TobiiOSVR {
    static const char* kTobiiDriverName = "Tobii";

    class HardwareDetection {
    public:
        HardwareDetection();
        ~HardwareDetection();

        OSVR_ReturnCode operator()(OSVR_PluginRegContext pContext, const char *params);
        OSVR_ReturnCode HardwareDetection::operator()(OSVR_PluginRegContext pContext);

    private:
        osvr::util::log::LoggerPtr mLog;

        bool mFound = false;
        bool mDeviceAdded = false;
        TrackerDevice *mTrackerDevice = nullptr;
    };
}

#endif // INCLUDED_HardwareDetection_h_GUID_338514CB_A4A5_4284_A427_12C72A9B7F3E
