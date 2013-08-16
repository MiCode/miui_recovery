/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AudioPolicyManagerBase"
//#define LOG_NDEBUG 0

//#define VERY_VERBOSE_LOGGING
#ifdef VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

// A device mask for all audio input devices that are considered "virtual" when evaluating
// active inputs in getActiveInput()
#define APM_AUDIO_IN_DEVICE_VIRTUAL_ALL  AUDIO_DEVICE_IN_REMOTE_SUBMIX

#include <utils/Log.h>
#include <hardware_legacy/AudioPolicyManagerBase.h>
#include <hardware/audio_effect.h>
#include <hardware/audio.h>
#include <math.h>
#include <hardware_legacy/audio_policy_conf.h>

namespace android_audio_legacy {

// ----------------------------------------------------------------------------
// AudioPolicyInterface implementation
// ----------------------------------------------------------------------------


status_t AudioPolicyManagerBase::setDeviceConnectionState(audio_devices_t device,
                                                  AudioSystem::device_connection_state state,
                                                  const char *device_address)
{
    SortedVector <audio_io_handle_t> outputs;

    ALOGV("setDeviceConnectionState() device: %x, state %d, address %s", device, state, device_address);

    // connect/disconnect only 1 device at a time
    if (!audio_is_output_device(device) && !audio_is_input_device(device)) return BAD_VALUE;

    if (strlen(device_address) >= MAX_DEVICE_ADDRESS_LEN) {
        ALOGE("setDeviceConnectionState() invalid address: %s", device_address);
        return BAD_VALUE;
    }

    // handle output devices
    if (audio_is_output_device(device)) {

        if (!mHasA2dp && audio_is_a2dp_device(device)) {
            ALOGE("setDeviceConnectionState() invalid A2DP device: %x", device);
            return BAD_VALUE;
        }
        if (!mHasUsb && audio_is_usb_device(device)) {
            ALOGE("setDeviceConnectionState() invalid USB audio device: %x", device);
            return BAD_VALUE;
        }
        if (!mHasRemoteSubmix && audio_is_remote_submix_device((audio_devices_t)device)) {
            ALOGE("setDeviceConnectionState() invalid remote submix audio device: %x", device);
            return BAD_VALUE;
        }

        // save a copy of the opened output descriptors before any output is opened or closed
        // by checkOutputsForDevice(). This will be needed by checkOutputForAllStrategies()
        mPreviousOutputs = mOutputs;
        switch (state)
        {
        // handle output device connection
        case AudioSystem::DEVICE_STATE_AVAILABLE:
            if (mAvailableOutputDevices & device) {
                ALOGW("setDeviceConnectionState() device already connected: %x", device);
                return INVALID_OPERATION;
            }
            ALOGV("setDeviceConnectionState() connecting device %x", device);

            if (checkOutputsForDevice(device, state, outputs) != NO_ERROR) {
                return INVALID_OPERATION;
            }
            ALOGV("setDeviceConnectionState() checkOutputsForDevice() returned %d outputs",
                  outputs.size());
            // register new device as available
            mAvailableOutputDevices = (audio_devices_t)(mAvailableOutputDevices | device);

            if (!outputs.isEmpty()) {
                String8 paramStr;
                if (mHasA2dp && audio_is_a2dp_device(device)) {
                    // handle A2DP device connection
                    AudioParameter param;
                    param.add(String8(AUDIO_PARAMETER_A2DP_SINK_ADDRESS), String8(device_address));
                    paramStr = param.toString();
                    mA2dpDeviceAddress = String8(device_address, MAX_DEVICE_ADDRESS_LEN);
                    mA2dpSuspended = false;
                } else if (audio_is_bluetooth_sco_device(device)) {
                    // handle SCO device connection
                    mScoDeviceAddress = String8(device_address, MAX_DEVICE_ADDRESS_LEN);
                } else if (mHasUsb && audio_is_usb_device(device)) {
                    // handle USB device connection
                    mUsbCardAndDevice = String8(device_address, MAX_DEVICE_ADDRESS_LEN);
                    paramStr = mUsbCardAndDevice;
                }
                // not currently handling multiple simultaneous submixes: ignoring remote submix
                //   case and address
                if (!paramStr.isEmpty()) {
                    for (size_t i = 0; i < outputs.size(); i++) {
                        mpClientInterface->setParameters(outputs[i], paramStr);
                    }
                }
            }
            break;
        // handle output device disconnection
        case AudioSystem::DEVICE_STATE_UNAVAILABLE: {
            if (!(mAvailableOutputDevices & device)) {
                ALOGW("setDeviceConnectionState() device not connected: %x", device);
                return INVALID_OPERATION;
            }

            ALOGV("setDeviceConnectionState() disconnecting device %x", device);
            // remove device from available output devices
            mAvailableOutputDevices = (audio_devices_t)(mAvailableOutputDevices & ~device);

            checkOutputsForDevice(device, state, outputs);
            if (mHasA2dp && audio_is_a2dp_device(device)) {
                // handle A2DP device disconnection
                mA2dpDeviceAddress = "";
                mA2dpSuspended = false;
            } else if (audio_is_bluetooth_sco_device(device)) {
                // handle SCO device disconnection
                mScoDeviceAddress = "";
            } else if (mHasUsb && audio_is_usb_device(device)) {
                // handle USB device disconnection
                mUsbCardAndDevice = "";
            }
            // not currently handling multiple simultaneous submixes: ignoring remote submix
            //   case and address
            } break;

        default:
            ALOGE("setDeviceConnectionState() invalid state: %x", state);
            return BAD_VALUE;
        }

        checkA2dpSuspend();
        checkOutputForAllStrategies();
        // outputs must be closed after checkOutputForAllStrategies() is executed
        if (!outputs.isEmpty()) {
            for (size_t i = 0; i < outputs.size(); i++) {
                // close unused outputs after device disconnection or direct outputs that have been
                // opened by checkOutputsForDevice() to query dynamic parameters
                if ((state == AudioSystem::DEVICE_STATE_UNAVAILABLE) ||
                        (mOutputs.valueFor(outputs[i])->mFlags & AUDIO_OUTPUT_FLAG_DIRECT)) {
                    closeOutput(outputs[i]);
                }
            }
        }

        updateDevicesAndOutputs();
        for (size_t i = 0; i < mOutputs.size(); i++) {
            // do not force device change on duplicated output because if device is 0, it will
            // also force a device 0 for the two outputs it is duplicated to which may override
            // a valid device selection on those outputs.
            setOutputDevice(mOutputs.keyAt(i),
                            getNewDevice(mOutputs.keyAt(i), true /*fromCache*/),
                            !mOutputs.valueAt(i)->isDuplicated(),
                            0);
        }

        if (device == AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            device = AUDIO_DEVICE_IN_WIRED_HEADSET;
        } else if (device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO ||
                   device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET ||
                   device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT) {
            device = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        } else {
            return NO_ERROR;
        }
    }
    // handle input devices
    if (audio_is_input_device(device)) {

        switch (state)
        {
        // handle input device connection
        case AudioSystem::DEVICE_STATE_AVAILABLE: {
            if (mAvailableInputDevices & device) {
                ALOGW("setDeviceConnectionState() device already connected: %d", device);
                return INVALID_OPERATION;
            }
            mAvailableInputDevices = mAvailableInputDevices | (device & ~AUDIO_DEVICE_BIT_IN);
            }
            break;

        // handle input device disconnection
        case AudioSystem::DEVICE_STATE_UNAVAILABLE: {
            if (!(mAvailableInputDevices & device)) {
                ALOGW("setDeviceConnectionState() device not connected: %d", device);
                return INVALID_OPERATION;
            }
            mAvailableInputDevices = (audio_devices_t) (mAvailableInputDevices & ~device);
            } break;

        default:
            ALOGE("setDeviceConnectionState() invalid state: %x", state);
            return BAD_VALUE;
        }

        audio_io_handle_t activeInput = getActiveInput();
        if (activeInput != 0) {
            AudioInputDescriptor *inputDesc = mInputs.valueFor(activeInput);
            audio_devices_t newDevice = getDeviceForInputSource(inputDesc->mInputSource);
            if ((newDevice != AUDIO_DEVICE_NONE) && (newDevice != inputDesc->mDevice)) {
                ALOGV("setDeviceConnectionState() changing device from %x to %x for input %d",
                        inputDesc->mDevice, newDevice, activeInput);
                inputDesc->mDevice = newDevice;
                AudioParameter param = AudioParameter();
                param.addInt(String8(AudioParameter::keyRouting), (int)newDevice);
                mpClientInterface->setParameters(activeInput, param.toString());
            }
        }

        return NO_ERROR;
    }

    ALOGW("setDeviceConnectionState() invalid device: %x", device);
    return BAD_VALUE;
}

AudioSystem::device_connection_state AudioPolicyManagerBase::getDeviceConnectionState(audio_devices_t device,
                                                  const char *device_address)
{
    AudioSystem::device_connection_state state = AudioSystem::DEVICE_STATE_UNAVAILABLE;
    String8 address = String8(device_address);
    if (audio_is_output_device(device)) {
        if (device & mAvailableOutputDevices) {
            if (audio_is_a2dp_device(device) &&
                (!mHasA2dp || (address != "" && mA2dpDeviceAddress != address))) {
                return state;
            }
            if (audio_is_bluetooth_sco_device(device) &&
                address != "" && mScoDeviceAddress != address) {
                return state;
            }
            if (audio_is_usb_device(device) &&
                (!mHasUsb || (address != "" && mUsbCardAndDevice != address))) {
                ALOGE("getDeviceConnectionState() invalid device: %x", device);
                return state;
            }
            if (audio_is_remote_submix_device((audio_devices_t)device) && !mHasRemoteSubmix) {
                return state;
            }
            state = AudioSystem::DEVICE_STATE_AVAILABLE;
        }
    } else if (audio_is_input_device(device)) {
        if (device & mAvailableInputDevices) {
            state = AudioSystem::DEVICE_STATE_AVAILABLE;
        }
    }

    return state;
}

void AudioPolicyManagerBase::setPhoneState(int state)
{
    ALOGV("setPhoneState() state %d", state);
    audio_devices_t newDevice = AUDIO_DEVICE_NONE;
    if (state < 0 || state >= AudioSystem::NUM_MODES) {
        ALOGW("setPhoneState() invalid state %d", state);
        return;
    }

    if (state == mPhoneState ) {
        ALOGW("setPhoneState() setting same state %d", state);
        return;
    }

    // if leaving call state, handle special case of active streams
    // pertaining to sonification strategy see handleIncallSonification()
    if (isInCall()) {
        ALOGV("setPhoneState() in call state management: new state is %d", state);
        for (int stream = 0; stream < AudioSystem::NUM_STREAM_TYPES; stream++) {
            handleIncallSonification(stream, false, true);
        }
    }

    // store previous phone state for management of sonification strategy below
    int oldState = mPhoneState;
    mPhoneState = state;
    bool force = false;

    // are we entering or starting a call
    if (!isStateInCall(oldState) && isStateInCall(state)) {
        ALOGV("  Entering call in setPhoneState()");
        // force routing command to audio hardware when starting a call
        // even if no device change is needed
        force = true;
    } else if (isStateInCall(oldState) && !isStateInCall(state)) {
        ALOGV("  Exiting call in setPhoneState()");
        // force routing command to audio hardware when exiting a call
        // even if no device change is needed
        force = true;
    } else if (isStateInCall(state) && (state != oldState)) {
        ALOGV("  Switching between telephony and VoIP in setPhoneState()");
        // force routing command to audio hardware when switching between telephony and VoIP
        // even if no device change is needed
        force = true;
    }

    // check for device and output changes triggered by new phone state
    newDevice = getNewDevice(mPrimaryOutput, false /*fromCache*/);
    checkA2dpSuspend();
    checkOutputForAllStrategies();
    updateDevicesAndOutputs();

    AudioOutputDescriptor *hwOutputDesc = mOutputs.valueFor(mPrimaryOutput);

    // force routing command to audio hardware when ending call
    // even if no device change is needed
    if (isStateInCall(oldState) && newDevice == AUDIO_DEVICE_NONE) {
        newDevice = hwOutputDesc->device();
    }

    // when changing from ring tone to in call mode, mute the ringing tone
    // immediately and delay the route change to avoid sending the ring tone
    // tail into the earpiece or headset.
    int delayMs = 0;
    if (isStateInCall(state) && oldState == AudioSystem::MODE_RINGTONE) {
        // delay the device change command by twice the output latency to have some margin
        // and be sure that audio buffers not yet affected by the mute are out when
        // we actually apply the route change
        delayMs = hwOutputDesc->mLatency*2;
        setStreamMute(AudioSystem::RING, true, mPrimaryOutput);
    }

    if (isStateInCall(state)) {
        for (size_t i = 0; i < mOutputs.size(); i++) {
            AudioOutputDescriptor *desc = mOutputs.valueAt(i);
            //take the biggest latency for all outputs
            if (delayMs < (int)desc->mLatency*2) {
                delayMs = desc->mLatency*2;
            }
            //mute STRATEGY_MEDIA on all outputs
            if (desc->strategyRefCount(STRATEGY_MEDIA) != 0) {
                setStrategyMute(STRATEGY_MEDIA, true, mOutputs.keyAt(i));
                setStrategyMute(STRATEGY_MEDIA, false, mOutputs.keyAt(i), MUTE_TIME_MS,
                    getDeviceForStrategy(STRATEGY_MEDIA, true /*fromCache*/));
            }
        }
    }

    // change routing is necessary
    setOutputDevice(mPrimaryOutput, newDevice, force, delayMs);

    // if entering in call state, handle special case of active streams
    // pertaining to sonification strategy see handleIncallSonification()
    if (isStateInCall(state)) {
        ALOGV("setPhoneState() in call state management: new state is %d", state);
        // unmute the ringing tone after a sufficient delay if it was muted before
        // setting output device above
        if (oldState == AudioSystem::MODE_RINGTONE) {
            setStreamMute(AudioSystem::RING, false, mPrimaryOutput, MUTE_TIME_MS);
        }
        for (int stream = 0; stream < AudioSystem::NUM_STREAM_TYPES; stream++) {
            handleIncallSonification(stream, true, true);
        }
    }

    // Flag that ringtone volume must be limited to music volume until we exit MODE_RINGTONE
    if (state == AudioSystem::MODE_RINGTONE &&
        isStreamActive(AudioSystem::MUSIC, SONIFICATION_HEADSET_MUSIC_DELAY)) {
        mLimitRingtoneVolume = true;
    } else {
        mLimitRingtoneVolume = false;
    }
}

void AudioPolicyManagerBase::setForceUse(AudioSystem::force_use usage, AudioSystem::forced_config config)
{
    ALOGV("setForceUse() usage %d, config %d, mPhoneState %d", usage, config, mPhoneState);

    bool forceVolumeReeval = false;
    switch(usage) {
    case AudioSystem::FOR_COMMUNICATION:
        if (config != AudioSystem::FORCE_SPEAKER && config != AudioSystem::FORCE_BT_SCO &&
            config != AudioSystem::FORCE_NONE) {
            ALOGW("setForceUse() invalid config %d for FOR_COMMUNICATION", config);
            return;
        }
        forceVolumeReeval = true;
        mForceUse[usage] = config;
        break;
    case AudioSystem::FOR_MEDIA:
        if (config != AudioSystem::FORCE_HEADPHONES && config != AudioSystem::FORCE_BT_A2DP &&
            config != AudioSystem::FORCE_WIRED_ACCESSORY &&
            config != AudioSystem::FORCE_ANALOG_DOCK &&
            config != AudioSystem::FORCE_DIGITAL_DOCK && config != AudioSystem::FORCE_NONE &&
            config != AudioSystem::FORCE_NO_BT_A2DP) {
            ALOGW("setForceUse() invalid config %d for FOR_MEDIA", config);
            return;
        }
        mForceUse[usage] = config;
        break;
    case AudioSystem::FOR_RECORD:
        if (config != AudioSystem::FORCE_BT_SCO && config != AudioSystem::FORCE_WIRED_ACCESSORY &&
            config != AudioSystem::FORCE_NONE) {
            ALOGW("setForceUse() invalid config %d for FOR_RECORD", config);
            return;
        }
        mForceUse[usage] = config;
        break;
    case AudioSystem::FOR_DOCK:
        if (config != AudioSystem::FORCE_NONE && config != AudioSystem::FORCE_BT_CAR_DOCK &&
            config != AudioSystem::FORCE_BT_DESK_DOCK &&
            config != AudioSystem::FORCE_WIRED_ACCESSORY &&
            config != AudioSystem::FORCE_ANALOG_DOCK &&
            config != AudioSystem::FORCE_DIGITAL_DOCK) {
            ALOGW("setForceUse() invalid config %d for FOR_DOCK", config);
        }
        forceVolumeReeval = true;
        mForceUse[usage] = config;
        break;
    case AudioSystem::FOR_SYSTEM:
        if (config != AudioSystem::FORCE_NONE &&
            config != AudioSystem::FORCE_SYSTEM_ENFORCED) {
            ALOGW("setForceUse() invalid config %d for FOR_SYSTEM", config);
        }
        forceVolumeReeval = true;
        mForceUse[usage] = config;
        break;
    default:
        ALOGW("setForceUse() invalid usage %d", usage);
        break;
    }

    // check for device and output changes triggered by new force usage
    checkA2dpSuspend();
    checkOutputForAllStrategies();
    updateDevicesAndOutputs();
    for (size_t i = 0; i < mOutputs.size(); i++) {
        audio_io_handle_t output = mOutputs.keyAt(i);
        audio_devices_t newDevice = getNewDevice(output, true /*fromCache*/);
        setOutputDevice(output, newDevice, (newDevice != AUDIO_DEVICE_NONE));
        if (forceVolumeReeval && (newDevice != AUDIO_DEVICE_NONE)) {
            applyStreamVolumes(output, newDevice, 0, true);
        }
    }

    audio_io_handle_t activeInput = getActiveInput();
    if (activeInput != 0) {
        AudioInputDescriptor *inputDesc = mInputs.valueFor(activeInput);
        audio_devices_t newDevice = getDeviceForInputSource(inputDesc->mInputSource);
        if ((newDevice != AUDIO_DEVICE_NONE) && (newDevice != inputDesc->mDevice)) {
            ALOGV("setForceUse() changing device from %x to %x for input %d",
                    inputDesc->mDevice, newDevice, activeInput);
            inputDesc->mDevice = newDevice;
            AudioParameter param = AudioParameter();
            param.addInt(String8(AudioParameter::keyRouting), (int)newDevice);
            mpClientInterface->setParameters(activeInput, param.toString());
        }
    }

}

AudioSystem::forced_config AudioPolicyManagerBase::getForceUse(AudioSystem::force_use usage)
{
    return mForceUse[usage];
}

void AudioPolicyManagerBase::setSystemProperty(const char* property, const char* value)
{
    ALOGV("setSystemProperty() property %s, value %s", property, value);
}

AudioPolicyManagerBase::IOProfile *AudioPolicyManagerBase::getProfileForDirectOutput(
                                                               audio_devices_t device,
                                                               uint32_t samplingRate,
                                                               uint32_t format,
                                                               uint32_t channelMask,
                                                               audio_output_flags_t flags)
{
    for (size_t i = 0; i < mHwModules.size(); i++) {
        if (mHwModules[i]->mHandle == 0) {
            continue;
        }
        for (size_t j = 0; j < mHwModules[i]->mOutputProfiles.size(); j++) {
           IOProfile *profile = mHwModules[i]->mOutputProfiles[j];
           if (profile->isCompatibleProfile(device, samplingRate, format,
                                           channelMask,
#ifdef QCOM_HARDWARE
                                          (audio_output_flags_t)(flags|AUDIO_OUTPUT_FLAG_DIRECT)))
#else
                                           AUDIO_OUTPUT_FLAG_DIRECT))
#endif
           {
               if (mAvailableOutputDevices & profile->mSupportedDevices) {
                   return mHwModules[i]->mOutputProfiles[j];
               }
           }
        }
    }
    return 0;
}

