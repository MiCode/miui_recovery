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

#ifndef SAMSUNG_SENSORBASE_H
#define SAMSUNG_SENSORBASE_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "SamsungSensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

class SamsungSensorBase:public SensorBase {
protected:
    bool mEnabled;
    bool mHasPendingEvent;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvent;
    char *mInputSysfsEnable;
    char *mInputSysfsPollDelay;
    int mSensorCode;
    pthread_mutex_t mLock;

    char *makeSysfsName(const char *input_name,
                        const char *input_file);

    virtual int handleEnable(int en);
    virtual bool handleEvent(input_event const * event);

public:
    SamsungSensorBase(const char* dev_name,
                      const char* data_name,
                      int sensor_code);

    virtual ~SamsungSensorBase();
    virtual int enable(int32_t handle, int en);
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int readEvents(sensors_event_t *data, int count);
};
#endif /* SAMSUNG_SENSORBASE_H */
