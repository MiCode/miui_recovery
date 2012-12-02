/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef _BUILD_TOOLS_APPLYPATCH_UTILS_H
#define _BUILD_TOOLS_APPLYPATCH_UTILS_H

#include <stdio.h>

// Read and write little-endian values of various sizes.

void Write4(int value, FILE* f);
void Write8(long long value, FILE* f);
int Read2(void* p);
int Read4(void* p);
long long Read8(void* p);

#endif //  _BUILD_TOOLS_APPLYPATCH_UTILS_H