audio_io_handle_t AudioPolicyManagerBase::getOutput(AudioSystem::stream_type stream,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channelMask,
                                    AudioSystem::output_flags flags)
{
    audio_io_handle_t output = 0;
    uint32_t latency = 0;
    routing_strategy strategy = getStrategy((AudioSystem::stream_type)stream);
    audio_devices_t device = getDeviceForStrategy(strategy, false /*fromCache*/);
    ALOGV("getOutput() stream %d, samplingRate %d, format %d, channelMask %x, flags %x",
          stream, samplingRate, format, channelMask, flags);

#ifdef AUDIO_POLICY_TEST
    if (mCurOutput != 0) {
        ALOGV("getOutput() test output mCurOutput %d, samplingRate %d, format %d, channelMask %x, mDirectOutput %d",
                mCurOutput, mTestSamplingRate, mTestFormat, mTestChannels, mDirectOutput);

        if (mTestOutputs[mCurOutput] == 0) {
            ALOGV("getOutput() opening test output");
            AudioOutputDescriptor *outputDesc = new AudioOutputDescriptor(NULL);
            outputDesc->mDevice = mTestDevice;
            outputDesc->mSamplingRate = mTestSamplingRate;
            outputDesc->mFormat = mTestFormat;
            outputDesc->mChannelMask = mTestChannels;
            outputDesc->mLatency = mTestLatencyMs;
            outputDesc->mFlags = (audio_output_flags_t)(mDirectOutput ? AudioSystem::OUTPUT_FLAG_DIRECT : 0);
            outputDesc->mRefCount[stream] = 0;
            mTestOutputs[mCurOutput] = mpClientInterface->openOutput(0, &outputDesc->mDevice,
                                            &outputDesc->mSamplingRate,
                                            &outputDesc->mFormat,
                                            &outputDesc->mChannelMask,
                                            &outputDesc->mLatency,
                                            outputDesc->mFlags);
            if (mTestOutputs[mCurOutput]) {
                AudioParameter outputCmd = AudioParameter();
                outputCmd.addInt(String8("set_id"),mCurOutput);
                mpClientInterface->setParameters(mTestOutputs[mCurOutput],outputCmd.toString());
                addOutput(mTestOutputs[mCurOutput], outputDesc);
            }
        }
        return mTestOutputs[mCurOutput];
    }
#endif //AUDIO_POLICY_TEST

    IOProfile *profile = getProfileForDirectOutput(device,
                                            samplingRate,
                                            format,
                                            channelMask,
                                            (audio_output_flags_t)flags);

    if (profile != NULL) {

        ALOGV("getOutput() opening direct output device %x", device);

        AudioOutputDescriptor *outputDesc = new AudioOutputDescriptor(profile);
        outputDesc->mDevice = device;
        outputDesc->mSamplingRate = samplingRate;
        outputDesc->mFormat = (audio_format_t)format;
        outputDesc->mChannelMask = (audio_channel_mask_t)channelMask;
        outputDesc->mLatency = 0;
        outputDesc->mFlags = (audio_output_flags_t)(flags | AUDIO_OUTPUT_FLAG_DIRECT);;
        outputDesc->mRefCount[stream] = 0;
        outputDesc->mStopTime[stream] = 0;
        output = mpClientInterface->openOutput(profile->mModule->mHandle,
                                        &outputDesc->mDevice,
                                        &outputDesc->mSamplingRate,
                                        &outputDesc->mFormat,
                                        &outputDesc->mChannelMask,
                                        &outputDesc->mLatency,
                                        outputDesc->mFlags);

        // only accept an output with the requested parameters
        if (output == 0 ||
            (samplingRate != 0 && samplingRate != outputDesc->mSamplingRate) ||
            (format != 0 && format != outputDesc->mFormat) ||
            (channelMask != 0 && channelMask != outputDesc->mChannelMask)) {
            ALOGV("getOutput() failed opening direct output: output %d samplingRate %d %d,"
                    "format %d %d, channelMask %04x %04x", output, samplingRate,
                    outputDesc->mSamplingRate, format, outputDesc->mFormat, channelMask,
                    outputDesc->mChannelMask);
            if (output != 0) {
                mpClientInterface->closeOutput(output);
            }
            delete outputDesc;
            return 0;
        }
        addOutput(output, outputDesc);
        ALOGV("getOutput() returns direct output %d", output);
        return output;
    }

    // ignoring channel mask due to downmix capability in mixer

    // open a non direct output

    // get which output is suitable for the specified stream. The actual routing change will happen
    // when startOutput() will be called
    SortedVector<audio_io_handle_t> outputs = getOutputsForDevice(device, mOutputs);

    output = selectOutput(outputs, flags);

    ALOGW_IF((output ==0), "getOutput() could not find output for stream %d, samplingRate %d,"
            "format %d, channels %x, flags %x", stream, samplingRate, format, channelMask, flags);

    ALOGV("getOutput() returns output %d", output);

    return output;
}

audio_io_handle_t AudioPolicyManagerBase::selectOutput(const SortedVector<audio_io_handle_t>& outputs,
                                                       AudioSystem::output_flags flags)
{
    // select one output among several that provide a path to a particular device or set of
    // devices (the list was previously build by getOutputsForDevice()).
    // The priority is as follows:
    // 1: the output with the highest number of requested policy flags
    // 2: the primary output
    // 3: the first output in the list

    if (outputs.size() == 0) {
        return 0;
    }
    if (outputs.size() == 1) {
        return outputs[0];
    }

    int maxCommonFlags = 0;
    audio_io_handle_t outputFlags = 0;
    audio_io_handle_t outputPrimary = 0;

    for (size_t i = 0; i < outputs.size(); i++) {
        AudioOutputDescriptor *outputDesc = mOutputs.valueFor(outputs[i]);
        if (!outputDesc->isDuplicated()) {
            int commonFlags = (int)AudioSystem::popCount(outputDesc->mProfile->mFlags & flags);
            if (commonFlags > maxCommonFlags) {
                outputFlags = outputs[i];
                maxCommonFlags = commonFlags;
                ALOGV("selectOutput() commonFlags for output %d, %04x", outputs[i], commonFlags);
            }
            if (outputDesc->mProfile->mFlags & AUDIO_OUTPUT_FLAG_PRIMARY) {
                outputPrimary = outputs[i];
            }
        }
    }

    if (outputFlags != 0) {
        return outputFlags;
    }
    if (outputPrimary != 0) {
        return outputPrimary;
    }

    return outputs[0];
}

status_t AudioPolicyManagerBase::startOutput(audio_io_handle_t output,
                                             AudioSystem::stream_type stream,
                                             int session)
{
    ALOGV("startOutput() output %d, stream %d, session %d", output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("startOutput() unknow output %d", output);
        return BAD_VALUE;
    }

    AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);

    // increment usage count for this stream on the requested output:
    // NOTE that the usage count is the same for duplicated output and hardware output which is
    // necessary for a correct control of hardware output routing by startOutput() and stopOutput()
    outputDesc->changeRefCount(stream, 1);

    if (outputDesc->mRefCount[stream] == 1) {
        audio_devices_t newDevice = getNewDevice(output, false /*fromCache*/);
        routing_strategy strategy = getStrategy(stream);
        bool shouldWait = (strategy == STRATEGY_SONIFICATION) ||
                            (strategy == STRATEGY_SONIFICATION_RESPECTFUL);
        uint32_t waitMs = 0;
        bool force = false;
        for (size_t i = 0; i < mOutputs.size(); i++) {
            AudioOutputDescriptor *desc = mOutputs.valueAt(i);
            if (desc != outputDesc) {
                // force a device change if any other output is managed by the same hw
                // module and has a current device selection that differs from selected device.
                // In this case, the audio HAL must receive the new device selection so that it can
                // change the device currently selected by the other active output.
                if (outputDesc->sharesHwModuleWith(desc) &&
                    desc->device() != newDevice) {
                    force = true;
                }
                // wait for audio on other active outputs to be presented when starting
                // a notification so that audio focus effect can propagate.
                uint32_t latency = desc->latency();
                if (shouldWait && desc->isActive(latency * 2) && (waitMs < latency)) {
                    waitMs = latency;
                }
            }
        }
        uint32_t muteWaitMs = setOutputDevice(output, newDevice, force);

        // handle special case for sonification while in call
        if (isInCall()) {
            handleIncallSonification(stream, true, false);
        }

        // apply volume rules for current stream and device if necessary
        checkAndSetVolume(stream,
                          mStreams[stream].getVolumeIndex(newDevice),
                          output,
                          newDevice);

        // update the outputs if starting an output with a stream that can affect notification
        // routing
        handleNotificationRoutingForStream(stream);
        if (waitMs > muteWaitMs) {
            usleep((waitMs - muteWaitMs) * 2 * 1000);
        }
    }
    return NO_ERROR;
}


status_t AudioPolicyManagerBase::stopOutput(audio_io_handle_t output,
                                            AudioSystem::stream_type stream,
                                            int session)
{
    ALOGV("stopOutput() output %d, stream %d, session %d", output, stream, session);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("stopOutput() unknow output %d", output);
        return BAD_VALUE;
    }

    AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);

    // handle special case for sonification while in call
    if (isInCall()) {
        handleIncallSonification(stream, false, false);
    }

    if (outputDesc->mRefCount[stream] > 0) {
        // decrement usage count of this stream on the output
        outputDesc->changeRefCount(stream, -1);
        // store time at which the stream was stopped - see isStreamActive()
        if (outputDesc->mRefCount[stream] == 0) {
            outputDesc->mStopTime[stream] = systemTime();
            audio_devices_t newDevice = getNewDevice(output, false /*fromCache*/);
            // delay the device switch by twice the latency because stopOutput() is executed when
            // the track stop() command is received and at that time the audio track buffer can
            // still contain data that needs to be drained. The latency only covers the audio HAL
            // and kernel buffers. Also the latency does not always include additional delay in the
            // audio path (audio DSP, CODEC ...)
            setOutputDevice(output, newDevice, false, outputDesc->mLatency*2);

            // force restoring the device selection on other active outputs if it differs from the
            // one being selected for this output
            for (size_t i = 0; i < mOutputs.size(); i++) {
                audio_io_handle_t curOutput = mOutputs.keyAt(i);
                AudioOutputDescriptor *desc = mOutputs.valueAt(i);
                if (curOutput != output &&
                        desc->refCount() != 0 &&
                        outputDesc->sharesHwModuleWith(desc) &&
                        newDevice != desc->device()) {
                    setOutputDevice(curOutput,
                                    getNewDevice(curOutput, false /*fromCache*/),
                                    true,
                                    outputDesc->mLatency*2);
                }
            }
            // update the outputs if stopping one with a stream that can affect notification routing
            handleNotificationRoutingForStream(stream);
        }
        return NO_ERROR;
    } else {
        ALOGW("stopOutput() refcount is already 0 for output %d", output);
        return INVALID_OPERATION;
    }
}

void AudioPolicyManagerBase::releaseOutput(audio_io_handle_t output)
{
    ALOGV("releaseOutput() %d", output);
    ssize_t index = mOutputs.indexOfKey(output);
    if (index < 0) {
        ALOGW("releaseOutput() releasing unknown output %d", output);
        return;
    }

#ifdef AUDIO_POLICY_TEST
    int testIndex = testOutputIndex(output);
    if (testIndex != 0) {
        AudioOutputDescriptor *outputDesc = mOutputs.valueAt(index);
        if (outputDesc->refCount() == 0) {
            mpClientInterface->closeOutput(output);
            delete mOutputs.valueAt(index);
            mOutputs.removeItem(output);
            mTestOutputs[testIndex] = 0;
        }
        return;
    }
#endif //AUDIO_POLICY_TEST

    if (mOutputs.valueAt(index)->mFlags & AudioSystem::OUTPUT_FLAG_DIRECT) {
        mpClientInterface->closeOutput(output);
        delete mOutputs.valueAt(index);
        mOutputs.removeItem(output);
        mPreviousOutputs = mOutputs;
    }

}

audio_io_handle_t AudioPolicyManagerBase::getInput(int inputSource,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channelMask,
                                    AudioSystem::audio_in_acoustics acoustics)
{
    audio_io_handle_t input = 0;
    audio_devices_t device = getDeviceForInputSource(inputSource);

    ALOGV("getInput() inputSource %d, samplingRate %d, format %d, channelMask %x, acoustics %x",
          inputSource, samplingRate, format, channelMask, acoustics);

    if (device == AUDIO_DEVICE_NONE) {
        ALOGW("getInput() could not find device for inputSource %d", inputSource);
        return 0;
    }

    // adapt channel selection to input source
    switch(inputSource) {
    case AUDIO_SOURCE_VOICE_UPLINK:
        channelMask = AudioSystem::CHANNEL_IN_VOICE_UPLINK;
        break;
    case AUDIO_SOURCE_VOICE_DOWNLINK:
        channelMask = AudioSystem::CHANNEL_IN_VOICE_DNLINK;
        break;
    case AUDIO_SOURCE_VOICE_CALL:
        channelMask = (AudioSystem::CHANNEL_IN_VOICE_UPLINK | AudioSystem::CHANNEL_IN_VOICE_DNLINK);
        break;
    default:
        break;
    }

    IOProfile *profile = getInputProfile(device,
                                         samplingRate,
                                         format,
                                         channelMask);
    if (profile == NULL) {
        ALOGW("getInput() could not find profile for device %04x, samplingRate %d, format %d,"
                "channelMask %04x",
                device, samplingRate, format, channelMask);
        return 0;
    }

    if (profile->mModule->mHandle == 0) {
        ALOGE("getInput(): HW module %s not opened", profile->mModule->mName);
        return 0;
    }

    AudioInputDescriptor *inputDesc = new AudioInputDescriptor(profile);

    inputDesc->mInputSource = inputSource;
    inputDesc->mDevice = device;
    inputDesc->mSamplingRate = samplingRate;
    inputDesc->mFormat = (audio_format_t)format;
    inputDesc->mChannelMask = (audio_channel_mask_t)channelMask;
    inputDesc->mRefCount = 0;
    input = mpClientInterface->openInput(profile->mModule->mHandle,
                                    &inputDesc->mDevice,
                                    &inputDesc->mSamplingRate,
                                    &inputDesc->mFormat,
                                    &inputDesc->mChannelMask);

    // only accept input with the exact requested set of parameters
    if (input == 0 ||
        (samplingRate != inputDesc->mSamplingRate) ||
        (format != inputDesc->mFormat) ||
        (channelMask != inputDesc->mChannelMask)) {
        ALOGV("getInput() failed opening input: samplingRate %d, format %d, channelMask %d",
                samplingRate, format, channelMask);
        if (input != 0) {
            mpClientInterface->closeInput(input);
        }
        delete inputDesc;
        return 0;
    }
    mInputs.add(input, inputDesc);
    return input;
}

status_t AudioPolicyManagerBase::startInput(audio_io_handle_t input)
{
    ALOGV("startInput() input %d", input);
    ssize_t index = mInputs.indexOfKey(input);
    if (index < 0) {
        ALOGW("startInput() unknow input %d", input);
        return BAD_VALUE;
    }
    AudioInputDescriptor *inputDesc = mInputs.valueAt(index);

#ifdef AUDIO_POLICY_TEST
    if (mTestInput == 0)
#endif //AUDIO_POLICY_TEST
    {
        // refuse 2 active AudioRecord clients at the same time
        if (getActiveInput() != 0) {
            ALOGW("startInput() input %d failed: other input already started", input);
            return INVALID_OPERATION;
        }
    }

    AudioParameter param = AudioParameter();
    param.addInt(String8(AudioParameter::keyRouting), (int)inputDesc->mDevice);

    param.addInt(String8(AudioParameter::keyInputSource), (int)inputDesc->mInputSource);
    ALOGV("AudioPolicyManager::startInput() input source = %d", inputDesc->mInputSource);

    mpClientInterface->setParameters(input, param.toString());

    inputDesc->mRefCount = 1;
    return NO_ERROR;
}

status_t AudioPolicyManagerBase::stopInput(audio_io_handle_t input)
{
    ALOGV("stopInput() input %d", input);
    ssize_t index = mInputs.indexOfKey(input);
    if (index < 0) {
        ALOGW("stopInput() unknow input %d", input);
        return BAD_VALUE;
    }
    AudioInputDescriptor *inputDesc = mInputs.valueAt(index);

    if (inputDesc->mRefCount == 0) {
        ALOGW("stopInput() input %d already stopped", input);
        return INVALID_OPERATION;
    } else {
        AudioParameter param = AudioParameter();
        param.addInt(String8(AudioParameter::keyRouting), 0);
        mpClientInterface->setParameters(input, param.toString());
        inputDesc->mRefCount = 0;
        return NO_ERROR;
    }
}

