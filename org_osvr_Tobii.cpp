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

// Library/third-party includes
#include <osvr/PluginKit/PluginKit.h>

// Standard includes
// - none

OSVR_PLUGIN(com_osvr_Tobii) {
    osvr::pluginkit::PluginContext context(ctx);
    auto hd = new TobiiOSVR::HardwareDetection();

    context.registerDriverInstantiationCallback(TobiiOSVR::kTobiiDriverName, hd);
    context.registerHardwareDetectCallback(hd);

    return OSVR_RETURN_FAILURE;
}
