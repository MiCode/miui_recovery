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
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <pthread.h>

#include <cutils/log.h>

#include "ProximitySensor.h"

/*****************************************************************************/

ProximitySensor::ProximitySensor()
    : SamsungSensorBase(NULL, "proximity", ABS_DISTANCE)
{
    mPendingEvent.sensor = ID_P;
    mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
}

int ProximitySensor::setDelay(int32_t handle, int64_t ns)
{
    return -1;
}

float ProximitySensor::indexToValue(size_t index) const
{
    return index * PROXIMITY_THRESHOLD_GP2A;
}

bool ProximitySensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int ProximitySensor::handleEnable(int en) {
    if (!en)
        return 0;

    struct input_absinfo absinfo;
    if (!ioctl(data_fd, EVIOCGABS(ABS_DISTANCE), &absinfo)) {
        mHasPendingEvent = true;
        mPendingEvent.distance = indexToValue(absinfo.value);
        return 0;
    } else {
        return -1;
    }
}

bool ProximitySensor::handleEvent(input_event const *event) {
    mPendingEvent.distance = indexToValue(event->value);
    return true;
}