void AudioPolicyManagerBase::releaseInput(audio_io_handle_t input)
{
    ALOGV("releaseInput() %d", input);
    ssize_t index = mInputs.indexOfKey(input);
    if (index < 0) {
        ALOGW("releaseInput() releasing unknown input %d", input);
        return;
    }
    mpClientInterface->closeInput(input);
    delete mInputs.valueAt(index);
    mInputs.removeItem(input);
    ALOGV("releaseInput() exit");
}

void AudioPolicyManagerBase::initStreamVolume(AudioSystem::stream_type stream,
                                            int indexMin,
                                            int indexMax)
{
    ALOGV("initStreamVolume() stream %d, min %d, max %d", stream , indexMin, indexMax);
    if (indexMin < 0 || indexMin >= indexMax) {
        ALOGW("initStreamVolume() invalid index limits for stream %d, min %d, max %d", stream , indexMin, indexMax);
        return;
    }
    mStreams[stream].mIndexMin = indexMin;
    mStreams[stream].mIndexMax = indexMax;
}

status_t AudioPolicyManagerBase::setStreamVolumeIndex(AudioSystem::stream_type stream,
                                                      int index,
                                                      audio_devices_t device)
{

    if ((index < mStreams[stream].mIndexMin) || (index > mStreams[stream].mIndexMax)) {
        return BAD_VALUE;
    }
    if (!audio_is_output_device(device)) {
        return BAD_VALUE;
    }

    // Force max volume if stream cannot be muted
    if (!mStreams[stream].mCanBeMuted) index = mStreams[stream].mIndexMax;

    ALOGV("setStreamVolumeIndex() stream %d, device %04x, index %d",
          stream, device, index);

    // if device is AUDIO_DEVICE_OUT_DEFAULT set default value and
    // clear all device specific values
    if (device == AUDIO_DEVICE_OUT_DEFAULT) {
        mStreams[stream].mIndexCur.clear();
    }
    mStreams[stream].mIndexCur.add(device, index);

    // compute and apply stream volume on all outputs according to connected device
    status_t status = NO_ERROR;
    for (size_t i = 0; i < mOutputs.size(); i++) {
        audio_devices_t curDevice =
                getDeviceForVolume(mOutputs.valueAt(i)->device());
#ifndef ICS_AUDIO_BLOB
        if (device == curDevice)
#endif
        {
            status_t volStatus = checkAndSetVolume(stream, index, mOutputs.keyAt(i), curDevice);
            if (volStatus != NO_ERROR) {
                status = volStatus;
            }
        }
    }
    return status;
}

status_t AudioPolicyManagerBase::getStreamVolumeIndex(AudioSystem::stream_type stream,
                                                      int *index,
                                                      audio_devices_t device)
{
    if (index == NULL) {
        return BAD_VALUE;
    }
#ifndef ICS_AUDIO_BLOB
    if (!audio_is_output_device(device)) {
        return BAD_VALUE;
    }
    // if device is AUDIO_DEVICE_OUT_DEFAULT, return volume for device corresponding to
    // the strategy the stream belongs to.
    if (device == AUDIO_DEVICE_OUT_DEFAULT) {
        device = getDeviceForStrategy(getStrategy(stream), true /*fromCache*/);
    }
    device = getDeviceForVolume(device);

    *index =  mStreams[stream].getVolumeIndex(device);
#else
    *index =  mStreams[stream].mIndexCur.valueAt(0);
#endif
    ALOGV("getStreamVolumeIndex() stream %d device %08x index %d", stream, device, *index);
    return NO_ERROR;
}

audio_io_handle_t AudioPolicyManagerBase::getOutputForEffect(const effect_descriptor_t *desc)
{
    ALOGV("getOutputForEffect()");
    // apply simple rule where global effects are attached to the same output as MUSIC streams

    routing_strategy strategy = getStrategy(AudioSystem::MUSIC);
    audio_devices_t device = getDeviceForStrategy(strategy, false /*fromCache*/);
    SortedVector<audio_io_handle_t> dstOutputs = getOutputsForDevice(device, mOutputs);
    int outIdx = 0;
    for (size_t i = 0; i < dstOutputs.size(); i++) {
        AudioOutputDescriptor *desc = mOutputs.valueFor(dstOutputs[i]);
        if (desc->mFlags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
            outIdx = i;
        }
    }
    return dstOutputs[outIdx];
}

status_t AudioPolicyManagerBase::registerEffect(const effect_descriptor_t *desc,
                                audio_io_handle_t io,
                                uint32_t strategy,
                                int session,
                                int id)
{
    ssize_t index = mOutputs.indexOfKey(io);
    if (index < 0) {
        index = mInputs.indexOfKey(io);
        if (index < 0) {
            ALOGW("registerEffect() unknown io %d", io);
            return INVALID_OPERATION;
        }
    }

    if (mTotalEffectsMemory + desc->memoryUsage > getMaxEffectsMemory()) {
        ALOGW("registerEffect() memory limit exceeded for Fx %s, Memory %d KB",
                desc->name, desc->memoryUsage);
        return INVALID_OPERATION;
    }
    mTotalEffectsMemory += desc->memoryUsage;
    ALOGV("registerEffect() effect %s, io %d, strategy %d session %d id %d",
            desc->name, io, strategy, session, id);
    ALOGV("registerEffect() memory %d, total memory %d", desc->memoryUsage, mTotalEffectsMemory);

    EffectDescriptor *pDesc = new EffectDescriptor();
    memcpy (&pDesc->mDesc, desc, sizeof(effect_descriptor_t));
    pDesc->mIo = io;
    pDesc->mStrategy = (routing_strategy)strategy;
    pDesc->mSession = session;
    pDesc->mEnabled = false;

    mEffects.add(id, pDesc);

    return NO_ERROR;
}

status_t AudioPolicyManagerBase::unregisterEffect(int id)
{
    ssize_t index = mEffects.indexOfKey(id);
    if (index < 0) {
        ALOGW("unregisterEffect() unknown effect ID %d", id);
        return INVALID_OPERATION;
    }

    EffectDescriptor *pDesc = mEffects.valueAt(index);

    setEffectEnabled(pDesc, false);

    if (mTotalEffectsMemory < pDesc->mDesc.memoryUsage) {
        ALOGW("unregisterEffect() memory %d too big for total %d",
                pDesc->mDesc.memoryUsage, mTotalEffectsMemory);
        pDesc->mDesc.memoryUsage = mTotalEffectsMemory;
    }
    mTotalEffectsMemory -= pDesc->mDesc.memoryUsage;
    ALOGV("unregisterEffect() effect %s, ID %d, memory %d total memory %d",
            pDesc->mDesc.name, id, pDesc->mDesc.memoryUsage, mTotalEffectsMemory);

    mEffects.removeItem(id);
    delete pDesc;

    return NO_ERROR;
}

status_t AudioPolicyManagerBase::setEffectEnabled(int id, bool enabled)
{
    ssize_t index = mEffects.indexOfKey(id);
    if (index < 0) {
        ALOGW("unregisterEffect() unknown effect ID %d", id);
        return INVALID_OPERATION;
    }

    return setEffectEnabled(mEffects.valueAt(index), enabled);
}

status_t AudioPolicyManagerBase::setEffectEnabled(EffectDescriptor *pDesc, bool enabled)
{
    if (enabled == pDesc->mEnabled) {
        ALOGV("setEffectEnabled(%s) effect already %s",
             enabled?"true":"false", enabled?"enabled":"disabled");
        return INVALID_OPERATION;
    }

    if (enabled) {
        if (mTotalEffectsCpuLoad + pDesc->mDesc.cpuLoad > getMaxEffectsCpuLoad()) {
            ALOGW("setEffectEnabled(true) CPU Load limit exceeded for Fx %s, CPU %f MIPS",
                 pDesc->mDesc.name, (float)pDesc->mDesc.cpuLoad/10);
            return INVALID_OPERATION;
        }
        mTotalEffectsCpuLoad += pDesc->mDesc.cpuLoad;
        ALOGV("setEffectEnabled(true) total CPU %d", mTotalEffectsCpuLoad);
    } else {
        if (mTotalEffectsCpuLoad < pDesc->mDesc.cpuLoad) {
            ALOGW("setEffectEnabled(false) CPU load %d too high for total %d",
                    pDesc->mDesc.cpuLoad, mTotalEffectsCpuLoad);
            pDesc->mDesc.cpuLoad = mTotalEffectsCpuLoad;
        }
        mTotalEffectsCpuLoad -= pDesc->mDesc.cpuLoad;
        ALOGV("setEffectEnabled(false) total CPU %d", mTotalEffectsCpuLoad);
    }
    pDesc->mEnabled = enabled;
    return NO_ERROR;
}

bool AudioPolicyManagerBase::isStreamActive(int stream, uint32_t inPastMs) const
{
    nsecs_t sysTime = systemTime();
    for (size_t i = 0; i < mOutputs.size(); i++) {
        if (mOutputs.valueAt(i)->mRefCount[stream] != 0 ||
            ns2ms(sysTime - mOutputs.valueAt(i)->mStopTime[stream]) < inPastMs) {
            return true;
        }
    }
    return false;
}

bool AudioPolicyManagerBase::isSourceActive(audio_source_t source) const
{
    for (size_t i = 0; i < mInputs.size(); i++) {
        const AudioInputDescriptor * inputDescriptor = mInputs.valueAt(i);
        if ((inputDescriptor->mInputSource == (int) source)
                && (inputDescriptor->mRefCount > 0)) {
            return true;
        }
    }
    return false;
}



status_t AudioPolicyManagerBase::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "\nAudioPolicyManager Dump: %p\n", this);
    result.append(buffer);

    snprintf(buffer, SIZE, " Primary Output: %d\n", mPrimaryOutput);
    result.append(buffer);
    snprintf(buffer, SIZE, " A2DP device address: %s\n", mA2dpDeviceAddress.string());
    result.append(buffer);
    snprintf(buffer, SIZE, " SCO device address: %s\n", mScoDeviceAddress.string());
    result.append(buffer);
    snprintf(buffer, SIZE, " USB audio ALSA %s\n", mUsbCardAndDevice.string());
    result.append(buffer);
    snprintf(buffer, SIZE, " Output devices: %08x\n", mAvailableOutputDevices);
    result.append(buffer);
    snprintf(buffer, SIZE, " Input devices: %08x\n", mAvailableInputDevices);
    result.append(buffer);
    snprintf(buffer, SIZE, " Phone state: %d\n", mPhoneState);
    result.append(buffer);
    snprintf(buffer, SIZE, " Force use for communications %d\n", mForceUse[AudioSystem::FOR_COMMUNICATION]);
    result.append(buffer);
    snprintf(buffer, SIZE, " Force use for media %d\n", mForceUse[AudioSystem::FOR_MEDIA]);
    result.append(buffer);
    snprintf(buffer, SIZE, " Force use for record %d\n", mForceUse[AudioSystem::FOR_RECORD]);
    result.append(buffer);
    snprintf(buffer, SIZE, " Force use for dock %d\n", mForceUse[AudioSystem::FOR_DOCK]);
    result.append(buffer);
    snprintf(buffer, SIZE, " Force use for system %d\n", mForceUse[AudioSystem::FOR_SYSTEM]);
    result.append(buffer);
    write(fd, result.string(), result.size());


    snprintf(buffer, SIZE, "\nHW Modules dump:\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < mHwModules.size(); i++) {
        snprintf(buffer, SIZE, "- HW Module %d:\n", i + 1);
        write(fd, buffer, strlen(buffer));
        mHwModules[i]->dump(fd);
    }

    snprintf(buffer, SIZE, "\nOutputs dump:\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < mOutputs.size(); i++) {
        snprintf(buffer, SIZE, "- Output %d dump:\n", mOutputs.keyAt(i));
        write(fd, buffer, strlen(buffer));
        mOutputs.valueAt(i)->dump(fd);
    }

    snprintf(buffer, SIZE, "\nInputs dump:\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < mInputs.size(); i++) {
        snprintf(buffer, SIZE, "- Input %d dump:\n", mInputs.keyAt(i));
        write(fd, buffer, strlen(buffer));
        mInputs.valueAt(i)->dump(fd);
    }

    snprintf(buffer, SIZE, "\nStreams dump:\n");
    write(fd, buffer, strlen(buffer));
    snprintf(buffer, SIZE,
             " Stream  Can be muted  Index Min  Index Max  Index Cur [device : index]...\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
        snprintf(buffer, SIZE, " %02d      ", i);
        write(fd, buffer, strlen(buffer));
        mStreams[i].dump(fd);
    }

    snprintf(buffer, SIZE, "\nTotal Effects CPU: %f MIPS, Total Effects memory: %d KB\n",
            (float)mTotalEffectsCpuLoad/10, mTotalEffectsMemory);
    write(fd, buffer, strlen(buffer));

    snprintf(buffer, SIZE, "Registered effects:\n");
    write(fd, buffer, strlen(buffer));
    for (size_t i = 0; i < mEffects.size(); i++) {
        snprintf(buffer, SIZE, "- Effect %d dump:\n", mEffects.keyAt(i));
        write(fd, buffer, strlen(buffer));
        mEffects.valueAt(i)->dump(fd);
    }


    return NO_ERROR;
}

// ----------------------------------------------------------------------------
// AudioPolicyManagerBase
// ----------------------------------------------------------------------------

AudioPolicyManagerBase::AudioPolicyManagerBase(AudioPolicyClientInterface *clientInterface)
    :
#ifdef AUDIO_POLICY_TEST
    Thread(false),
#endif //AUDIO_POLICY_TEST
    mPrimaryOutput((audio_io_handle_t)0),
    mAvailableOutputDevices(AUDIO_DEVICE_NONE),
    mPhoneState(AudioSystem::MODE_NORMAL),
    mLimitRingtoneVolume(false), mLastVoiceVolume(-1.0f),
    mTotalEffectsCpuLoad(0), mTotalEffectsMemory(0),
    mA2dpSuspended(false), mHasA2dp(false), mHasUsb(false), mHasRemoteSubmix(false)
{
    mpClientInterface = clientInterface;

    for (int i = 0; i < AudioSystem::NUM_FORCE_USE; i++) {
        mForceUse[i] = AudioSystem::FORCE_NONE;
    }

    initializeVolumeCurves();

    mA2dpDeviceAddress = String8("");
    mScoDeviceAddress = String8("");
    mUsbCardAndDevice = String8("");

    if (loadAudioPolicyConfig(AUDIO_POLICY_VENDOR_CONFIG_FILE) != NO_ERROR) {
        if (loadAudioPolicyConfig(AUDIO_POLICY_CONFIG_FILE) != NO_ERROR) {
            ALOGE("could not load audio policy configuration file, setting defaults");
            defaultAudioPolicyConfig();
        }
    }

    // open all output streams needed to access attached devices
    for (size_t i = 0; i < mHwModules.size(); i++) {
        mHwModules[i]->mHandle = mpClientInterface->loadHwModule(mHwModules[i]->mName);
        if (mHwModules[i]->mHandle == 0) {
            ALOGW("could not open HW module %s", mHwModules[i]->mName);
            continue;
        }
        // open all output streams needed to access attached devices
        for (size_t j = 0; j < mHwModules[i]->mOutputProfiles.size(); j++)
        {
            const IOProfile *outProfile = mHwModules[i]->mOutputProfiles[j];
#ifdef QCOM_HARDWARE
            if ( (outProfile->mSupportedDevices & mAttachedOutputDevices) &&
                  !(outProfile->mFlags & AUDIO_OUTPUT_FLAG_DIRECT) )
#else
            if (outProfile->mSupportedDevices & mAttachedOutputDevices)
#endif
            {
                AudioOutputDescriptor *outputDesc = new AudioOutputDescriptor(outProfile);
                outputDesc->mDevice = (audio_devices_t)(mDefaultOutputDevice &
                                                            outProfile->mSupportedDevices);
                audio_io_handle_t output = mpClientInterface->openOutput(
                                                outProfile->mModule->mHandle,
                                                &outputDesc->mDevice,
                                                &outputDesc->mSamplingRate,
                                                &outputDesc->mFormat,
                                                &outputDesc->mChannelMask,
                                                &outputDesc->mLatency,
                                                outputDesc->mFlags);
                if (output == 0) {
                    delete outputDesc;
                } else {
                    mAvailableOutputDevices = (audio_devices_t)(mAvailableOutputDevices |
                                            (outProfile->mSupportedDevices & mAttachedOutputDevices));
                    if (mPrimaryOutput == 0 &&
                            outProfile->mFlags & AUDIO_OUTPUT_FLAG_PRIMARY) {
                        mPrimaryOutput = output;
                    }
                    addOutput(output, outputDesc);
                    setOutputDevice(output,
                                    (audio_devices_t)(mDefaultOutputDevice &
                                                        outProfile->mSupportedDevices),
                                    true);
                }
            }
        }
    }

    ALOGE_IF((mAttachedOutputDevices & ~mAvailableOutputDevices),
             "Not output found for attached devices %08x",
             (mAttachedOutputDevices & ~mAvailableOutputDevices));

    ALOGE_IF((mPrimaryOutput == 0), "Failed to open primary output");

    updateDevicesAndOutputs();

#ifdef AUDIO_POLICY_TEST
    if (mPrimaryOutput != 0) {
        AudioParameter outputCmd = AudioParameter();
        outputCmd.addInt(String8("set_id"), 0);
        mpClientInterface->setParameters(mPrimaryOutput, outputCmd.toString());

        mTestDevice = AUDIO_DEVICE_OUT_SPEAKER;
        mTestSamplingRate = 44100;
        mTestFormat = AudioSystem::PCM_16_BIT;
        mTestChannels =  AudioSystem::CHANNEL_OUT_STEREO;
        mTestLatencyMs = 0;
        mCurOutput = 0;
        mDirectOutput = false;
        for (int i = 0; i < NUM_TEST_OUTPUTS; i++) {
            mTestOutputs[i] = 0;
        }

        const size_t SIZE = 256;
        char buffer[SIZE];
        snprintf(buffer, SIZE, "AudioPolicyManagerTest");
        run(buffer, ANDROID_PRIORITY_AUDIO);
    }
#endif //AUDIO_POLICY_TEST
}

