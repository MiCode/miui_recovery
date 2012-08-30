/*
** Copyright 2008, The Android Open-Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AUDIO_HARDWARE_H
#define ANDROID_AUDIO_HARDWARE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/threads.h>
#include <utils/SortedVector.h>

#include <hardware_legacy/AudioHardwareBase.h>
#include <media/mediarecorder.h>
#include <hardware/audio_effect.h>

#include "secril-client.h"

#include <audio_utils/resampler.h>
#include <audio_utils/echo_reference.h>

extern "C" {
    struct pcm;
    struct mixer;
    struct mixer_ctl;
};

namespace android_audio_legacy {
    using android::AutoMutex;
    using android::Mutex;
    using android::RefBase;
    using android::SortedVector;
    using android::sp;
    using android::String16;
    using android::Vector;

// TODO: determine actual audio DSP and hardware latency
// Additionnal latency introduced by audio DSP and hardware in ms
#define AUDIO_HW_OUT_LATENCY_MS 0
// Default audio output sample rate
#define AUDIO_HW_OUT_SAMPLERATE 44100
// Default audio output channel mask
#define AUDIO_HW_OUT_CHANNELS (AudioSystem::CHANNEL_OUT_STEREO)
// Default audio output sample format
#define AUDIO_HW_OUT_FORMAT (AudioSystem::PCM_16_BIT)
// Kernel pcm out buffer size in frames at 44.1kHz
#define AUDIO_HW_OUT_PERIOD_SZ 1024
#define AUDIO_HW_OUT_PERIOD_CNT 4
// Default audio output buffer size in bytes
#define AUDIO_HW_OUT_PERIOD_BYTES (AUDIO_HW_OUT_PERIOD_SZ * 2 * sizeof(int16_t))

// Default audio input sample rate
#define AUDIO_HW_IN_SAMPLERATE 44100
// Default audio input channel mask
#define AUDIO_HW_IN_CHANNELS (AudioSystem::CHANNEL_IN_MONO)
// Default audio input sample format
#define AUDIO_HW_IN_FORMAT (AudioSystem::PCM_16_BIT)
// Kernel pcm in buffer size in frames at 44.1kHz (before resampling)
#define AUDIO_HW_IN_PERIOD_SZ 1024
#define AUDIO_HW_IN_PERIOD_CNT 4
// Default audio input buffer size in bytes (8kHz mono)
#define AUDIO_HW_IN_PERIOD_BYTES ((AUDIO_HW_IN_PERIOD_SZ*sizeof(int16_t))/8)


class AudioHardware : public AudioHardwareBase
{
    class AudioStreamOutALSA;
    class AudioStreamInALSA;

public:

    // input path names used to translate from input sources to driver paths
    static const char *inputPathNameDefault;
    static const char *inputPathNameCamcorder;
    static const char *inputPathNameVoiceRecognition;

    AudioHardware();
    virtual ~AudioHardware();
    virtual status_t initCheck();

    virtual status_t setVoiceVolume(float volume);
    virtual status_t setMasterVolume(float volume);

    virtual status_t setMode(int mode);

    virtual status_t setMicMute(bool state);
    virtual status_t getMicMute(bool* state);

    virtual status_t setParameters(const String8& keyValuePairs);
    virtual String8 getParameters(const String8& keys);

    virtual AudioStreamOut* openOutputStream(
        uint32_t devices, int *format=0, uint32_t *channels=0,
        uint32_t *sampleRate=0, status_t *status=0);

    virtual AudioStreamIn* openInputStream(
        uint32_t devices, int *format, uint32_t *channels,
        uint32_t *sampleRate, status_t *status,
        AudioSystem::audio_in_acoustics acoustics);

    virtual void closeOutputStream(AudioStreamOut* out);
    virtual void closeInputStream(AudioStreamIn* in);

    virtual size_t getInputBufferSize(
        uint32_t sampleRate, int format, int channelCount);

            int  mode() { return mMode; }
            const char *getOutputRouteFromDevice(uint32_t device);
            const char *getInputRouteFromDevice(uint32_t device);
            const char *getVoiceRouteFromDevice(uint32_t device);

            status_t setIncallPath_l(uint32_t device);

            status_t setInputSource_l(audio_source source);

            void setVoiceVolume_l(float volume);

    static uint32_t    getInputSampleRate(uint32_t sampleRate);
           sp <AudioStreamInALSA> getActiveInput_l();

           Mutex& lock() { return mLock; }

           struct pcm *openPcmOut_l();
           void closePcmOut_l();

           struct mixer *openMixer_l();
           void closeMixer_l();

           sp <AudioStreamOutALSA>  output() { return mOutput; }

           struct echo_reference_itfe *getEchoReference(audio_format_t format,
                                          uint32_t channelCount,
                                          uint32_t samplingRate);
           void releaseEchoReference(struct echo_reference_itfe *reference);

protected:
    virtual status_t dump(int fd, const Vector<String16>& args);

private:

    enum tty_modes {
        TTY_MODE_OFF,
        TTY_MODE_VCO,
        TTY_MODE_HCO,
        TTY_MODE_FULL
    };

    bool            mInit;
    bool            mMicMute;
    sp <AudioStreamOutALSA>                 mOutput;
    SortedVector < sp<AudioStreamInALSA> >   mInputs;
    Mutex           mLock;
    struct pcm*     mPcm;
    struct mixer*   mMixer;
    uint32_t        mPcmOpenCnt;
    uint32_t        mMixerOpenCnt;
    bool            mInCallAudioMode;
    float           mVoiceVol;

    audio_source    mInputSource;
    bool            mBluetoothNrec;
    int             mTTYMode;

    void*           mSecRilLibHandle;
    HRilClient      mRilClient;
    bool            mActivatedCP;
    HRilClient      (*openClientRILD)  (void);
    int             (*disconnectRILD)  (HRilClient);
    int             (*closeClientRILD) (HRilClient);
    int             (*isConnectedRILD) (HRilClient);
    int             (*connectRILD)     (HRilClient);
    int             (*setCallVolume)   (HRilClient, SoundType, int);
    int             (*setCallAudioPath)(HRilClient, AudioPath);
    int             (*setCallClockSync)(HRilClient, SoundClockCondition);
    void            loadRILD(void);
    status_t        connectRILDIfRequired(void);
    struct echo_reference_itfe *mEchoReference;

    //  trace driver operations for dump
    int             mDriverOp;

    static uint32_t         checkInputSampleRate(uint32_t sampleRate);

    // column index in inputConfigTable[][]
    enum {
        INPUT_CONFIG_SAMPLE_RATE,
        INPUT_CONFIG_BUFFER_RATIO,
        INPUT_CONFIG_CNT
    };

    // contains the list of valid sampling rates for input streams as well as the ratio
    // between the kernel buffer size and audio hal buffer size for each sampling rate
    static const uint32_t  inputConfigTable[][INPUT_CONFIG_CNT];

    class AudioStreamOutALSA : public AudioStreamOut, public RefBase
    {
    public:
        AudioStreamOutALSA();
        virtual ~AudioStreamOutALSA();
        status_t set(AudioHardware* mHardware,
                     uint32_t devices,
                     int *pFormat,
                     uint32_t *pChannels,
                     uint32_t *pRate);
        virtual uint32_t sampleRate()
            const { return mSampleRate; }
        virtual size_t bufferSize()
            const { return mBufferSize; }
        virtual uint32_t channels()
            const { return mChannels; }
        virtual int format()
            const { return AUDIO_HW_OUT_FORMAT; }
        virtual uint32_t latency()
            const { return (1000 * AUDIO_HW_OUT_PERIOD_CNT *
                            (bufferSize()/frameSize()))/sampleRate() +
                AUDIO_HW_OUT_LATENCY_MS; }
        virtual status_t setVolume(float left, float right)
        { return INVALID_OPERATION; }
        virtual ssize_t write(const void* buffer, size_t bytes);
        virtual status_t standby();
                bool checkStandby();

        virtual status_t dump(int fd, const Vector<String16>& args);
        virtual status_t setParameters(const String8& keyValuePairs);
        virtual String8 getParameters(const String8& keys);
        uint32_t device() { return mDevices; }
        virtual status_t getRenderPosition(uint32_t *dspFrames);

                void doStandby_l();
                void close_l();
                status_t open_l();
                int standbyCnt() { return mStandbyCnt; }

                int prepareLock();
                void lock();
                void unlock();

                void addEchoReference(struct echo_reference_itfe *reference);
                void removeEchoReference(struct echo_reference_itfe *reference);

    private:

                int computeEchoReferenceDelay(size_t frames, struct timespec *echoRefRenderTime);
                int getPlaybackDelay(size_t frames, struct echo_reference_buffer *buffer);

        Mutex mLock;
        AudioHardware* mHardware;
        struct pcm *mPcm;
        struct mixer *mMixer;
        struct mixer_ctl *mRouteCtl;
        const char *next_route;
        bool mStandby;
        uint32_t mDevices;
        uint32_t mChannels;
        uint32_t mSampleRate;
        size_t mBufferSize;
        //  trace driver operations for dump
        int mDriverOp;
        int mStandbyCnt;
        bool mSleepReq;
        struct echo_reference_itfe *mEchoReference;
    };

    class AudioStreamInALSA : public AudioStreamIn, public RefBase
    {

     public:
                    AudioStreamInALSA();
        virtual     ~AudioStreamInALSA();
        status_t    set(AudioHardware* hw,
                    uint32_t devices,
                    int *pFormat,
                    uint32_t *pChannels,
                    uint32_t *pRate,
                    AudioSystem::audio_in_acoustics acoustics);
        virtual size_t bufferSize() const { return mBufferSize; }
        virtual uint32_t channels() const { return mChannels; }
        virtual int format() const { return AUDIO_HW_IN_FORMAT; }
        virtual uint32_t sampleRate() const { return mSampleRate; }
        virtual status_t setGain(float gain) { return INVALID_OPERATION; }
        virtual ssize_t read(void* buffer, ssize_t bytes);
        virtual status_t dump(int fd, const Vector<String16>& args);
        virtual status_t standby();
                bool checkStandby();
        virtual status_t setParameters(const String8& keyValuePairs);
        virtual String8 getParameters(const String8& keys);
        virtual unsigned int getInputFramesLost() const { return 0; }
        virtual status_t    addAudioEffect(effect_handle_t effect);
        virtual status_t    removeAudioEffect(effect_handle_t effect);

                uint32_t device() { return mDevices; }
                void doStandby_l();
                void close_l();
                status_t open_l();
                int standbyCnt() { return mStandbyCnt; }

        static size_t getBufferSize(uint32_t sampleRate, int channelCount);

        // resampler_buffer_provider
        static int getNextBufferStatic(struct resampler_buffer_provider *provider,
                             struct resampler_buffer* buffer);
        static void releaseBufferStatic(struct resampler_buffer_provider *provider,
                             struct resampler_buffer* buffer);

        int prepareLock();
        void lock();
        void unlock();

     private:

        struct ResamplerBufferProvider {
            struct resampler_buffer_provider mProvider;
            AudioStreamInALSA *mInputStream;
        };

        ssize_t readFrames(void* buffer, ssize_t frames);
        ssize_t processFrames(void* buffer, ssize_t frames);
        int32_t updateEchoReference(size_t frames);
        void pushEchoReference(size_t frames);
        void updateEchoDelay(size_t frames, struct timespec *echoRefRenderTime);
        void getCaptureDelay(size_t frames, struct echo_reference_buffer *buffer);
        status_t setPreProcessorEchoDelay(effect_handle_t handle, int32_t delayUs);
        status_t setPreprocessorParam(effect_handle_t handle, effect_param_t *param);

        // BufferProvider
        status_t getNextBuffer(struct resampler_buffer* buffer);
        void releaseBuffer(struct resampler_buffer* buffer);

        Mutex mLock;
        AudioHardware* mHardware;
        struct pcm *mPcm;
        struct mixer *mMixer;
        struct mixer_ctl *mRouteCtl;
        const char *next_route;
        bool mStandby;
        uint32_t mDevices;
        uint32_t mChannels;
        uint32_t mChannelCount;
        uint32_t mSampleRate;
        size_t mBufferSize;
        struct resampler_itfe *mDownSampler;
        struct ResamplerBufferProvider mBufferProvider;
        status_t mReadStatus;
        size_t mInputFramesIn;
        int16_t *mInputBuf;
        //  trace driver operations for dump
        int mDriverOp;
        int mStandbyCnt;
        bool mSleepReq;
        SortedVector<effect_handle_t> mPreprocessors;
        int16_t *mProcBuf;
        size_t mProcBufSize;
        size_t mProcFramesIn;
        int16_t *mRefBuf;
        size_t mRefBufSize;
        size_t mRefFramesIn;
        struct echo_reference_itfe *mEchoReference;
        bool mNeedEchoReference;
    };

};

}; // namespace android

#endif
