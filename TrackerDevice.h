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

#ifndef INCLUDED_TrackerDevice_h_GUID_BF698D0B_A4DF_43BA_D40B_6FDB8B60108A
#define INCLUDED_TrackerDevice_h_GUID_BF698D0B_A4DF_43BA_D40B_6FDB8B60108A


// Internal Includes
#include "TobiiLoggerNames.h"

// Library/third-party includes
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/EyeTrackerInterfaceC.h>
#include <osvr/AnalysisPluginKit/AnalysisPluginKitC.h>
#include <osvr/ClientKit/InterfaceC.h>
#include <osvr/Util/Vec2C.h>
#include <osvr/Util/Vec3C.h>
#include <osvr/Util/Log.h>

// Standard includes
#include <memory>
#include <thread>
#include <mutex>
#include <string>

namespace TobiiOSVR {
	
	typedef struct {
		OSVR_EyeGazePosition2DState gazePosition;
		OSVR_EyeGazeDirectionState gazeDirection;
		OSVR_EyeGazeBasePoint3DState gazeBasePoint;
	} GazeState;

    class EyeTrackerBase {
        protected:
            
            osvr::util::log::LoggerPtr mLog;

            bool mLastIsBlinking = false;
            bool mInitialized = false;
        public:
			EyeTrackerBase() {
                mLog = osvr::util::log::make_logger(EYE_TRACKER_LOG);
			}

            virtual ~EyeTrackerBase() {}

            virtual bool init() {
                return mInitialized;
            }

            virtual bool waitForData() {
                return true;
            };

            virtual void getLeftEyeGazeState(GazeState &gazeState) {
                osvrVec2Zero(&gazeState.gazePosition);
                osvrVec3Zero(&gazeState.gazeDirection);
                osvrVec3Zero(&gazeState.gazeBasePoint);
            }

            virtual void getRightEyeGazeState(GazeState &gazeState) {
                osvrVec2Zero(&gazeState.gazePosition);
                osvrVec3Zero(&gazeState.gazeDirection);
                osvrVec3Zero(&gazeState.gazeBasePoint);
            }

            virtual bool getIsBlinking() {
                mLastIsBlinking = !mLastIsBlinking;
                return mLastIsBlinking;
            }
    };

    class TrackerDevice {
    public:
        TrackerDevice(OSVR_PluginRegContext pContext);
        ~TrackerDevice();
        OSVR_ReturnCode operator()(OSVR_PluginRegContext pContext);
        OSVR_ReturnCode update();

        bool tryInit();
    private:

        enum EyeTrackerChannel {
            LeftEyeTrackerChannel,
            RightEyeTrackerChannel,

            NumEyeTrackerChannels
        };

		enum BlinkChannel {
			BlinkChannel,

			NumBlinkChannels
		};

		osvr::util::log::LoggerPtr mLog;

        bool mInitialized = false;
        
		OSVR_EyeTrackerDeviceInterface mEyeTrackerInterface;
		osvr::pluginkit::DeviceToken mDeviceToken;

		std::shared_ptr<EyeTrackerBase> mEyeTracker;
		bool mLastIsBlinking = false;
    };
}

#endif // INCLUDED_TrackerDevice_h_GUID_BF698D0B_A4DF_43BA_D40B_6FDB8B60108A