AudioPolicyManagerBase::~AudioPolicyManagerBase()
{
#ifdef AUDIO_POLICY_TEST
    exit();
#endif //AUDIO_POLICY_TEST
   for (size_t i = 0; i < mOutputs.size(); i++) {
        mpClientInterface->closeOutput(mOutputs.keyAt(i));
        delete mOutputs.valueAt(i);
   }
   for (size_t i = 0; i < mInputs.size(); i++) {
        mpClientInterface->closeInput(mInputs.keyAt(i));
        delete mInputs.valueAt(i);
   }
   for (size_t i = 0; i < mHwModules.size(); i++) {
        delete mHwModules[i];
   }
}

status_t AudioPolicyManagerBase::initCheck()
{
    return (mPrimaryOutput == 0) ? NO_INIT : NO_ERROR;
}

#ifdef AUDIO_POLICY_TEST
bool AudioPolicyManagerBase::threadLoop()
{
    ALOGV("entering threadLoop()");
    while (!exitPending())
    {
        String8 command;
        int valueInt;
        String8 value;

        Mutex::Autolock _l(mLock);
        mWaitWorkCV.waitRelative(mLock, milliseconds(50));

        command = mpClientInterface->getParameters(0, String8("test_cmd_policy"));
        AudioParameter param = AudioParameter(command);

        if (param.getInt(String8("test_cmd_policy"), valueInt) == NO_ERROR &&
            valueInt != 0) {
            ALOGV("Test command %s received", command.string());
            String8 target;
            if (param.get(String8("target"), target) != NO_ERROR) {
                target = "Manager";
            }
            if (param.getInt(String8("test_cmd_policy_output"), valueInt) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_output"));
                mCurOutput = valueInt;
            }
            if (param.get(String8("test_cmd_policy_direct"), value) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_direct"));
                if (value == "false") {
                    mDirectOutput = false;
                } else if (value == "true") {
                    mDirectOutput = true;
                }
            }
            if (param.getInt(String8("test_cmd_policy_input"), valueInt) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_input"));
                mTestInput = valueInt;
            }

            if (param.get(String8("test_cmd_policy_format"), value) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_format"));
                int format = AudioSystem::INVALID_FORMAT;
                if (value == "PCM 16 bits") {
                    format = AudioSystem::PCM_16_BIT;
                } else if (value == "PCM 8 bits") {
                    format = AudioSystem::PCM_8_BIT;
                } else if (value == "Compressed MP3") {
                    format = AudioSystem::MP3;
                }
                if (format != AudioSystem::INVALID_FORMAT) {
                    if (target == "Manager") {
                        mTestFormat = format;
                    } else if (mTestOutputs[mCurOutput] != 0) {
                        AudioParameter outputParam = AudioParameter();
                        outputParam.addInt(String8("format"), format);
                        mpClientInterface->setParameters(mTestOutputs[mCurOutput], outputParam.toString());
                    }
                }
            }
            if (param.get(String8("test_cmd_policy_channels"), value) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_channels"));
                int channels = 0;

                if (value == "Channels Stereo") {
                    channels =  AudioSystem::CHANNEL_OUT_STEREO;
                } else if (value == "Channels Mono") {
                    channels =  AudioSystem::CHANNEL_OUT_MONO;
                }
                if (channels != 0) {
                    if (target == "Manager") {
                        mTestChannels = channels;
                    } else if (mTestOutputs[mCurOutput] != 0) {
                        AudioParameter outputParam = AudioParameter();
                        outputParam.addInt(String8("channels"), channels);
                        mpClientInterface->setParameters(mTestOutputs[mCurOutput], outputParam.toString());
                    }
                }
            }
            if (param.getInt(String8("test_cmd_policy_sampleRate"), valueInt) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_sampleRate"));
                if (valueInt >= 0 && valueInt <= 96000) {
                    int samplingRate = valueInt;
                    if (target == "Manager") {
                        mTestSamplingRate = samplingRate;
                    } else if (mTestOutputs[mCurOutput] != 0) {
                        AudioParameter outputParam = AudioParameter();
                        outputParam.addInt(String8("sampling_rate"), samplingRate);
                        mpClientInterface->setParameters(mTestOutputs[mCurOutput], outputParam.toString());
                    }
                }
            }

            if (param.get(String8("test_cmd_policy_reopen"), value) == NO_ERROR) {
                param.remove(String8("test_cmd_policy_reopen"));

                AudioOutputDescriptor *outputDesc = mOutputs.valueFor(mPrimaryOutput);
                mpClientInterface->closeOutput(mPrimaryOutput);

                audio_module_handle_t moduleHandle = outputDesc->mModule->mHandle;

                delete mOutputs.valueFor(mPrimaryOutput);
                mOutputs.removeItem(mPrimaryOutput);

                AudioOutputDescriptor *outputDesc = new AudioOutputDescriptor(NULL);
                outputDesc->mDevice = AUDIO_DEVICE_OUT_SPEAKER;
                mPrimaryOutput = mpClientInterface->openOutput(moduleHandle,
                                                &outputDesc->mDevice,
                                                &outputDesc->mSamplingRate,
                                                &outputDesc->mFormat,
                                                &outputDesc->mChannelMask,
                                                &outputDesc->mLatency,
                                                outputDesc->mFlags);
                if (mPrimaryOutput == 0) {
                    ALOGE("Failed to reopen hardware output stream, samplingRate: %d, format %d, channels %d",
                            outputDesc->mSamplingRate, outputDesc->mFormat, outputDesc->mChannelMask);
                } else {
                    AudioParameter outputCmd = AudioParameter();
                    outputCmd.addInt(String8("set_id"), 0);
                    mpClientInterface->setParameters(mPrimaryOutput, outputCmd.toString());
                    addOutput(mPrimaryOutput, outputDesc);
                }
            }


            mpClientInterface->setParameters(0, String8("test_cmd_policy="));
        }
    }
    return false;
}

void AudioPolicyManagerBase::exit()
{
    {
        AutoMutex _l(mLock);
        requestExit();
        mWaitWorkCV.signal();
    }
    requestExitAndWait();
}

int AudioPolicyManagerBase::testOutputIndex(audio_io_handle_t output)
{
    for (int i = 0; i < NUM_TEST_OUTPUTS; i++) {
        if (output == mTestOutputs[i]) return i;
    }
    return 0;
}
#endif //AUDIO_POLICY_TEST

// ---

void AudioPolicyManagerBase::addOutput(audio_io_handle_t id, AudioOutputDescriptor *outputDesc)
{
    outputDesc->mId = id;
    mOutputs.add(id, outputDesc);
}


status_t AudioPolicyManagerBase::checkOutputsForDevice(audio_devices_t device,
                                                       AudioSystem::device_connection_state state,
                                                       SortedVector<audio_io_handle_t>& outputs)
{
    AudioOutputDescriptor *desc;

    if (state == AudioSystem::DEVICE_STATE_AVAILABLE) {
        // first list already open outputs that can be routed to this device
        for (size_t i = 0; i < mOutputs.size(); i++) {
            desc = mOutputs.valueAt(i);
            if (!desc->isDuplicated() && (desc->mProfile->mSupportedDevices & device)) {
                ALOGV("checkOutputsForDevice(): adding opened output %d", mOutputs.keyAt(i));
                outputs.add(mOutputs.keyAt(i));
            }
        }
        // then look for output profiles that can be routed to this device
        SortedVector<IOProfile *> profiles;
        for (size_t i = 0; i < mHwModules.size(); i++)
        {
            if (mHwModules[i]->mHandle == 0) {
                continue;
            }
            for (size_t j = 0; j < mHwModules[i]->mOutputProfiles.size(); j++)
            {
                if (mHwModules[i]->mOutputProfiles[j]->mSupportedDevices & device) {
                    ALOGV("checkOutputsForDevice(): adding profile %d from module %d", j, i);
                    profiles.add(mHwModules[i]->mOutputProfiles[j]);
                }
            }
        }

        if (profiles.isEmpty() && outputs.isEmpty()) {
            ALOGW("checkOutputsForDevice(): No output available for device %04x", device);
            return BAD_VALUE;
        }

        // open outputs for matching profiles if needed. Direct outputs are also opened to
        // query for dynamic parameters and will be closed later by setDeviceConnectionState()
        for (ssize_t profile_index = 0; profile_index < (ssize_t)profiles.size(); profile_index++) {
            IOProfile *profile = profiles[profile_index];

            // nothing to do if one output is already opened for this profile
            size_t j;
            for (j = 0; j < mOutputs.size(); j++) {
                desc = mOutputs.valueAt(j);
                if (!desc->isDuplicated() && desc->mProfile == profile) {
                    break;
                }
            }
            if (j != mOutputs.size()) {
                continue;
            }

            ALOGV("opening output for device %08x", device);
            desc = new AudioOutputDescriptor(profile);
            desc->mDevice = device;
            audio_io_handle_t output = 0;
#ifdef QCOM_HARDWARE
            if (!(desc->mFlags & AUDIO_OUTPUT_FLAG_LPA || desc->mFlags & AUDIO_OUTPUT_FLAG_TUNNEL ||
                desc->mFlags & AUDIO_OUTPUT_FLAG_VOIP_RX)) {
#endif
                output =  mpClientInterface->openOutput(profile->mModule->mHandle,
                                                        &desc->mDevice,
                                                        &desc->mSamplingRate,
                                                        &desc->mFormat,
                                                        &desc->mChannelMask,
                                                        &desc->mLatency,
                                                        desc->mFlags);
#ifdef QCOM_HARDWARE
            }
#endif
            if (output != 0) {
                if (desc->mFlags & AUDIO_OUTPUT_FLAG_DIRECT) {
                    String8 reply;
                    char *value;
                    if (profile->mSamplingRates[0] == 0) {
                        reply = mpClientInterface->getParameters(output,
                                                String8(AUDIO_PARAMETER_STREAM_SUP_SAMPLING_RATES));
                        ALOGV("checkOutputsForDevice() direct output sup sampling rates %s",
                                  reply.string());
                        value = strpbrk((char *)reply.string(), "=");
                        if (value != NULL) {
                            loadSamplingRates(value + 1, profile);
                        }
                    }
                    if (profile->mFormats[0] == 0) {
                        reply = mpClientInterface->getParameters(output,
                                                       String8(AUDIO_PARAMETER_STREAM_SUP_FORMATS));
                        ALOGV("checkOutputsForDevice() direct output sup formats %s",
                                  reply.string());
                        value = strpbrk((char *)reply.string(), "=");
                        if (value != NULL) {
                            loadFormats(value + 1, profile);
                        }
                    }
                    if (profile->mChannelMasks[0] == 0) {
                        reply = mpClientInterface->getParameters(output,
                                                      String8(AUDIO_PARAMETER_STREAM_SUP_CHANNELS));
                        ALOGV("checkOutputsForDevice() direct output sup channel masks %s",
                                  reply.string());
                        value = strpbrk((char *)reply.string(), "=");
                        if (value != NULL) {
                            loadOutChannels(value + 1, profile);
                        }
                    }
                    if (((profile->mSamplingRates[0] == 0) &&
                             (profile->mSamplingRates.size() < 2)) ||
                         ((profile->mFormats[0] == 0) &&
                             (profile->mFormats.size() < 2)) ||
                         ((profile->mFormats[0] == 0) &&
                             (profile->mChannelMasks.size() < 2))) {
                        ALOGW("checkOutputsForDevice() direct output missing param");
                        mpClientInterface->closeOutput(output);
                        output = 0;
                    } else {
                        addOutput(output, desc);
                    }
                } else {
                    audio_io_handle_t duplicatedOutput = 0;
                    // add output descriptor
                    addOutput(output, desc);
                    // set initial stream volume for device
                    applyStreamVolumes(output, device, 0, true);

                    //TODO: configure audio effect output stage here

                    // open a duplicating output thread for the new output and the primary output
                    duplicatedOutput = mpClientInterface->openDuplicateOutput(output,
                                                                              mPrimaryOutput);
                    if (duplicatedOutput != 0) {
                        // add duplicated output descriptor
                        AudioOutputDescriptor *dupOutputDesc = new AudioOutputDescriptor(NULL);
                        dupOutputDesc->mOutput1 = mOutputs.valueFor(mPrimaryOutput);
                        dupOutputDesc->mOutput2 = mOutputs.valueFor(output);
                        dupOutputDesc->mSamplingRate = desc->mSamplingRate;
                        dupOutputDesc->mFormat = desc->mFormat;
                        dupOutputDesc->mChannelMask = desc->mChannelMask;
                        dupOutputDesc->mLatency = desc->mLatency;
                        addOutput(duplicatedOutput, dupOutputDesc);
                        applyStreamVolumes(duplicatedOutput, device, 0, true);
                    } else {
                        ALOGW("checkOutputsForDevice() could not open dup output for %d and %d",
                                mPrimaryOutput, output);
                        mpClientInterface->closeOutput(output);
                        mOutputs.removeItem(output);
                        output = 0;
                    }
                }
            }
            if (output == 0) {
                ALOGW("checkOutputsForDevice() could not open output for device %x", device);
                delete desc;
                profiles.removeAt(profile_index);
                profile_index--;
            } else {
                outputs.add(output);
                ALOGV("checkOutputsForDevice(): adding output %d", output);
            }
        }

        if (profiles.isEmpty()) {
            ALOGW("checkOutputsForDevice(): No output available for device %04x", device);
            return BAD_VALUE;
        }
    } else {
        // check if one opened output is not needed any more after disconnecting one device
        for (size_t i = 0; i < mOutputs.size(); i++) {
            desc = mOutputs.valueAt(i);
            if (!desc->isDuplicated() &&
                    !(desc->mProfile->mSupportedDevices & mAvailableOutputDevices)) {
                ALOGV("checkOutputsForDevice(): disconnecting adding output %d", mOutputs.keyAt(i));
                outputs.add(mOutputs.keyAt(i));
            }
        }
        for (size_t i = 0; i < mHwModules.size(); i++)
        {
            if (mHwModules[i]->mHandle == 0) {
                continue;
            }
            for (size_t j = 0; j < mHwModules[i]->mOutputProfiles.size(); j++)
            {
                IOProfile *profile = mHwModules[i]->mOutputProfiles[j];
                if ((profile->mSupportedDevices & device) &&
                        (profile->mFlags & AUDIO_OUTPUT_FLAG_DIRECT)) {
                    ALOGV("checkOutputsForDevice(): clearing direct output profile %d on module %d",
                          j, i);
                    if (profile->mSamplingRates[0] == 0) {
                        profile->mSamplingRates.clear();
                        profile->mSamplingRates.add(0);
                    }
                    if (profile->mFormats[0] == 0) {
                        profile->mFormats.clear();
                        profile->mFormats.add((audio_format_t)0);
                    }
                    if (profile->mChannelMasks[0] == 0) {
                        profile->mChannelMasks.clear();
                        profile->mChannelMasks.add((audio_channel_mask_t)0);
                    }
                }
            }
        }
    }
    return NO_ERROR;
}

void AudioPolicyManagerBase::closeOutput(audio_io_handle_t output)
{
    ALOGV("closeOutput(%d)", output);

    AudioOutputDescriptor *outputDesc = mOutputs.valueFor(output);
    if (outputDesc == NULL) {
        ALOGW("closeOutput() unknown output %d", output);
        return;
    }

    // look for duplicated outputs connected to the output being removed.
    for (size_t i = 0; i < mOutputs.size(); i++) {
        AudioOutputDescriptor *dupOutputDesc = mOutputs.valueAt(i);
        if (dupOutputDesc->isDuplicated() &&
                (dupOutputDesc->mOutput1 == outputDesc ||
                dupOutputDesc->mOutput2 == outputDesc)) {
            AudioOutputDescriptor *outputDesc2;
            if (dupOutputDesc->mOutput1 == outputDesc) {
                outputDesc2 = dupOutputDesc->mOutput2;
            } else {
                outputDesc2 = dupOutputDesc->mOutput1;
            }
            // As all active tracks on duplicated output will be deleted,
            // and as they were also referenced on the other output, the reference
            // count for their stream type must be adjusted accordingly on
            // the other output.
            for (int j = 0; j < (int)AudioSystem::NUM_STREAM_TYPES; j++) {
                int refCount = dupOutputDesc->mRefCount[j];
                outputDesc2->changeRefCount((AudioSystem::stream_type)j,-refCount);
            }
            audio_io_handle_t duplicatedOutput = mOutputs.keyAt(i);
            ALOGV("closeOutput() closing also duplicated output %d", duplicatedOutput);

            mpClientInterface->closeOutput(duplicatedOutput);
            delete mOutputs.valueFor(duplicatedOutput);
            mOutputs.removeItem(duplicatedOutput);
        }
    }

    AudioParameter param;
    param.add(String8("closing"), String8("true"));
    mpClientInterface->setParameters(output, param.toString());

    mpClientInterface->closeOutput(output);
    delete mOutputs.valueFor(output);
    mOutputs.removeItem(output);
}

SortedVector<audio_io_handle_t> AudioPolicyManagerBase::getOutputsForDevice(audio_devices_t device,
                        DefaultKeyedVector<audio_io_handle_t, AudioOutputDescriptor *> openOutputs)
{
    SortedVector<audio_io_handle_t> outputs;

    ALOGVV("getOutputsForDevice() device %04x", device);
    for (size_t i = 0; i < openOutputs.size(); i++) {
        ALOGVV("output %d isDuplicated=%d device=%04x",
                i, openOutputs.valueAt(i)->isDuplicated(), openOutputs.valueAt(i)->supportedDevices());
        if ((device & openOutputs.valueAt(i)->supportedDevices()) == device) {
            ALOGVV("getOutputsForDevice() found output %d", openOutputs.keyAt(i));
            outputs.add(openOutputs.keyAt(i));
        }
    }
    return outputs;
}

bool AudioPolicyManagerBase::vectorsEqual(SortedVector<audio_io_handle_t>& outputs1,
                                   SortedVector<audio_io_handle_t>& outputs2)
{
    if (outputs1.size() != outputs2.size()) {
        return false;
    }
    for (size_t i = 0; i < outputs1.size(); i++) {
        if (outputs1[i] != outputs2[i]) {
            return false;
        }
    }
    return true;
}

