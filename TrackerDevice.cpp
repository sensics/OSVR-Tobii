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
#include "TrackerDevice.h"
#include "org_osvr_Tobii_json.h"

// Library/third-party includes
#include <osvr/ClientKit/InterfaceC.h>
#include <osvr/ClientKit/InterfaceCallbackC.h>
#include <osvr/Util/EigenInterop.h>
#include <osvr/Util/Pose3C.h>
#include <osvr/Util/Vec2C.h>
#include <osvr/Util/Vec3C.h>
#include <osvr/Util/Logger.h>
#include <tobii/tobii.h>
#include <tobii/tobii_wearable.h>
#include <tobii/tobii_engine.h>

// Standard includes
#include <chrono> // for std::chrono_literals
#include <thread> // for std::thread, std::mutex, std::lock_guard
#include <ostream> // for std::flush

using namespace osvr::pluginkit;
using namespace TobiiOSVR;

class TobiiEyeTracker : public ::TobiiOSVR::EyeTrackerBase {
    protected:
        bool mLastIsBlinkingSynced = false;
        GazeState mLastLeftEyeGazeStateSynced;
        GazeState mLastRightEyeGazeStateSynced;
        tobii_api_t* mAPI = nullptr;
        tobii_engine_t* mEngine = nullptr;
        tobii_device_t* mDevice = nullptr;
        bool mWearableSubscribed = false;

        std::mutex mMutex;

        static void url_receiver(char const* url, void* user_data) {
            //TobiiEyeTracker* _this = reinterpret_cast<TobiiEyeTracker>(user_data);
            // TODO: do something with this?
            char* buffer = reinterpret_cast<char*>(user_data);
            if(!buffer || *buffer != '\0') {
                return;
            }

            if(strlen(url) < 256) {
                strcpy_s(buffer, 255, url);
            }
        }

        static bool convertGazeState(
			tobii_wearable_eye_t const &eye,
            GazeState& gazeState, 
            bool &blinkState) {
            if(eye.gaze_origin_validity == TOBII_VALIDITY_INVALID ||
               eye.gaze_direction_validity == TOBII_VALIDITY_INVALID ||
               eye.eye_openness_validity == TOBII_VALIDITY_INVALID ||
               eye.pupil_position_in_sensor_area_validity == TOBII_VALIDITY_INVALID) {
                // no need to log, this may happen frequently and is probably normal
                return false;
            }

            // TODO: transform data->gaze_origin_mm_xyz into the OSVR head space
            osvrVec3SetX(&gazeState.gazeBasePoint, eye.gaze_origin_mm_xyz[0] * 1000);
            osvrVec3SetY(&gazeState.gazeBasePoint, eye.gaze_origin_mm_xyz[1] * 1000);
            osvrVec3SetZ(&gazeState.gazeBasePoint, eye.gaze_origin_mm_xyz[2] * 1000);

            osvrVec3SetX(&gazeState.gazeDirection, eye.gaze_direction_normalized_xyz[0] * 1000);
            osvrVec3SetY(&gazeState.gazeDirection, eye.gaze_direction_normalized_xyz[1] * 1000);
            osvrVec3SetZ(&gazeState.gazeDirection, eye.gaze_direction_normalized_xyz[2] * 1000);

            // TODO: convert this (0.0, 0.0) -> (1.0, 1.0) range into what OSVR expects here
            osvrVec2SetX(&gazeState.gazePosition, eye.pupil_position_in_sensor_area_xy[0]);
            osvrVec2SetY(&gazeState.gazePosition, eye.pupil_position_in_sensor_area_xy[1]);

            // TODO: what's a good threshold here? 0.5 is fully open
            blinkState = (eye.eye_openness < 0.1 || eye.eye_openness > 0.9);
            return true;
        }

        // This callback is not gauranteed to be on the same thread as the one that subscribed
        // to these callbacks. Using mutex to protect access to synced variables.
        static void wearable_callback(tobii_wearable_data_t const* data, void* user_data) {
            TobiiEyeTracker* _this = reinterpret_cast<TobiiEyeTracker*>(user_data);
            std::lock_guard<std::mutex> lock(_this->mMutex);
            bool leftIsBlinking, rightIsBlinking;
            convertGazeState(data->left, _this->mLastLeftEyeGazeStateSynced, leftIsBlinking);
            convertGazeState(data->right, _this->mLastRightEyeGazeStateSynced, rightIsBlinking);
            _this->mLastIsBlinking = leftIsBlinking || rightIsBlinking;
            // TODO: do we need to report left/right eye blinking separately?
        }

        std::string logTobiiError(std::string const &functionName, tobii_error_t errorCode) {
            mLog->error() << "Tobii SDK function " << functionName 
                << " returned the following error: " << tobii_error_message(errorCode)
                << std::flush;
        }

