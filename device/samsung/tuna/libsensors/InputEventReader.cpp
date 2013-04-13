/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <cutils/log.h>

#include "InputEventReader.h"

/*****************************************************************************/

template <typename T>
static inline T min(T a, T b) {
    return a<b ? a : b;
}

struct input_event;

InputEventCircularReader::InputEventCircularReader(size_t numEvents)
    : mBuffer(new input_event[numEvents]),
      mBufferEnd(mBuffer + numEvents),
      mHead(mBuffer),
      mCurr(mBuffer),
      mEvents(numEvents),
      mFreeSpace(numEvents)
{
}

InputEventCircularReader::~InputEventCircularReader()
{
    delete [] mBuffer;
}

ssize_t InputEventCircularReader::fill(int fd)
{
    size_t numEventsRead = 0;
    if (mFreeSpace) {
        struct iovec iov[2];

        const size_t numFirst = min(mFreeSpace, (size_t)(mBufferEnd - mHead));
        const size_t numSecond = mFreeSpace - numFirst;

        int iovcnt = 1;
        iov[0].iov_base = mHead;
        iov[0].iov_len = numFirst * sizeof(input_event);

        if (numSecond > 0)
        {
            iovcnt++;
            iov[1].iov_base = mBuffer;
            iov[1].iov_len = numSecond * sizeof(input_event);
        }

        const ssize_t nread = readv(fd, iov, iovcnt);
        if (nread < 0 || nread % sizeof(input_event)) {
            // we got a partial event!!
            return nread < 0 ? -errno : -EINVAL;
        }

        numEventsRead = nread / sizeof(input_event);
        if (numEventsRead) {
            mHead += numEventsRead;
            mFreeSpace -= numEventsRead;
            if (mHead >= mBufferEnd)
                mHead -= mBufferEnd - mBuffer;
        }
    }

    return numEventsRead;
}

bool InputEventCircularReader::readEvent(int fd, input_event const** events)
{
    if (mFreeSpace >= mEvents) {
        ssize_t eventCount = fill(fd);
        if (eventCount <= 0)
            return false;
    }
    *events = mCurr;
    return true;
}

void InputEventCircularReader::next()
{
    mCurr++;
    mFreeSpace++;
    if (mCurr >= mBufferEnd) {
        mCurr = mBuffer;
    }
}