void AudioPolicyManagerBase::checkOutputForStrategy(routing_strategy strategy)
{
    audio_devices_t oldDevice = getDeviceForStrategy(strategy, true /*fromCache*/);
    audio_devices_t newDevice = getDeviceForStrategy(strategy, false /*fromCache*/);
    SortedVector<audio_io_handle_t> srcOutputs = getOutputsForDevice(oldDevice, mPreviousOutputs);
    SortedVector<audio_io_handle_t> dstOutputs = getOutputsForDevice(newDevice, mOutputs);

    if (!vectorsEqual(srcOutputs,dstOutputs)) {
        ALOGV("checkOutputForStrategy() strategy %d, moving from output %d to output %d",
              strategy, srcOutputs[0], dstOutputs[0]);
        // mute strategy while moving tracks from one output to another
        for (size_t i = 0; i < srcOutputs.size(); i++) {
            AudioOutputDescriptor *desc = mOutputs.valueFor(srcOutputs[i]);
            if (desc->strategyRefCount(strategy) != 0) {
                setStrategyMute(strategy, true, srcOutputs[i]);
                setStrategyMute(strategy, false, srcOutputs[i], MUTE_TIME_MS, newDevice);
            }
        }

        // Move effects associated to this strategy from previous output to new output
        if (strategy == STRATEGY_MEDIA) {
            int outIdx = 0;
            for (size_t i = 0; i < dstOutputs.size(); i++) {
                AudioOutputDescriptor *desc = mOutputs.valueFor(dstOutputs[i]);
                if (desc->mFlags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) {
                    outIdx = i;
                }
            }
            SortedVector<audio_io_handle_t> moved;
            for (size_t i = 0; i < mEffects.size(); i++) {
                EffectDescriptor *desc = mEffects.valueAt(i);
                if (desc->mSession == AUDIO_SESSION_OUTPUT_MIX &&
                        desc->mIo != dstOutputs[outIdx]) {
                    if (moved.indexOf(desc->mIo) < 0) {
                        ALOGV("checkOutputForStrategy() moving effect %d to output %d",
                              mEffects.keyAt(i), dstOutputs[outIdx]);
                        mpClientInterface->moveEffects(AUDIO_SESSION_OUTPUT_MIX, desc->mIo,
                                                       dstOutputs[outIdx]);
                        moved.add(desc->mIo);
                    }
                    desc->mIo = dstOutputs[outIdx];
                }
            }
        }
        // Move tracks associated to this strategy from previous output to new output
        for (int i = 0; i < (int)AudioSystem::NUM_STREAM_TYPES; i++) {
            if (getStrategy((AudioSystem::stream_type)i) == strategy) {
                //FIXME see fixme on name change
                mpClientInterface->setStreamOutput((AudioSystem::stream_type)i,
                                                   dstOutputs[0] /* ignored */);
            }
        }
    }
}

void AudioPolicyManagerBase::checkOutputForAllStrategies()
{
    checkOutputForStrategy(STRATEGY_ENFORCED_AUDIBLE);
    checkOutputForStrategy(STRATEGY_PHONE);
    checkOutputForStrategy(STRATEGY_SONIFICATION);
    checkOutputForStrategy(STRATEGY_SONIFICATION_RESPECTFUL);
    checkOutputForStrategy(STRATEGY_MEDIA);
    checkOutputForStrategy(STRATEGY_DTMF);
}

audio_io_handle_t AudioPolicyManagerBase::getA2dpOutput()
{
    if (!mHasA2dp) {
        return 0;
    }

    for (size_t i = 0; i < mOutputs.size(); i++) {
        AudioOutputDescriptor *outputDesc = mOutputs.valueAt(i);
        if (!outputDesc->isDuplicated() && outputDesc->device() & AUDIO_DEVICE_OUT_ALL_A2DP) {
            return mOutputs.keyAt(i);
        }
    }

    return 0;
}

void AudioPolicyManagerBase::checkA2dpSuspend()
{
    if (!mHasA2dp) {
        return;
    }
    audio_io_handle_t a2dpOutput = getA2dpOutput();
    if (a2dpOutput == 0) {
        return;
    }

    // suspend A2DP output if:
    //      (NOT already suspended) &&
    //      ((SCO device is connected &&
    //       (forced usage for communication || for record is SCO))) ||
    //      (phone state is ringing || in call)
    //
    // restore A2DP output if:
    //      (Already suspended) &&
    //      ((SCO device is NOT connected ||
    //       (forced usage NOT for communication && NOT for record is SCO))) &&
    //      (phone state is NOT ringing && NOT in call)
    //
    if (mA2dpSuspended) {
        if (((mScoDeviceAddress == "") ||
             ((mForceUse[AudioSystem::FOR_COMMUNICATION] != AudioSystem::FORCE_BT_SCO) &&
              (mForceUse[AudioSystem::FOR_RECORD] != AudioSystem::FORCE_BT_SCO))) &&
             ((mPhoneState != AudioSystem::MODE_IN_CALL) &&
              (mPhoneState != AudioSystem::MODE_RINGTONE))) {

            mpClientInterface->restoreOutput(a2dpOutput);
            mA2dpSuspended = false;
        }
    } else {
        if (((mScoDeviceAddress != "") &&
             ((mForceUse[AudioSystem::FOR_COMMUNICATION] == AudioSystem::FORCE_BT_SCO) ||
              (mForceUse[AudioSystem::FOR_RECORD] == AudioSystem::FORCE_BT_SCO))) ||
             ((mPhoneState == AudioSystem::MODE_IN_CALL) ||
              (mPhoneState == AudioSystem::MODE_RINGTONE))) {

            mpClientInterface->suspendOutput(a2dpOutput);
            mA2dpSuspended = true;
        }
    }
}

audio_devices_t AudioPolicyManagerBase::getNewDevice(audio_io_handle_t output, bool fromCache)
{
    audio_devices_t device = AUDIO_DEVICE_NONE;

    AudioOutputDescriptor *outputDesc = mOutputs.valueFor(output);
    // check the following by order of priority to request a routing change if necessary:
    // 1: the strategy enforced audible is active on the output:
    //      use device for strategy enforced audible
    // 2: we are in call or the strategy phone is active on the output:
    //      use device for strategy phone
    // 3: the strategy sonification is active on the output:
    //      use device for strategy sonification
    // 4: the strategy "respectful" sonification is active on the output:
    //      use device for strategy "respectful" sonification
    // 5: the strategy media is active on the output:
    //      use device for strategy media
    // 6: the strategy DTMF is active on the output:
    //      use device for strategy DTMF
    if (outputDesc->isUsedByStrategy(STRATEGY_ENFORCED_AUDIBLE)) {
        device = getDeviceForStrategy(STRATEGY_ENFORCED_AUDIBLE, fromCache);
    } else if (isInCall() ||
                    outputDesc->isUsedByStrategy(STRATEGY_PHONE)) {
        device = getDeviceForStrategy(STRATEGY_PHONE, fromCache);
    } else if (outputDesc->isUsedByStrategy(STRATEGY_SONIFICATION)){
        device = getDeviceForStrategy(STRATEGY_SONIFICATION, fromCache);
    } else if (outputDesc->isUsedByStrategy(STRATEGY_SONIFICATION_RESPECTFUL)) {
        device = getDeviceForStrategy(STRATEGY_SONIFICATION_RESPECTFUL, fromCache);
    } else if (outputDesc->isUsedByStrategy(STRATEGY_MEDIA)) {
        device = getDeviceForStrategy(STRATEGY_MEDIA, fromCache);
    } else if (outputDesc->isUsedByStrategy(STRATEGY_DTMF)) {
        device = getDeviceForStrategy(STRATEGY_DTMF, fromCache);
    }

    ALOGV("getNewDevice() selected device %x", device);
    return device;
}

uint32_t AudioPolicyManagerBase::getStrategyForStream(AudioSystem::stream_type stream) {
    return (uint32_t)getStrategy(stream);
}

audio_devices_t AudioPolicyManagerBase::getDevicesForStream(AudioSystem::stream_type stream) {
    audio_devices_t devices;
    // By checking the range of stream before calling getStrategy, we avoid
    // getStrategy's behavior for invalid streams.  getStrategy would do a ALOGE
    // and then return STRATEGY_MEDIA, but we want to return the empty set.
    if (stream < (AudioSystem::stream_type) 0 || stream >= AudioSystem::NUM_STREAM_TYPES) {
        devices = AUDIO_DEVICE_NONE;
    } else {
        AudioPolicyManagerBase::routing_strategy strategy = getStrategy(stream);
        devices = getDeviceForStrategy(strategy, true /*fromCache*/);
    }
    return devices;
}

AudioPolicyManagerBase::routing_strategy AudioPolicyManagerBase::getStrategy(
        AudioSystem::stream_type stream) {
    // stream to strategy mapping
    switch (stream) {
    case AudioSystem::VOICE_CALL:
    case AudioSystem::BLUETOOTH_SCO:
        return STRATEGY_PHONE;
    case AudioSystem::RING:
    case AudioSystem::ALARM:
        return STRATEGY_SONIFICATION;
    case AudioSystem::NOTIFICATION:
        return STRATEGY_SONIFICATION_RESPECTFUL;
    case AudioSystem::DTMF:
        return STRATEGY_DTMF;
    default:
        ALOGE("unknown stream type");
    case AudioSystem::SYSTEM:
        // NOTE: SYSTEM stream uses MEDIA strategy because muting music and switching outputs
        // while key clicks are played produces a poor result
    case AudioSystem::TTS:
    case AudioSystem::MUSIC:
#ifdef QCOM_FM_ENABLED
    case AudioSystem::FM:
#endif
        return STRATEGY_MEDIA;
    case AudioSystem::ENFORCED_AUDIBLE:
        return STRATEGY_ENFORCED_AUDIBLE;
    }
}

void AudioPolicyManagerBase::handleNotificationRoutingForStream(AudioSystem::stream_type stream) {
    switch(stream) {
    case AudioSystem::MUSIC:
        checkOutputForStrategy(STRATEGY_SONIFICATION_RESPECTFUL);
        updateDevicesAndOutputs();
        break;
    default:
        break;
    }
}

audio_devices_t AudioPolicyManagerBase::getDeviceForStrategy(routing_strategy strategy,
                                                             bool fromCache)
{
    uint32_t device = AUDIO_DEVICE_NONE;

    if (fromCache) {
        ALOGVV("getDeviceForStrategy() from cache strategy %d, device %x",
              strategy, mDeviceForStrategy[strategy]);
        return mDeviceForStrategy[strategy];
    }

    switch (strategy) {

    case STRATEGY_SONIFICATION_RESPECTFUL:
        if (isInCall()) {
            device = getDeviceForStrategy(STRATEGY_SONIFICATION, false /*fromCache*/);
        } else if (isStreamActive(AudioSystem::MUSIC, SONIFICATION_RESPECTFUL_AFTER_MUSIC_DELAY)) {
            // while media is playing (or has recently played), use the same device
            device = getDeviceForStrategy(STRATEGY_MEDIA, false /*fromCache*/);
        } else {
            // when media is not playing anymore, fall back on the sonification behavior
            device = getDeviceForStrategy(STRATEGY_SONIFICATION, false /*fromCache*/);
        }

        break;

    case STRATEGY_DTMF:
        if (!isInCall()) {
            // when off call, DTMF strategy follows the same rules as MEDIA strategy
            device = getDeviceForStrategy(STRATEGY_MEDIA, false /*fromCache*/);
            break;
        }
        // when in call, DTMF and PHONE strategies follow the same rules
        // FALL THROUGH

    case STRATEGY_PHONE:
        // for phone strategy, we first consider the forced use and then the available devices by order
        // of priority
        switch (mForceUse[AudioSystem::FOR_COMMUNICATION]) {
        case AudioSystem::FORCE_BT_SCO:
            if (!isInCall() || strategy != STRATEGY_DTMF) {
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT;
                if (device) break;
            }
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
            if (device) break;
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_SCO;
            if (device) break;
            // if SCO device is requested but no SCO device is available, fall back to default case
            // FALL THROUGH

        default:    // FORCE_NONE
            // when not in a phone call, phone strategy should route STREAM_VOICE_CALL to A2DP
            if (mHasA2dp && !isInCall() &&
                    (mForceUse[AudioSystem::FOR_MEDIA] != AudioSystem::FORCE_NO_BT_A2DP) &&
                    (getA2dpOutput() != 0) && !mA2dpSuspended) {
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;
                if (device) break;
            }
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
            if (device) break;
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_WIRED_HEADSET;
            if (device) break;
            if (mPhoneState != AudioSystem::MODE_IN_CALL) {
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_ACCESSORY;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_DEVICE;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_AUX_DIGITAL;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET;
                if (device) break;
            }
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_EARPIECE;
            if (device) break;
            device = mDefaultOutputDevice;
            if (device == AUDIO_DEVICE_NONE) {
                ALOGE("getDeviceForStrategy() no device found for STRATEGY_PHONE");
            }
            break;

        case AudioSystem::FORCE_SPEAKER:
            // when not in a phone call, phone strategy should route STREAM_VOICE_CALL to
            // A2DP speaker when forcing to speaker output
            if (mHasA2dp && !isInCall() &&
                    (mForceUse[AudioSystem::FOR_MEDIA] != AudioSystem::FORCE_NO_BT_A2DP) &&
                    (getA2dpOutput() != 0) && !mA2dpSuspended) {
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER;
                if (device) break;
            }
            if (mPhoneState != AudioSystem::MODE_IN_CALL) {
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_ACCESSORY;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_DEVICE;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_AUX_DIGITAL;
                if (device) break;
                device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET;
                if (device) break;
            }
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_SPEAKER;
            if (device) break;
            device = mDefaultOutputDevice;
            if (device == AUDIO_DEVICE_NONE) {
                ALOGE("getDeviceForStrategy() no device found for STRATEGY_PHONE, FORCE_SPEAKER");
            }
            break;
        }
    break;

    case STRATEGY_SONIFICATION:

        // If incall, just select the STRATEGY_PHONE device: The rest of the behavior is handled by
        // handleIncallSonification().
        if (isInCall()) {
            device = getDeviceForStrategy(STRATEGY_PHONE, false /*fromCache*/);
            break;
        }
        // FALL THROUGH

    case STRATEGY_ENFORCED_AUDIBLE:
        // strategy STRATEGY_ENFORCED_AUDIBLE uses same routing policy as STRATEGY_SONIFICATION
        // except:
        //   - when in call where it doesn't default to STRATEGY_PHONE behavior
        //   - in countries where not enforced in which case it follows STRATEGY_MEDIA

        if ((strategy == STRATEGY_SONIFICATION) ||
                (mForceUse[AudioSystem::FOR_SYSTEM] == AudioSystem::FORCE_SYSTEM_ENFORCED)) {
            device = mAvailableOutputDevices & AUDIO_DEVICE_OUT_SPEAKER;
            if (device == AUDIO_DEVICE_NONE) {
                ALOGE("getDeviceForStrategy() speaker device not found for STRATEGY_SONIFICATION");
            }
        }
        // The second device used for sonification is the same as the device used by media strategy
        // FALL THROUGH

    case STRATEGY_MEDIA: {
        uint32_t device2 = AUDIO_DEVICE_NONE;
        if (strategy != STRATEGY_SONIFICATION) {
            // no sonification on remote submix (e.g. WFD)
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_REMOTE_SUBMIX;
        }
        if ((device2 == AUDIO_DEVICE_NONE) &&
                mHasA2dp && (mForceUse[AudioSystem::FOR_MEDIA] != AudioSystem::FORCE_NO_BT_A2DP) &&
                (getA2dpOutput() != 0) && !mA2dpSuspended) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP;
            if (device2 == AUDIO_DEVICE_NONE) {
                device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;
            }
            if (device2 == AUDIO_DEVICE_NONE) {
                device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER;
            }
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_WIRED_HEADSET;
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_ACCESSORY;
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_USB_DEVICE;
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET;
        }
        if ((device2 == AUDIO_DEVICE_NONE) && (strategy != STRATEGY_SONIFICATION)) {
            // no sonification on aux digital (e.g. HDMI)
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_AUX_DIGITAL;
        }
        if ((device2 == AUDIO_DEVICE_NONE) &&
                (mForceUse[AudioSystem::FOR_DOCK] == AudioSystem::FORCE_ANALOG_DOCK)) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET;
        }
        if (device2 == AUDIO_DEVICE_NONE) {
            device2 = mAvailableOutputDevices & AUDIO_DEVICE_OUT_SPEAKER;
        }

        // device is DEVICE_OUT_SPEAKER if we come from case STRATEGY_SONIFICATION or
        // STRATEGY_ENFORCED_AUDIBLE, AUDIO_DEVICE_NONE otherwise
        device |= device2;
        if (device) break;
        device = mDefaultOutputDevice;
        if (device == AUDIO_DEVICE_NONE) {
            ALOGE("getDeviceForStrategy() no device found for STRATEGY_MEDIA");
        }
        } break;

    default:
        ALOGW("getDeviceForStrategy() unknown strategy: %d", strategy);
        break;
    }

    ALOGVV("getDeviceForStrategy() strategy %d, device %x", strategy, device);
    return device;
}

void AudioPolicyManagerBase::updateDevicesAndOutputs()
{
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        mDeviceForStrategy[i] = getDeviceForStrategy((routing_strategy)i, false /*fromCache*/);
    }
    mPreviousOutputs = mOutputs;
}