    public:
		TobiiEyeTracker() : EyeTrackerBase() {}
        virtual ~TobiiEyeTracker() {
            tobii_error_t err = TOBII_ERROR_NO_ERROR;
            if(mDevice) {
                if(mWearableSubscribed) {
                    err = tobii_wearable_data_unsubscribe(mDevice);
                    if(err != TOBII_ERROR_NO_ERROR) {
                        logTobiiError("tobii_wearable_data_unsubscribe", err);
                    }
                }

                err = tobii_device_destroy(mDevice);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_device_destroy", err);
                }
            }
            if(mEngine) {
                err = tobii_engine_destroy(mEngine);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_engine_destroy", err);
                }
            }
            if(mAPI) {
                err = tobii_api_destroy(mAPI);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_api_destroy", err);
                }
            }
        }

        virtual bool init() override {
            if(mInitialized) {
                return true;
            }
            tobii_error_t err = TOBII_ERROR_NO_ERROR;
            
            if(!mAPI) {
                err = tobii_api_create(&mAPI, nullptr, nullptr);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_api_create", err);
                    mAPI = nullptr;
                    return false;
                }
            }

            if(!mEngine) {
                err = tobii_engine_create(mAPI, &mEngine);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_engine_create", err);
                    mEngine = nullptr;
                    return false;
                }
            }
            
            // for now, only try once per device
            if(!mDevice) {
                char url[256] = {0};
                err = tobii_enumerate_local_device_urls(mAPI, url_receiver, url);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_enumerate_local_device_urls", err);
                    return false;
                }

                err = tobii_device_create(mAPI, url, &mDevice);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_device_create", err);
                    mDevice = nullptr;
                    return false;
                }

                err = tobii_device_clear_callback_buffers(mDevice);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_device_clear_callback_buffers", err);
                    return false; // TODO: Do we actually need to return false here?
                }

                tobii_supported_t supported = TOBII_NOT_SUPPORTED;
                err = tobii_stream_supported(mDevice, TOBII_STREAM_WEARABLE, &supported);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_stream_supported", err);
                    return false;
                }
                if(supported == TOBII_NOT_SUPPORTED) {
                    mLog->error() << "Tobii device reports that it does not support the TOBII_STREAM_WEARABLE stream type."
                        << " TOBII_STREAM_WEARABLE is required for OSVR-Tobii." << std::flush;
                    return false;
                }
            }

            if(!mWearableSubscribed) {
                err = tobii_wearable_data_subscribe(mDevice, wearable_callback, this);
                if(err != TOBII_ERROR_NO_ERROR) {
                    logTobiiError("tobii_wearable_data_subscribe", err);
                    return false;
                }
                mWearableSubscribed = true;
            }
            mInitialized = true;
        }

        virtual bool waitForData() override {
            if(!mInitialized) {
                mLog->error() << "Must call TobiiEyeTracker::init before waitForData." << std::flush;
                return false;
            }

            tobii_error_t err = TOBII_ERROR_NO_ERROR;
            err = tobii_wait_for_callbacks(mEngine, 1, &mDevice);
            if(err != TOBII_ERROR_NO_ERROR) {
                // TOBII_ERROR_TIMED_OUT is normal/non-error, so don't log it
                if(err != TOBII_ERROR_TIMED_OUT) {
                    logTobiiError("tobii_wait_for_callbacks", err);
                }
                return false;
            }

            err = tobii_device_process_callbacks(mDevice);
            if(err != TOBII_ERROR_NO_ERROR) {
                logTobiiError("tobii_device_process_callbacks", err);
                return false;
            }

            return true;
        }

        virtual void getLeftEyeGazeState(GazeState &gazeState) override {
            std::lock_guard<std::mutex> lock(mMutex);
            gazeState = mLastLeftEyeGazeStateSynced;
        }

        virtual void getRightEyeGazeState(GazeState &gazeState) override {
            std::lock_guard<std::mutex> lock(mMutex);
            gazeState = mLastRightEyeGazeStateSynced;
        }

        virtual bool getIsBlinking() override {
            std::lock_guard<std::mutex> lock(mMutex);
            return mLastIsBlinkingSynced;
        }

};

TrackerDevice::TrackerDevice(OSVR_PluginRegContext pContext) {
	mLog = osvr::util::log::make_logger(EYE_TRACKER_LOG);

    OSVR_DeviceInitOptions options = osvrDeviceCreateInitOptions(pContext);
    osvrDeviceEyeTrackerConfigure(options, &mEyeTrackerInterface, NumEyeTrackerChannels);

    mDeviceToken.initAsync(pContext, "TobiiDevice", options);

    mDeviceToken.sendJsonDescriptor(org_osvr_Tobii_json);

    this->tryInit();

    mDeviceToken.registerUpdateCallback(this);
}

bool TrackerDevice::tryInit() {
    if(mInitialized) {
        return true;
    }
    if(!mEyeTracker) {
        mEyeTracker = std::make_shared<EyeTrackerBase>();
    }

    if(!mEyeTracker->init()) {
        mLog->warn() << "Could not initialize tobii eye tracker. Will try again later."
            << std::flush;
        return false;
    }
    mInitialized = true;
    return true;
}

TrackerDevice::~TrackerDevice() {}

OSVR_ReturnCode TrackerDevice::update() {
    OSVR_TimeValue timestamp;
    osvrTimeValueGetNow(&timestamp);
    if(mEyeTracker->waitForData()) {

        GazeState leftGazeState, rightGazeState;
        mEyeTracker->getLeftEyeGazeState(leftGazeState);
        mEyeTracker->getRightEyeGazeState(rightGazeState);

        osvrDeviceEyeTrackerReportGaze(
            mEyeTrackerInterface,
            leftGazeState.gazePosition,
            leftGazeState.gazeDirection,
            leftGazeState.gazeBasePoint,
            LeftEyeTrackerChannel,
            &timestamp);
        
        osvrDeviceEyeTrackerReportGaze(
            mEyeTrackerInterface,
            rightGazeState.gazePosition,
            rightGazeState.gazeDirection,
            rightGazeState.gazeBasePoint,
            RightEyeTrackerChannel,
            &timestamp);

        if (mLastIsBlinking != mEyeTracker->getIsBlinking()) {
            mLastIsBlinking = mEyeTracker->getIsBlinking();
            osvrDeviceEyeTrackerReportBlink(mEyeTrackerInterface, mLastIsBlinking, BlinkChannel, &timestamp);
        }
    }

    
    return OSVR_RETURN_SUCCESS;
}
