/** @file
    @brief Implementation

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

// Internal Includes
#include "HardwareDetection.h"
#include "TrackerDevice.h"

// Library/third-party includes
#include <osvr/Util/Logger.h>

// Standard includes
#include <iostream>

using namespace TobiiOSVR;

HardwareDetection::HardwareDetection() {
    mLog = osvr::util::log::make_logger(EYE_TRACKER_LOG);
}

HardwareDetection::~HardwareDetection() {
    if(mTrackerDevice) {
        delete mTrackerDevice;
    }
}

OSVR_ReturnCode HardwareDetection::operator()(OSVR_PluginRegContext pContext) {
    return (*this)(pContext, nullptr);
}

OSVR_ReturnCode HardwareDetection::operator()(OSVR_PluginRegContext pContext, const char *params) {
    if(!mFound) {
        // connect to device
        if(!mTrackerDevice) {
            mTrackerDevice = new TrackerDevice(pContext);
        }
        mFound = mTrackerDevice->tryInit();
        if(mFound) {
            // transfer ownership of mTrackerDevice to pContext
            osvr::pluginkit::registerObjectForDeletion(pContext, mTrackerDevice);
            mTrackerDevice = nullptr;
        } else {
            mLog->error() << "Device not detected.";
            return OSVR_RETURN_FAILURE;
        }
    }
    return OSVR_RETURN_SUCCESS;
}