uint32_t AudioPolicyManagerBase::checkDeviceMuteStrategies(AudioOutputDescriptor *outputDesc,
                                                       audio_devices_t prevDevice,
                                                       uint32_t delayMs)
{
    // mute/unmute strategies using an incompatible device combination
    // if muting, wait for the audio in pcm buffer to be drained before proceeding
    // if unmuting, unmute only after the specified delay
    if (outputDesc->isDuplicated()) {
        return 0;
    }

    uint32_t muteWaitMs = 0;
    audio_devices_t device = outputDesc->device();
    bool shouldMute = (outputDesc->refCount() != 0) &&
                    (AudioSystem::popCount(device) >= 2);
    // temporary mute output if device selection changes to avoid volume bursts due to
    // different per device volumes
    bool tempMute = (outputDesc->refCount() != 0) && (device != prevDevice);

    for (size_t i = 0; i < NUM_STRATEGIES; i++) {
        audio_devices_t curDevice = getDeviceForStrategy((routing_strategy)i, false /*fromCache*/);
        bool mute = shouldMute && (curDevice & device) && (curDevice != device);
        bool doMute = false;

        if (mute && !outputDesc->mStrategyMutedByDevice[i]) {
            doMute = true;
            outputDesc->mStrategyMutedByDevice[i] = true;
        } else if (!mute && outputDesc->mStrategyMutedByDevice[i]){
            doMute = true;
            outputDesc->mStrategyMutedByDevice[i] = false;
        }
        if (doMute || tempMute) {
            for (size_t j = 0; j < mOutputs.size(); j++) {
                AudioOutputDescriptor *desc = mOutputs.valueAt(j);
                if ((desc->supportedDevices() & outputDesc->supportedDevices())
                        == AUDIO_DEVICE_NONE) {
                    continue;
                }
                audio_io_handle_t curOutput = mOutputs.keyAt(j);
                ALOGVV("checkDeviceMuteStrategies() %s strategy %d (curDevice %04x) on output %d",
                      mute ? "muting" : "unmuting", i, curDevice, curOutput);
                setStrategyMute((routing_strategy)i, mute, curOutput, mute ? 0 : delayMs);
                if (desc->strategyRefCount((routing_strategy)i) != 0) {
                    if (tempMute) {
                        setStrategyMute((routing_strategy)i, true, curOutput);
                        setStrategyMute((routing_strategy)i, false, curOutput,
#ifdef QCOM_HARDWARE
                                            desc->latency() * 4,
#else
                                            desc->latency() * 2,
#endif
                                            device);
                    }
                    if (tempMute || mute) {
                        if (muteWaitMs < desc->latency()) {
                            muteWaitMs = desc->latency();
                        }
                    }
                }
            }
        }
    }

    // FIXME: should not need to double latency if volume could be applied immediately by the
    // audioflinger mixer. We must account for the delay between now and the next time
    // the audioflinger thread for this output will process a buffer (which corresponds to
    // one buffer size, usually 1/2 or 1/4 of the latency).
    muteWaitMs *= 2;
    // wait for the PCM output buffers to empty before proceeding with the rest of the command
    if (muteWaitMs > delayMs) {
        muteWaitMs -= delayMs;
        usleep(muteWaitMs * 1000);
        return muteWaitMs;
    }
    return 0;
}

uint32_t AudioPolicyManagerBase::setOutputDevice(audio_io_handle_t output,
                                             audio_devices_t device,
                                             bool force,
                                             int delayMs)
{
    ALOGV("setOutputDevice() output %d device %04x delayMs %d", output, device, delayMs);
    AudioOutputDescriptor *outputDesc = mOutputs.valueFor(output);
    AudioParameter param;
    uint32_t muteWaitMs = 0;

    if (outputDesc->isDuplicated()) {
        muteWaitMs = setOutputDevice(outputDesc->mOutput1->mId, device, force, delayMs);
        muteWaitMs += setOutputDevice(outputDesc->mOutput2->mId, device, force, delayMs);
        return muteWaitMs;
    }
    // filter devices according to output selected
    device = (audio_devices_t)(device & outputDesc->mProfile->mSupportedDevices);

    audio_devices_t prevDevice = outputDesc->mDevice;

    ALOGV("setOutputDevice() prevDevice %04x", prevDevice);

    if (device != AUDIO_DEVICE_NONE) {
        outputDesc->mDevice = device;
    }
    muteWaitMs = checkDeviceMuteStrategies(outputDesc, prevDevice, delayMs);

    // Do not change the routing if:
    //  - the requested device is AUDIO_DEVICE_NONE
    //  - the requested device is the same as current device and force is not specified.
    // Doing this check here allows the caller to call setOutputDevice() without conditions
    if ((device == AUDIO_DEVICE_NONE || device == prevDevice) && !force) {
        ALOGV("setOutputDevice() setting same device %04x or null device for output %d", device, output);
        return muteWaitMs;
    }

    ALOGV("setOutputDevice() changing device");
    // do the routing
    param.addInt(String8(AudioParameter::keyRouting), (int)device);
    mpClientInterface->setParameters(output, param.toString(), delayMs);

    // update stream volumes according to new device
    applyStreamVolumes(output, device, delayMs);

    return muteWaitMs;
}

AudioPolicyManagerBase::IOProfile *AudioPolicyManagerBase::getInputProfile(audio_devices_t device,
                                                   uint32_t samplingRate,
                                                   uint32_t format,
                                                   uint32_t channelMask)
{
    // Choose an input profile based on the requested capture parameters: select the first available
    // profile supporting all requested parameters.

    for (size_t i = 0; i < mHwModules.size(); i++)
    {
        if (mHwModules[i]->mHandle == 0) {
            continue;
        }
        for (size_t j = 0; j < mHwModules[i]->mInputProfiles.size(); j++)
        {
            IOProfile *profile = mHwModules[i]->mInputProfiles[j];
            if (profile->isCompatibleProfile(device, samplingRate, format,
                                             channelMask,(audio_output_flags_t)0)) {
                return profile;
            }
        }
    }
    return NULL;
}

audio_devices_t AudioPolicyManagerBase::getDeviceForInputSource(int inputSource)
{
    uint32_t device = AUDIO_DEVICE_NONE;

    switch(inputSource) {
    case AUDIO_SOURCE_DEFAULT:
    case AUDIO_SOURCE_MIC:
    case AUDIO_SOURCE_VOICE_RECOGNITION:
    case AUDIO_SOURCE_VOICE_COMMUNICATION:
        if (mForceUse[AudioSystem::FOR_RECORD] == AudioSystem::FORCE_BT_SCO &&
            mAvailableInputDevices & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
            device = AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
        } else if (mAvailableInputDevices & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            device = AUDIO_DEVICE_IN_WIRED_HEADSET;
        } else if (mAvailableInputDevices & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            device = AUDIO_DEVICE_IN_BUILTIN_MIC;
        }
        break;
    case AUDIO_SOURCE_CAMCORDER:
        if (mAvailableInputDevices & AUDIO_DEVICE_IN_BACK_MIC) {
            device = AUDIO_DEVICE_IN_BACK_MIC;
        } else if (mAvailableInputDevices & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            device = AUDIO_DEVICE_IN_BUILTIN_MIC;
        }
        break;
    case AUDIO_SOURCE_VOICE_UPLINK:
    case AUDIO_SOURCE_VOICE_DOWNLINK:
    case AUDIO_SOURCE_VOICE_CALL:
        if (mAvailableInputDevices & AUDIO_DEVICE_IN_VOICE_CALL) {
            device = AUDIO_DEVICE_IN_VOICE_CALL;
        }
        break;
    case AUDIO_SOURCE_REMOTE_SUBMIX:
        if (mAvailableInputDevices & AUDIO_DEVICE_IN_REMOTE_SUBMIX) {
            device = AUDIO_DEVICE_IN_REMOTE_SUBMIX;
        }
        break;
    default:
        ALOGW("getDeviceForInputSource() invalid input source %d", inputSource);
        break;
    }
    ALOGV("getDeviceForInputSource()input source %d, device %08x", inputSource, device);
    return device;
}

bool AudioPolicyManagerBase::isVirtualInputDevice(audio_devices_t device)
{
    if ((device & AUDIO_DEVICE_BIT_IN) != 0) {
        device &= ~AUDIO_DEVICE_BIT_IN;
        if ((popcount(device) == 1) && ((device & ~APM_AUDIO_IN_DEVICE_VIRTUAL_ALL) == 0))
            return true;
    }
    return false;
}

audio_io_handle_t AudioPolicyManagerBase::getActiveInput(bool ignoreVirtualInputs)
{
    for (size_t i = 0; i < mInputs.size(); i++) {
        const AudioInputDescriptor * input_descriptor = mInputs.valueAt(i);
        if ((input_descriptor->mRefCount > 0)
                && (!ignoreVirtualInputs || !isVirtualInputDevice(input_descriptor->mDevice))) {
            return mInputs.keyAt(i);
        }
    }
    return 0;
}


audio_devices_t AudioPolicyManagerBase::getDeviceForVolume(audio_devices_t device)
{
    if (device == AUDIO_DEVICE_NONE) {
        // this happens when forcing a route update and no track is active on an output.
        // In this case the returned category is not important.
        device =  AUDIO_DEVICE_OUT_SPEAKER;
    } else if (AudioSystem::popCount(device) > 1) {
        // Multiple device selection is either:
        //  - speaker + one other device: give priority to speaker in this case.
        //  - one A2DP device + another device: happens with duplicated output. In this case
        // retain the device on the A2DP output as the other must not correspond to an active
        // selection if not the speaker.
        if (device & AUDIO_DEVICE_OUT_SPEAKER) {
            device = AUDIO_DEVICE_OUT_SPEAKER;
        } else {
            device = (audio_devices_t)(device & AUDIO_DEVICE_OUT_ALL_A2DP);
        }
    }

    ALOGW_IF(AudioSystem::popCount(device) != 1,
            "getDeviceForVolume() invalid device combination: %08x",
            device);

    return device;
}

AudioPolicyManagerBase::device_category AudioPolicyManagerBase::getDeviceCategory(audio_devices_t device)
{
    switch(getDeviceForVolume(device)) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            return DEVICE_CATEGORY_EARPIECE;
        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES:
#if defined(QCOM_FM_ENABLED) || defined(STE_FM)
        case AUDIO_DEVICE_OUT_FM:
#endif
            return DEVICE_CATEGORY_HEADSET;
        case AUDIO_DEVICE_OUT_SPEAKER:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
        case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER:
        case AUDIO_DEVICE_OUT_AUX_DIGITAL:
        case AUDIO_DEVICE_OUT_USB_ACCESSORY:
        case AUDIO_DEVICE_OUT_USB_DEVICE:
        case AUDIO_DEVICE_OUT_REMOTE_SUBMIX:
        default:
            return DEVICE_CATEGORY_SPEAKER;
    }
}

float AudioPolicyManagerBase::volIndexToAmpl(audio_devices_t device, const StreamDescriptor& streamDesc,
        int indexInUi)
{
    device_category deviceCategory = getDeviceCategory(device);
    const VolumeCurvePoint *curve = streamDesc.mVolumeCurve[deviceCategory];

    // the volume index in the UI is relative to the min and max volume indices for this stream type
    int nbSteps = 1 + curve[VOLMAX].mIndex -
            curve[VOLMIN].mIndex;
    int volIdx = (nbSteps * (indexInUi - streamDesc.mIndexMin)) /
            (streamDesc.mIndexMax - streamDesc.mIndexMin);

    // find what part of the curve this index volume belongs to, or if it's out of bounds
    int segment = 0;
    if (volIdx < curve[VOLMIN].mIndex) {         // out of bounds
        return 0.0f;
    } else if (volIdx < curve[VOLKNEE1].mIndex) {
        segment = 0;
    } else if (volIdx < curve[VOLKNEE2].mIndex) {
        segment = 1;
    } else if (volIdx <= curve[VOLMAX].mIndex) {
        segment = 2;
    } else {                                                               // out of bounds
        return 1.0f;
    }

    // linear interpolation in the attenuation table in dB
    float decibels = curve[segment].mDBAttenuation +
            ((float)(volIdx - curve[segment].mIndex)) *
                ( (curve[segment+1].mDBAttenuation -
                        curve[segment].mDBAttenuation) /
                    ((float)(curve[segment+1].mIndex -
                            curve[segment].mIndex)) );

    float amplification = exp( decibels * 0.115129f); // exp( dB * ln(10) / 20 )

    ALOGVV("VOLUME vol index=[%d %d %d], dB=[%.1f %.1f %.1f] ampl=%.5f",
            curve[segment].mIndex, volIdx,
            curve[segment+1].mIndex,
            curve[segment].mDBAttenuation,
            decibels,
            curve[segment+1].mDBAttenuation,
            amplification);

    return amplification;
}

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sDefaultVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -49.5f}, {33, -33.5f}, {66, -17.0f}, {100, 0.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sDefaultMediaVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -58.0f}, {20, -40.0f}, {60, -17.0f}, {100, 0.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sSpeakerMediaVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -56.0f}, {20, -34.0f}, {60, -11.0f}, {100, 0.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sSpeakerSonificationVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -29.7f}, {33, -20.1f}, {66, -10.2f}, {100, 0.0f}
};

