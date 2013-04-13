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
#include <cutils/log.h>
#include <pthread.h>

#include "LightSensor.h"

LightSensor::LightSensor()
    : SamsungSensorBase(NULL, "lightsensor-level", ABS_MISC)
{
    mPendingEvent.sensor = ID_L;
    mPendingEvent.type = SENSOR_TYPE_LIGHT;
}

bool LightSensor::handleEvent(input_event const *event) {
    if (event->value == -1) {
        return false;
    }
    // Convert adc value to lux assuming:
    // I = 10 * log(Ev) uA
    // R = 24kOhm
    // Max adc value 1023 = 1.25V
    // 1/4 of light reaches sensor
    mPendingEvent.light = powf(10, event->value * (125.0f / 1023.0f / 24.0f)) * 4;
    return true;
}
