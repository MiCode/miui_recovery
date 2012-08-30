/*
 * Copyright (C) 2011 Samsung
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <pthread.h>

#include "SamsungSensorBase.h"

char *SamsungSensorBase::makeSysfsName(const char *input_name,
                                       const char *file_name) {
    char *name;
    int length = strlen("/sys/class/input/") +
        strlen(input_name) +
        strlen("/device/") +
        strlen(file_name);

    name = new char[length + 1];
    if (name) {
        strcpy(name, "/sys/class/input/");
        strcat(name, input_name);
        strcat(name, "/device/");
        strcat(name, file_name);
    }

    return name;
}

bool SamsungSensorBase::handleEvent(input_event const * event) {
    return true;
}

int SamsungSensorBase::handleEnable(int en) {
    return 0;
}

SamsungSensorBase::SamsungSensorBase(const char *dev_name,
                                     const char *data_name,
                                     int sensor_code)
    : SensorBase(dev_name, data_name),
      mEnabled(true),
      mHasPendingEvent(false),
      mInputReader(4),
      mSensorCode(sensor_code),
      mLock(PTHREAD_MUTEX_INITIALIZER)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    if (!data_fd)
        return;
    mInputSysfsEnable = makeSysfsName(input_name, "enable");
    if (!mInputSysfsEnable) {
        LOGE("%s: unable to allocate mem for %s:enable", __func__,
             data_name);
        return;
    }
    mInputSysfsPollDelay = makeSysfsName(input_name, "poll_delay");
    if (!mInputSysfsPollDelay) {
        LOGE("%s: unable to allocate mem for %s:poll_delay", __func__,
             data_name);
        return;
    }

    int flags = fcntl(data_fd, F_GETFL, 0);
    fcntl(data_fd, F_SETFL, flags | O_NONBLOCK);

    enable(0, 0);
}

SamsungSensorBase::~SamsungSensorBase() {
    if (mEnabled) {
        enable(0, 0);
    }
    delete[] mInputSysfsEnable;
    delete[] mInputSysfsPollDelay;
}

int SamsungSensorBase::enable(int32_t handle, int en)
{
    int err = 0;
    pthread_mutex_lock(&mLock);
    if (en != mEnabled) {
        int fd;
        fd = open(mInputSysfsEnable, O_RDWR);
        if (fd >= 0) {
            err = write(fd, en ? "1" : "0", 2);
            close(fd);
            if (err < 0) {
                goto cleanup;
            }
            mEnabled = en;
            err = handleEnable(en);
        } else {
            err = -1;
        }
    }
cleanup:
    pthread_mutex_unlock(&mLock);
    return err;
}

int SamsungSensorBase::setDelay(int32_t handle, int64_t ns)
{
    int fd;
    int result = 0;
    char buf[21];
    pthread_mutex_lock(&mLock);
    fd = open(mInputSysfsPollDelay, O_RDWR);
    if (fd < 0) {
        result = -1;
        goto done;
    }
    sprintf(buf, "%lld", ns);
    write(fd, buf, strlen(buf)+1);
    close(fd);
done:
    pthread_mutex_unlock(&mLock);
    return result;
}

int SamsungSensorBase::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    pthread_mutex_lock(&mLock);
    int numEventReceived = 0;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        if (mEnabled) {
            mPendingEvent.timestamp = getTimestamp();
            *data = mPendingEvent;
            numEventReceived++;
        }
        goto done;
    }

    input_event const* event;
    while (count && mInputReader.readEvent(data_fd, &event)) {
        if (event->type == EV_ABS) {
            if (event->code == mSensorCode) {
                if (mEnabled && handleEvent(event)) {
                    mPendingEvent.timestamp = timevalToNano(event->time);
                    *data++ = mPendingEvent;
                    count--;
                    numEventReceived++;
                }
            }
        }
        mInputReader.next();
    }

done:
    pthread_mutex_unlock(&mLock);
    return numEventReceived;

}