// AUDIO_STREAM_SYSTEM, AUDIO_STREAM_ENFORCED_AUDIBLE and AUDIO_STREAM_DTMF volume tracks
// AUDIO_STREAM_RING on phones and AUDIO_STREAM_MUSIC on tablets (See AudioService.java).
// The range is constrained between -24dB and -6dB over speaker and -30dB and -18dB over headset.
const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sDefaultSystemVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -24.0f}, {33, -18.0f}, {66, -12.0f}, {100, -6.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sHeadsetSystemVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {1, -30.0f}, {33, -26.0f}, {66, -22.0f}, {100, -18.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sDefaultVoiceVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {0, -42.0f}, {33, -28.0f}, {66, -14.0f}, {100, 0.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
    AudioPolicyManagerBase::sSpeakerVoiceVolumeCurve[AudioPolicyManagerBase::VOLCNT] = {
    {0, -24.0f}, {33, -16.0f}, {66, -8.0f}, {100, 0.0f}
};

const AudioPolicyManagerBase::VolumeCurvePoint
            *AudioPolicyManagerBase::sVolumeProfiles[AUDIO_STREAM_CNT]
                                                   [AudioPolicyManagerBase::DEVICE_CATEGORY_CNT] = {
    { // AUDIO_STREAM_VOICE_CALL
        sDefaultVoiceVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerVoiceVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultVoiceVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_SYSTEM
        sHeadsetSystemVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sDefaultSystemVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultSystemVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_RING
        sDefaultVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerSonificationVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_MUSIC
        sDefaultMediaVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerMediaVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultMediaVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_ALARM
        sDefaultVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerSonificationVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_NOTIFICATION
        sDefaultVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerSonificationVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_BLUETOOTH_SCO
        sDefaultVoiceVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerVoiceVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultVoiceVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_ENFORCED_AUDIBLE
        sHeadsetSystemVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sDefaultSystemVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultSystemVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    {  // AUDIO_STREAM_DTMF
        sHeadsetSystemVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sDefaultSystemVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultSystemVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
    { // AUDIO_STREAM_TTS
        sDefaultMediaVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerMediaVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultMediaVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
#ifdef QCOM_FM_ENABLED
    { // AUDIO_STREAM_FM
        sDefaultMediaVolumeCurve, // DEVICE_CATEGORY_HEADSET
        sSpeakerMediaVolumeCurve, // DEVICE_CATEGORY_SPEAKER
        sDefaultMediaVolumeCurve  // DEVICE_CATEGORY_EARPIECE
    },
#endif

};

void AudioPolicyManagerBase::initializeVolumeCurves()
{
    for (int i = 0; i < AUDIO_STREAM_CNT; i++) {
        for (int j = 0; j < DEVICE_CATEGORY_CNT; j++) {
            mStreams[i].mVolumeCurve[j] =
                    sVolumeProfiles[i][j];
        }
    }
}

float AudioPolicyManagerBase::computeVolume(int stream,
                                            int index,
                                            audio_io_handle_t output,
                                            audio_devices_t device)
{
    float volume = 1.0;
    AudioOutputDescriptor *outputDesc = mOutputs.valueFor(output);
    StreamDescriptor &streamDesc = mStreams[stream];

    if (device == AUDIO_DEVICE_NONE) {
        device = outputDesc->device();
    }

    // if volume is not 0 (not muted), force media volume to max on digital output
    if (stream == AudioSystem::MUSIC &&
        index != mStreams[stream].mIndexMin &&
        (device == AUDIO_DEVICE_OUT_AUX_DIGITAL ||
         device == AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET ||
         device == AUDIO_DEVICE_OUT_USB_ACCESSORY ||
         device == AUDIO_DEVICE_OUT_USB_DEVICE)) {
        return 1.0;
    }

    volume = volIndexToAmpl(device, streamDesc, index);

    // if a headset is connected, apply the following rules to ring tones and notifications
    // to avoid sound level bursts in user's ears:
    // - always attenuate ring tones and notifications volume by 6dB
    // - if music is playing, always limit the volume to current music volume,
    // with a minimum threshold at -36dB so that notification is always perceived.
    const routing_strategy stream_strategy = getStrategy((AudioSystem::stream_type)stream);
    if ((device & (AUDIO_DEVICE_OUT_BLUETOOTH_A2DP |
            AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES |
            AUDIO_DEVICE_OUT_WIRED_HEADSET |
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) &&
        ((stream_strategy == STRATEGY_SONIFICATION)
                || (stream_strategy == STRATEGY_SONIFICATION_RESPECTFUL)
                || (stream == AudioSystem::SYSTEM)
                || ((stream_strategy == STRATEGY_ENFORCED_AUDIBLE) &&
                    (mForceUse[AudioSystem::FOR_SYSTEM] == AudioSystem::FORCE_NONE))) &&
        streamDesc.mCanBeMuted) {
        volume *= SONIFICATION_HEADSET_VOLUME_FACTOR;
        // when the phone is ringing we must consider that music could have been paused just before
        // by the music application and behave as if music was active if the last music track was
        // just stopped
        if (isStreamActive(AudioSystem::MUSIC, SONIFICATION_HEADSET_MUSIC_DELAY) ||
                mLimitRingtoneVolume) {
            audio_devices_t musicDevice = getDeviceForStrategy(STRATEGY_MEDIA, true /*fromCache*/);
            float musicVol = computeVolume(AudioSystem::MUSIC,
                               mStreams[AudioSystem::MUSIC].getVolumeIndex(musicDevice),
                               output,
                               musicDevice);
            float minVol = (musicVol > SONIFICATION_HEADSET_VOLUME_MIN) ?
                                musicVol : SONIFICATION_HEADSET_VOLUME_MIN;
            if (volume > minVol) {
                volume = minVol;
                ALOGV("computeVolume limiting volume to %f musicVol %f", minVol, musicVol);
            }
        }
    }

    return volume;
}

status_t AudioPolicyManagerBase::checkAndSetVolume(int stream,
                                                   int index,
                                                   audio_io_handle_t output,
                                                   audio_devices_t device,
                                                   int delayMs,
                                                   bool force)
{

    // do not change actual stream volume if the stream is muted
    if (mOutputs.valueFor(output)->mMuteCount[stream] != 0) {
        ALOGVV("checkAndSetVolume() stream %d muted count %d",
              stream, mOutputs.valueFor(output)->mMuteCount[stream]);
        return NO_ERROR;
    }

    // do not change in call volume if bluetooth is connected and vice versa
    if ((stream == AudioSystem::VOICE_CALL && mForceUse[AudioSystem::FOR_COMMUNICATION] == AudioSystem::FORCE_BT_SCO) ||
        (stream == AudioSystem::BLUETOOTH_SCO && mForceUse[AudioSystem::FOR_COMMUNICATION] != AudioSystem::FORCE_BT_SCO)) {
        ALOGV("checkAndSetVolume() cannot set stream %d volume with force use = %d for comm",
             stream, mForceUse[AudioSystem::FOR_COMMUNICATION]);
        return INVALID_OPERATION;
    }

    float volume = computeVolume(stream, index, output, device);
    // We actually change the volume if:
    // - the float value returned by computeVolume() changed
    // - the force flag is set
    if (volume != mOutputs.valueFor(output)->mCurVolume[stream] ||
            force) {
        mOutputs.valueFor(output)->mCurVolume[stream] = volume;
        ALOGVV("checkAndSetVolume() for output %d stream %d, volume %f, delay %d", output, stream, volume, delayMs);
        // Force VOICE_CALL to track BLUETOOTH_SCO stream volume when bluetooth audio is
        // enabled
        if (stream == AudioSystem::BLUETOOTH_SCO) {
            mpClientInterface->setStreamVolume(AudioSystem::VOICE_CALL, volume, output, delayMs);
        }
        mpClientInterface->setStreamVolume((AudioSystem::stream_type)stream, volume, output, delayMs);
    }

    if (stream == AudioSystem::VOICE_CALL ||
        stream == AudioSystem::BLUETOOTH_SCO) {
        float voiceVolume;
        // Force voice volume to max for bluetooth SCO as volume is managed by the headset
        if (stream == AudioSystem::VOICE_CALL) {
            voiceVolume = (float)index/(float)mStreams[stream].mIndexMax;
        } else {
            voiceVolume = 1.0;
        }

        if (voiceVolume != mLastVoiceVolume && output == mPrimaryOutput) {
            mpClientInterface->setVoiceVolume(voiceVolume, delayMs);
            mLastVoiceVolume = voiceVolume;
        }
    }

    return NO_ERROR;
}

void AudioPolicyManagerBase::applyStreamVolumes(audio_io_handle_t output,
                                                audio_devices_t device,
                                                int delayMs,
                                                bool force)
{
    ALOGVV("applyStreamVolumes() for output %d and device %x", output, device);

    for (int stream = 0; stream < AudioSystem::NUM_STREAM_TYPES; stream++) {
        checkAndSetVolume(stream,
                          mStreams[stream].getVolumeIndex(device),
                          output,
                          device,
                          delayMs,
                          force);
    }
}

void AudioPolicyManagerBase::setStrategyMute(routing_strategy strategy,
                                             bool on,
                                             audio_io_handle_t output,
                                             int delayMs,
                                             audio_devices_t device)
{
    ALOGVV("setStrategyMute() strategy %d, mute %d, output %d", strategy, on, output);
    for (int stream = 0; stream < AudioSystem::NUM_STREAM_TYPES; stream++) {
        if (getStrategy((AudioSystem::stream_type)stream) == strategy) {
            setStreamMute(stream, on, output, delayMs, device);
        }
    }
}

void AudioPolicyManagerBase::setStreamMute(int stream,
                                           bool on,
                                           audio_io_handle_t output,
                                           int delayMs,
                                           audio_devices_t device)
{
    StreamDescriptor &streamDesc = mStreams[stream];
    AudioOutputDescriptor *outputDesc = mOutputs.valueFor(output);
    if (device == AUDIO_DEVICE_NONE) {
        device = outputDesc->device();
    }

    ALOGVV("setStreamMute() stream %d, mute %d, output %d, mMuteCount %d device %04x",
          stream, on, output, outputDesc->mMuteCount[stream], device);

    if (on) {
        if (outputDesc->mMuteCount[stream] == 0) {
            if (streamDesc.mCanBeMuted &&
                    ((stream != AudioSystem::ENFORCED_AUDIBLE) ||
                     (mForceUse[AudioSystem::FOR_SYSTEM] == AudioSystem::FORCE_NONE))) {
                checkAndSetVolume(stream, 0, output, device, delayMs);
            }
        }
        // increment mMuteCount after calling checkAndSetVolume() so that volume change is not ignored
        outputDesc->mMuteCount[stream]++;
    } else {
        if (outputDesc->mMuteCount[stream] == 0) {
            ALOGV("setStreamMute() unmuting non muted stream!");
            return;
        }
        if (--outputDesc->mMuteCount[stream] == 0) {
            checkAndSetVolume(stream,
                              streamDesc.getVolumeIndex(device),
                              output,
                              device,
                              delayMs);
        }
    }
}

void AudioPolicyManagerBase::handleIncallSonification(int stream, bool starting, bool stateChange)
{
    // if the stream pertains to sonification strategy and we are in call we must
    // mute the stream if it is low visibility. If it is high visibility, we must play a tone
    // in the device used for phone strategy and play the tone if the selected device does not
    // interfere with the device used for phone strategy
    // if stateChange is true, we are called from setPhoneState() and we must mute or unmute as
    // many times as there are active tracks on the output
    const routing_strategy stream_strategy = getStrategy((AudioSystem::stream_type)stream);
    if ((stream_strategy == STRATEGY_SONIFICATION) ||
            ((stream_strategy == STRATEGY_SONIFICATION_RESPECTFUL))) {
        AudioOutputDescriptor *outputDesc = mOutputs.valueFor(mPrimaryOutput);
        ALOGV("handleIncallSonification() stream %d starting %d device %x stateChange %d",
                stream, starting, outputDesc->mDevice, stateChange);
        if (outputDesc->mRefCount[stream]) {
            int muteCount = 1;
            if (stateChange) {
                muteCount = outputDesc->mRefCount[stream];
            }
            if (AudioSystem::isLowVisibility((AudioSystem::stream_type)stream)) {
                ALOGV("handleIncallSonification() low visibility, muteCount %d", muteCount);
                for (int i = 0; i < muteCount; i++) {
                    setStreamMute(stream, starting, mPrimaryOutput);
                }
            } else {
                ALOGV("handleIncallSonification() high visibility");
                if (outputDesc->device() &
                        getDeviceForStrategy(STRATEGY_PHONE, true /*fromCache*/)) {
                    ALOGV("handleIncallSonification() high visibility muted, muteCount %d", muteCount);
                    for (int i = 0; i < muteCount; i++) {
                        setStreamMute(stream, starting, mPrimaryOutput);
                    }
                }
                if (starting) {
                    mpClientInterface->startTone(ToneGenerator::TONE_SUP_CALL_WAITING, AudioSystem::VOICE_CALL);
                } else {
                    mpClientInterface->stopTone();
                }
            }
        }
    }
}

bool AudioPolicyManagerBase::isInCall()
{
    return isStateInCall(mPhoneState);
}

bool AudioPolicyManagerBase::isStateInCall(int state) {
    return ((state == AudioSystem::MODE_IN_CALL) ||
            (state == AudioSystem::MODE_IN_COMMUNICATION));
}

bool AudioPolicyManagerBase::needsDirectOuput(audio_stream_type_t stream,
                                              uint32_t samplingRate,
                                              audio_format_t format,
                                              audio_channel_mask_t channelMask,
                                              audio_output_flags_t flags,
                                              audio_devices_t device)
{
   return ((flags & AudioSystem::OUTPUT_FLAG_DIRECT) ||
          (format != 0 && !AudioSystem::isLinearPCM(format)));
}

uint32_t AudioPolicyManagerBase::getMaxEffectsCpuLoad()
{
    return MAX_EFFECTS_CPU_LOAD;
}

uint32_t AudioPolicyManagerBase::getMaxEffectsMemory()
{
    return MAX_EFFECTS_MEMORY;
}

// --- AudioOutputDescriptor class implementation

AudioPolicyManagerBase::AudioOutputDescriptor::AudioOutputDescriptor(
        const IOProfile *profile)
    : mId(0), mSamplingRate(0), mFormat((audio_format_t)0),
      mChannelMask((audio_channel_mask_t)0), mLatency(0),
    mFlags((audio_output_flags_t)0), mDevice(AUDIO_DEVICE_NONE),
    mOutput1(0), mOutput2(0), mProfile(profile)
{
    // clear usage count for all stream types
    for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
        mRefCount[i] = 0;
        mCurVolume[i] = -1.0;
        mMuteCount[i] = 0;
        mStopTime[i] = 0;
    }
    for (int i = 0; i < NUM_STRATEGIES; i++) {
        mStrategyMutedByDevice[i] = false;
    }
    if (profile != NULL) {
        mSamplingRate = profile->mSamplingRates[0];
        mFormat = profile->mFormats[0];
        mChannelMask = profile->mChannelMasks[0];
        mFlags = profile->mFlags;
    }
}

audio_devices_t AudioPolicyManagerBase::AudioOutputDescriptor::device()
{
    if (isDuplicated()) {
        return (audio_devices_t)(mOutput1->mDevice | mOutput2->mDevice);
    } else {
        return mDevice;
    }
}

uint32_t AudioPolicyManagerBase::AudioOutputDescriptor::latency()
{
    if (isDuplicated()) {
        return (mOutput1->mLatency > mOutput2->mLatency) ? mOutput1->mLatency : mOutput2->mLatency;
    } else {
        return mLatency;
    }
}

bool AudioPolicyManagerBase::AudioOutputDescriptor::sharesHwModuleWith(
        const AudioOutputDescriptor *outputDesc)
{
    if (isDuplicated()) {
        return mOutput1->sharesHwModuleWith(outputDesc) || mOutput2->sharesHwModuleWith(outputDesc);
    } else if (outputDesc->isDuplicated()){
        return sharesHwModuleWith(outputDesc->mOutput1) || sharesHwModuleWith(outputDesc->mOutput2);
    } else {
        return (mProfile->mModule == outputDesc->mProfile->mModule);
    }
}

void AudioPolicyManagerBase::AudioOutputDescriptor::changeRefCount(AudioSystem::stream_type stream, int delta)
{
    // forward usage count change to attached outputs
    if (isDuplicated()) {
        mOutput1->changeRefCount(stream, delta);
        mOutput2->changeRefCount(stream, delta);
    }
    if ((delta + (int)mRefCount[stream]) < 0) {
        ALOGW("changeRefCount() invalid delta %d for stream %d, refCount %d", delta, stream, mRefCount[stream]);
        mRefCount[stream] = 0;
        return;
    }
    mRefCount[stream] += delta;
    ALOGV("changeRefCount() stream %d, count %d", stream, mRefCount[stream]);
}

uint32_t AudioPolicyManagerBase::AudioOutputDescriptor::refCount()
{
    uint32_t refcount = 0;
    for (int i = 0; i < (int)AudioSystem::NUM_STREAM_TYPES; i++) {
        refcount += mRefCount[i];
    }
    return refcount;
}

uint32_t AudioPolicyManagerBase::AudioOutputDescriptor::strategyRefCount(routing_strategy strategy)
{
    uint32_t refCount = 0;
    for (int i = 0; i < (int)AudioSystem::NUM_STREAM_TYPES; i++) {
        if (getStrategy((AudioSystem::stream_type)i) == strategy) {
            refCount += mRefCount[i];
        }
    }
    return refCount;
}

audio_devices_t AudioPolicyManagerBase::AudioOutputDescriptor::supportedDevices()
{
    if (isDuplicated()) {
        return (audio_devices_t)(mOutput1->supportedDevices() | mOutput2->supportedDevices());
    } else {
        return mProfile->mSupportedDevices ;
    }
}

bool AudioPolicyManagerBase::AudioOutputDescriptor::isActive(uint32_t inPastMs) const
{
    nsecs_t sysTime = systemTime();
    for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
        if (mRefCount[i] != 0 ||
            ns2ms(sysTime - mStopTime[i]) < inPastMs) {
            return true;
        }
    }
    return false;
}

status_t AudioPolicyManagerBase::AudioOutputDescriptor::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, " Sampling rate: %d\n", mSamplingRate);
    result.append(buffer);
    snprintf(buffer, SIZE, " Format: %d\n", mFormat);
    result.append(buffer);
    snprintf(buffer, SIZE, " Channels: %08x\n", mChannelMask);
    result.append(buffer);
    snprintf(buffer, SIZE, " Latency: %d\n", mLatency);
    result.append(buffer);
    snprintf(buffer, SIZE, " Flags %08x\n", mFlags);
    result.append(buffer);
    snprintf(buffer, SIZE, " Devices %08x\n", device());
    result.append(buffer);
    snprintf(buffer, SIZE, " Stream volume refCount muteCount\n");
    result.append(buffer);
    for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
        snprintf(buffer, SIZE, " %02d     %.03f     %02d       %02d\n", i, mCurVolume[i], mRefCount[i], mMuteCount[i]);
        result.append(buffer);
    }
    write(fd, result.string(), result.size());

    return NO_ERROR;
}

// --- AudioInputDescriptor class implementation

AudioPolicyManagerBase::AudioInputDescriptor::AudioInputDescriptor(const IOProfile *profile)
    : mSamplingRate(0), mFormat((audio_format_t)0), mChannelMask((audio_channel_mask_t)0),
      mDevice(AUDIO_DEVICE_NONE), mRefCount(0),
      mInputSource(0), mProfile(profile)
{
}

status_t AudioPolicyManagerBase::AudioInputDescriptor::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, " Sampling rate: %d\n", mSamplingRate);
    result.append(buffer);
    snprintf(buffer, SIZE, " Format: %d\n", mFormat);
    result.append(buffer);
    snprintf(buffer, SIZE, " Channels: %08x\n", mChannelMask);
    result.append(buffer);
    snprintf(buffer, SIZE, " Devices %08x\n", mDevice);
    result.append(buffer);
    snprintf(buffer, SIZE, " Ref Count %d\n", mRefCount);
    result.append(buffer);
    write(fd, result.string(), result.size());

    return NO_ERROR;
}

// --- StreamDescriptor class implementation

AudioPolicyManagerBase::StreamDescriptor::StreamDescriptor()
    :   mIndexMin(0), mIndexMax(1), mCanBeMuted(true)
{
    mIndexCur.add(AUDIO_DEVICE_OUT_DEFAULT, 0);
}

int AudioPolicyManagerBase::StreamDescriptor::getVolumeIndex(audio_devices_t device)
{
    device = AudioPolicyManagerBase::getDeviceForVolume(device);
    // there is always a valid entry for AUDIO_DEVICE_OUT_DEFAULT
    if (mIndexCur.indexOfKey(device) < 0) {
        device = AUDIO_DEVICE_OUT_DEFAULT;
    }
    return mIndexCur.valueFor(device);
}

void AudioPolicyManagerBase::StreamDescriptor::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "%s         %02d         %02d         ",
             mCanBeMuted ? "true " : "false", mIndexMin, mIndexMax);
    result.append(buffer);
    for (size_t i = 0; i < mIndexCur.size(); i++) {
        snprintf(buffer, SIZE, "%04x : %02d, ",
                 mIndexCur.keyAt(i),
                 mIndexCur.valueAt(i));
        result.append(buffer);
    }
    result.append("\n");

    write(fd, result.string(), result.size());
}

// --- EffectDescriptor class implementation

status_t AudioPolicyManagerBase::EffectDescriptor::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, " I/O: %d\n", mIo);
    result.append(buffer);
    snprintf(buffer, SIZE, " Strategy: %d\n", mStrategy);
    result.append(buffer);
    snprintf(buffer, SIZE, " Session: %d\n", mSession);
    result.append(buffer);
    snprintf(buffer, SIZE, " Name: %s\n",  mDesc.name);
    result.append(buffer);
    snprintf(buffer, SIZE, " %s\n",  mEnabled ? "Enabled" : "Disabled");
    result.append(buffer);
    write(fd, result.string(), result.size());

    return NO_ERROR;
}

// --- IOProfile class implementation

AudioPolicyManagerBase::HwModule::HwModule(const char *name)
    : mName(strndup(name, AUDIO_HARDWARE_MODULE_ID_MAX_LEN)), mHandle(0)
{
}

AudioPolicyManagerBase::HwModule::~HwModule()
{
    for (size_t i = 0; i < mOutputProfiles.size(); i++) {
         delete mOutputProfiles[i];
    }
    for (size_t i = 0; i < mInputProfiles.size(); i++) {
         delete mInputProfiles[i];
    }
    free((void *)mName);
}

void AudioPolicyManagerBase::HwModule::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "  - name: %s\n", mName);
    result.append(buffer);
    snprintf(buffer, SIZE, "  - handle: %d\n", mHandle);
    result.append(buffer);
    write(fd, result.string(), result.size());
    if (mOutputProfiles.size()) {
        write(fd, "  - outputs:\n", sizeof("  - outputs:\n"));
        for (size_t i = 0; i < mOutputProfiles.size(); i++) {
            snprintf(buffer, SIZE, "    output %d:\n", i);
            write(fd, buffer, strlen(buffer));
            mOutputProfiles[i]->dump(fd);
        }
    }
    if (mInputProfiles.size()) {
        write(fd, "  - inputs:\n", sizeof("  - inputs:\n"));
        for (size_t i = 0; i < mInputProfiles.size(); i++) {
            snprintf(buffer, SIZE, "    input %d:\n", i);
            write(fd, buffer, strlen(buffer));
            mInputProfiles[i]->dump(fd);
        }
    }
}

AudioPolicyManagerBase::IOProfile::IOProfile(HwModule *module)
    : mFlags((audio_output_flags_t)0), mModule(module)
{
}

AudioPolicyManagerBase::IOProfile::~IOProfile()
{
}

// checks if the IO profile is compatible with specified parameters. By convention a value of 0
// means a parameter is don't care
bool AudioPolicyManagerBase::IOProfile::isCompatibleProfile(audio_devices_t device,
                                                            uint32_t samplingRate,
                                                            uint32_t format,
                                                            uint32_t channelMask,
                                                            audio_output_flags_t flags) const
{
    if ((mSupportedDevices & device) != device) {
        return false;
    }
    if ((mFlags & flags) != flags) {
        return false;
    }
    if (samplingRate != 0) {
        size_t i;
        for (i = 0; i < mSamplingRates.size(); i++)
        {
            if (mSamplingRates[i] == samplingRate) {
                break;
            }
        }
        if (i == mSamplingRates.size()) {
            return false;
        }
    }
    if (format != 0) {
        size_t i;
        for (i = 0; i < mFormats.size(); i++)
        {
            if (mFormats[i] == format) {
                break;
            }
        }
        if (i == mFormats.size()) {
            return false;
        }
    }
    if (channelMask != 0) {
        size_t i;
        for (i = 0; i < mChannelMasks.size(); i++)
        {
            if (mChannelMasks[i] == channelMask) {
                break;
            }
        }
        if (i == mChannelMasks.size()) {
            return false;
        }
    }
    return true;
}

void AudioPolicyManagerBase::IOProfile::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    snprintf(buffer, SIZE, "    - sampling rates: ");
    result.append(buffer);
    for (size_t i = 0; i < mSamplingRates.size(); i++) {
        snprintf(buffer, SIZE, "%d", mSamplingRates[i]);
        result.append(buffer);
        result.append(i == (mSamplingRates.size() - 1) ? "\n" : ", ");
    }

    snprintf(buffer, SIZE, "    - channel masks: ");
    result.append(buffer);
    for (size_t i = 0; i < mChannelMasks.size(); i++) {
        snprintf(buffer, SIZE, "%04x", mChannelMasks[i]);
        result.append(buffer);
        result.append(i == (mChannelMasks.size() - 1) ? "\n" : ", ");
    }

    snprintf(buffer, SIZE, "    - formats: ");
    result.append(buffer);
    for (size_t i = 0; i < mFormats.size(); i++) {
        snprintf(buffer, SIZE, "%d", mFormats[i]);
        result.append(buffer);
        result.append(i == (mFormats.size() - 1) ? "\n" : ", ");
    }

    snprintf(buffer, SIZE, "    - devices: %04x\n", mSupportedDevices);
    result.append(buffer);
    snprintf(buffer, SIZE, "    - flags: %04x\n", mFlags);
    result.append(buffer);

    write(fd, result.string(), result.size());
}

// --- audio_policy.conf file parsing

struct StringToEnum {
    const char *name;
    uint32_t value;
};

#define STRING_TO_ENUM(string) { #string, string }
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

const struct StringToEnum sDeviceNameToEnumTable[] = {
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_EARPIECE),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_SPEAKER),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_WIRED_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_WIRED_HEADPHONE),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ANC_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ANC_HEADPHONE),
#endif
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ALL_SCO),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ALL_A2DP),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_AUX_DIGITAL),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_USB_DEVICE),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_USB_ACCESSORY),
#if defined(QCOM_FM_ENABLED) || defined(STE_FM)
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_FM),
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_FM_TX),
#endif
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_ALL_USB),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_PROXY),
#endif
    STRING_TO_ENUM(AUDIO_DEVICE_OUT_REMOTE_SUBMIX),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_BUILTIN_MIC),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_WIRED_HEADSET),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_DEVICE_IN_ANC_HEADSET),
#endif
    STRING_TO_ENUM(AUDIO_DEVICE_IN_AUX_DIGITAL),
#if defined(QCOM_FM_ENABLED) || defined(STE_FM)
    STRING_TO_ENUM(AUDIO_DEVICE_IN_FM_RX),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_FM_RX_A2DP),
#endif
    STRING_TO_ENUM(AUDIO_DEVICE_IN_VOICE_CALL),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_BACK_MIC),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_REMOTE_SUBMIX),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_USB_ACCESSORY),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_DEVICE_IN_PROXY),
    STRING_TO_ENUM(AUDIO_DEVICE_IN_COMMUNICATION),
#endif
};

const struct StringToEnum sFlagNameToEnumTable[] = {
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_DIRECT),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_PRIMARY),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_FAST),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_DEEP_BUFFER),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_VOIP_RX),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_LPA),
    STRING_TO_ENUM(AUDIO_OUTPUT_FLAG_TUNNEL),
#endif
};

const struct StringToEnum sFormatNameToEnumTable[] = {
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_16_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_PCM_8_BIT),
    STRING_TO_ENUM(AUDIO_FORMAT_MP3),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC),
    STRING_TO_ENUM(AUDIO_FORMAT_VORBIS),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_FORMAT_AC3),
    STRING_TO_ENUM(AUDIO_FORMAT_EAC3),
    STRING_TO_ENUM(AUDIO_FORMAT_DTS),
    STRING_TO_ENUM(AUDIO_FORMAT_DTS_LBR),
    STRING_TO_ENUM(AUDIO_FORMAT_WMA),
    STRING_TO_ENUM(AUDIO_FORMAT_WMA_PRO),
    STRING_TO_ENUM(AUDIO_FORMAT_AAC_ADIF),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_NB),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_WB),
    STRING_TO_ENUM(AUDIO_FORMAT_AMR_WB_PLUS),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRC),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRCB),
    STRING_TO_ENUM(AUDIO_FORMAT_EVRCWB),
    STRING_TO_ENUM(AUDIO_FORMAT_QCELP),
    STRING_TO_ENUM(AUDIO_FORMAT_MP2),
#endif
};

const struct StringToEnum sOutChannelsNameToEnumTable[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_MONO),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_STEREO),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_2POINT1),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_QUAD),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_SURROUND),
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_PENTA),
#endif
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_5POINT1),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_6POINT1),
#endif
    STRING_TO_ENUM(AUDIO_CHANNEL_OUT_7POINT1),
};

const struct StringToEnum sInChannelsNameToEnumTable[] = {
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_MONO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_STEREO),
#ifdef QCOM_HARDWARE
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_5POINT1),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_VOICE_CALL_MONO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_VOICE_DNLINK_MONO),
    STRING_TO_ENUM(AUDIO_CHANNEL_IN_VOICE_UPLINK_MONO),
#endif
};


uint32_t AudioPolicyManagerBase::stringToEnum(const struct StringToEnum *table,
                                              size_t size,
                                              const char *name)
{
    for (size_t i = 0; i < size; i++) {
        if (strcmp(table[i].name, name) == 0) {
            ALOGV("stringToEnum() found %s", table[i].name);
            return table[i].value;
        }
    }
    return 0;
}

audio_output_flags_t AudioPolicyManagerBase::parseFlagNames(char *name)
{
    uint32_t flag = 0;

    // it is OK to cast name to non const here as we are not going to use it after
    // strtok() modifies it
    char *flagName = strtok(name, "|");
    while (flagName != NULL) {
        if (strlen(flagName) != 0) {
            flag |= stringToEnum(sFlagNameToEnumTable,
                               ARRAY_SIZE(sFlagNameToEnumTable),
                               flagName);
        }
        flagName = strtok(NULL, "|");
    }
    return (audio_output_flags_t)flag;
}

audio_devices_t AudioPolicyManagerBase::parseDeviceNames(char *name)
{
    uint32_t device = 0;

    char *devName = strtok(name, "|");
    while (devName != NULL) {
        if (strlen(devName) != 0) {
            device |= stringToEnum(sDeviceNameToEnumTable,
                                 ARRAY_SIZE(sDeviceNameToEnumTable),
                                 devName);
        }
        devName = strtok(NULL, "|");
    }
    return device;
}

void AudioPolicyManagerBase::loadSamplingRates(char *name, IOProfile *profile)
{
    char *str = strtok(name, "|");

    // by convention, "0' in the first entry in mSamplingRates indicates the supported sampling
    // rates should be read from the output stream after it is opened for the first time
    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG) == 0) {
        profile->mSamplingRates.add(0);
        return;
    }

    while (str != NULL) {
        uint32_t rate = atoi(str);
        if (rate != 0) {
            ALOGV("loadSamplingRates() adding rate %d", rate);
            profile->mSamplingRates.add(rate);
        }
        str = strtok(NULL, "|");
    }
    return;
}

void AudioPolicyManagerBase::loadFormats(char *name, IOProfile *profile)
{
    char *str = strtok(name, "|");

    // by convention, "0' in the first entry in mFormats indicates the supported formats
    // should be read from the output stream after it is opened for the first time
    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG) == 0) {
        profile->mFormats.add((audio_format_t)0);
        return;
    }

    while (str != NULL) {
        audio_format_t format = (audio_format_t)stringToEnum(sFormatNameToEnumTable,
                                                             ARRAY_SIZE(sFormatNameToEnumTable),
                                                             str);
        if (format != 0) {
            profile->mFormats.add(format);
        }
        str = strtok(NULL, "|");
    }
    return;
}

void AudioPolicyManagerBase::loadInChannels(char *name, IOProfile *profile)
{
    const char *str = strtok(name, "|");

    ALOGV("loadInChannels() %s", name);

    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG) == 0) {
        profile->mChannelMasks.add((audio_channel_mask_t)0);
        return;
    }

    while (str != NULL) {
        audio_channel_mask_t channelMask =
                (audio_channel_mask_t)stringToEnum(sInChannelsNameToEnumTable,
                                                   ARRAY_SIZE(sInChannelsNameToEnumTable),
                                                   str);
        if (channelMask != 0) {
            ALOGV("loadInChannels() adding channelMask %04x", channelMask);
            profile->mChannelMasks.add(channelMask);
        }
        str = strtok(NULL, "|");
    }
    return;
}

void AudioPolicyManagerBase::loadOutChannels(char *name, IOProfile *profile)
{
    const char *str = strtok(name, "|");

    ALOGV("loadOutChannels() %s", name);

    // by convention, "0' in the first entry in mChannelMasks indicates the supported channel
    // masks should be read from the output stream after it is opened for the first time
    if (str != NULL && strcmp(str, DYNAMIC_VALUE_TAG) == 0) {
        profile->mChannelMasks.add((audio_channel_mask_t)0);
        return;
    }

    while (str != NULL) {
        audio_channel_mask_t channelMask =
                (audio_channel_mask_t)stringToEnum(sOutChannelsNameToEnumTable,
                                                   ARRAY_SIZE(sOutChannelsNameToEnumTable),
                                                   str);
        if (channelMask != 0) {
            profile->mChannelMasks.add(channelMask);
        }
        str = strtok(NULL, "|");
    }
    return;
}

status_t AudioPolicyManagerBase::loadInput(cnode *root, HwModule *module)
{
    cnode *node = root->first_child;

    IOProfile *profile = new IOProfile(module);

    while (node) {
        if (strcmp(node->name, SAMPLING_RATES_TAG) == 0) {
            loadSamplingRates((char *)node->value, profile);
        } else if (strcmp(node->name, FORMATS_TAG) == 0) {
            loadFormats((char *)node->value, profile);
        } else if (strcmp(node->name, CHANNELS_TAG) == 0) {
            loadInChannels((char *)node->value, profile);
        } else if (strcmp(node->name, DEVICES_TAG) == 0) {
            profile->mSupportedDevices = parseDeviceNames((char *)node->value);
        }
        node = node->next;
    }
    ALOGW_IF(profile->mSupportedDevices == AUDIO_DEVICE_NONE,
            "loadInput() invalid supported devices");
    ALOGW_IF(profile->mChannelMasks.size() == 0,
            "loadInput() invalid supported channel masks");
    ALOGW_IF(profile->mSamplingRates.size() == 0,
            "loadInput() invalid supported sampling rates");
    ALOGW_IF(profile->mFormats.size() == 0,
            "loadInput() invalid supported formats");
    if ((profile->mSupportedDevices != AUDIO_DEVICE_NONE) &&
            (profile->mChannelMasks.size() != 0) &&
            (profile->mSamplingRates.size() != 0) &&
            (profile->mFormats.size() != 0)) {

        ALOGV("loadInput() adding input mSupportedDevices %04x", profile->mSupportedDevices);

        module->mInputProfiles.add(profile);
        return NO_ERROR;
    } else {
        delete profile;
        return BAD_VALUE;
    }
}

status_t AudioPolicyManagerBase::loadOutput(cnode *root, HwModule *module)
{
    cnode *node = root->first_child;

    IOProfile *profile = new IOProfile(module);

    while (node) {
        if (strcmp(node->name, SAMPLING_RATES_TAG) == 0) {
            loadSamplingRates((char *)node->value, profile);
        } else if (strcmp(node->name, FORMATS_TAG) == 0) {
            loadFormats((char *)node->value, profile);
        } else if (strcmp(node->name, CHANNELS_TAG) == 0) {
            loadOutChannels((char *)node->value, profile);
        } else if (strcmp(node->name, DEVICES_TAG) == 0) {
            profile->mSupportedDevices = parseDeviceNames((char *)node->value);
        } else if (strcmp(node->name, FLAGS_TAG) == 0) {
            profile->mFlags = parseFlagNames((char *)node->value);
        }
        node = node->next;
    }
    ALOGW_IF(profile->mSupportedDevices == AUDIO_DEVICE_NONE,
            "loadOutput() invalid supported devices");
    ALOGW_IF(profile->mChannelMasks.size() == 0,
            "loadOutput() invalid supported channel masks");
    ALOGW_IF(profile->mSamplingRates.size() == 0,
            "loadOutput() invalid supported sampling rates");
    ALOGW_IF(profile->mFormats.size() == 0,
            "loadOutput() invalid supported formats");
    if ((profile->mSupportedDevices != AUDIO_DEVICE_NONE) &&
            (profile->mChannelMasks.size() != 0) &&
            (profile->mSamplingRates.size() != 0) &&
            (profile->mFormats.size() != 0)) {

        ALOGV("loadOutput() adding output mSupportedDevices %04x, mFlags %04x",
              profile->mSupportedDevices, profile->mFlags);

        module->mOutputProfiles.add(profile);
        return NO_ERROR;
    } else {
        delete profile;
        return BAD_VALUE;
    }
}

void AudioPolicyManagerBase::loadHwModule(cnode *root)
{
    cnode *node = config_find(root, OUTPUTS_TAG);
    status_t status = NAME_NOT_FOUND;

    HwModule *module = new HwModule(root->name);

    if (node != NULL) {
        if (strcmp(root->name, AUDIO_HARDWARE_MODULE_ID_A2DP) == 0) {
            mHasA2dp = true;
        } else if (strcmp(root->name, AUDIO_HARDWARE_MODULE_ID_USB) == 0) {
            mHasUsb = true;
        } else if (strcmp(root->name, AUDIO_HARDWARE_MODULE_ID_REMOTE_SUBMIX) == 0) {
            mHasRemoteSubmix = true;
        }

        node = node->first_child;
        while (node) {
            ALOGV("loadHwModule() loading output %s", node->name);
            status_t tmpStatus = loadOutput(node, module);
            if (status == NAME_NOT_FOUND || status == NO_ERROR) {
                status = tmpStatus;
            }
            node = node->next;
        }
    }
    node = config_find(root, INPUTS_TAG);
    if (node != NULL) {
        node = node->first_child;
        while (node) {
            ALOGV("loadHwModule() loading input %s", node->name);
            status_t tmpStatus = loadInput(node, module);
            if (status == NAME_NOT_FOUND || status == NO_ERROR) {
                status = tmpStatus;
            }
            node = node->next;
        }
    }
    if (status == NO_ERROR) {
        mHwModules.add(module);
    } else {
        delete module;
    }
}

void AudioPolicyManagerBase::loadHwModules(cnode *root)
{
    cnode *node = config_find(root, AUDIO_HW_MODULE_TAG);
    if (node == NULL) {
        return;
    }

    node = node->first_child;
    while (node) {
        ALOGV("loadHwModules() loading module %s", node->name);
        loadHwModule(node);
        node = node->next;
    }
}

void AudioPolicyManagerBase::loadGlobalConfig(cnode *root)
{
    cnode *node = config_find(root, GLOBAL_CONFIG_TAG);
    if (node == NULL) {
        return;
    }
    node = node->first_child;
    while (node) {
        if (strcmp(ATTACHED_OUTPUT_DEVICES_TAG, node->name) == 0) {
            mAttachedOutputDevices = parseDeviceNames((char *)node->value);
            ALOGW_IF(mAttachedOutputDevices == AUDIO_DEVICE_NONE,
                    "loadGlobalConfig() no attached output devices");
            ALOGV("loadGlobalConfig() mAttachedOutputDevices %04x", mAttachedOutputDevices);
        } else if (strcmp(DEFAULT_OUTPUT_DEVICE_TAG, node->name) == 0) {
            mDefaultOutputDevice = (audio_devices_t)stringToEnum(sDeviceNameToEnumTable,
                                              ARRAY_SIZE(sDeviceNameToEnumTable),
                                              (char *)node->value);
            ALOGW_IF(mDefaultOutputDevice == AUDIO_DEVICE_NONE,
                    "loadGlobalConfig() default device not specified");
            ALOGV("loadGlobalConfig() mDefaultOutputDevice %04x", mDefaultOutputDevice);
        } else if (strcmp(ATTACHED_INPUT_DEVICES_TAG, node->name) == 0) {
            mAvailableInputDevices = parseDeviceNames((char *)node->value) & ~AUDIO_DEVICE_BIT_IN;
            ALOGV("loadGlobalConfig() mAvailableInputDevices %04x", mAvailableInputDevices);
        }
        node = node->next;
    }
}

status_t AudioPolicyManagerBase::loadAudioPolicyConfig(const char *path)
{
    cnode *root;
    char *data;

    data = (char *)load_file(path, NULL);
    if (data == NULL) {
        return -ENODEV;
    }
    root = config_node("", "");
    config_load(root, data);

    loadGlobalConfig(root);
    loadHwModules(root);

    config_free(root);
    free(root);
    free(data);

    ALOGI("loadAudioPolicyConfig() loaded %s\n", path);

    return NO_ERROR;
}

void AudioPolicyManagerBase::defaultAudioPolicyConfig(void)
{
    HwModule *module;
    IOProfile *profile;

    mDefaultOutputDevice = AUDIO_DEVICE_OUT_SPEAKER;
    mAttachedOutputDevices = AUDIO_DEVICE_OUT_SPEAKER;
    mAvailableInputDevices = AUDIO_DEVICE_IN_BUILTIN_MIC & ~AUDIO_DEVICE_BIT_IN;

    module = new HwModule("primary");

    profile = new IOProfile(module);
    profile->mSamplingRates.add(44100);
    profile->mFormats.add(AUDIO_FORMAT_PCM_16_BIT);
    profile->mChannelMasks.add(AUDIO_CHANNEL_OUT_STEREO);
    profile->mSupportedDevices = AUDIO_DEVICE_OUT_SPEAKER;
    profile->mFlags = AUDIO_OUTPUT_FLAG_PRIMARY;
    module->mOutputProfiles.add(profile);

    profile = new IOProfile(module);
    profile->mSamplingRates.add(8000);
    profile->mFormats.add(AUDIO_FORMAT_PCM_16_BIT);
    profile->mChannelMasks.add(AUDIO_CHANNEL_IN_MONO);
    profile->mSupportedDevices = AUDIO_DEVICE_IN_BUILTIN_MIC;
    module->mInputProfiles.add(profile);

    mHwModules.add(module);
}

}; // namespace android
