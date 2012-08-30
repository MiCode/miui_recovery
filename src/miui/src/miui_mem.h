/*
 * Copyright (C) 2011 xiaomi MIUI ( http://xiaomi.com/ )
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
 
//
// MALLOC WRAPPER
//
#ifndef __MIUI_MEM_H__
#define __MIUI_MEM_H__

#define __CURR_FILE() __FILE__
#define __CURR_LINE() __LINE__

extern void *miui_malloc(size_t size
#ifndef _MIUI_NODEBUG
, long line, char * filename
#endif
);

extern void * miui_realloc(void * x, size_t size
#ifndef _MIUI_NODEBUG
, long line, char * filename
#endif
);

void miui_memory_parentpid(int parent_pid);
void miui_memory_terminate(const char * message);

#ifndef malloc

#ifndef _MIUI_NODEBUG
  #define malloc(x) miui_malloc(x,__CURR_LINE(), __CURR_FILE())
#else
  #define malloc(x) miui_malloc(x)
#endif

#ifndef _MIUI_NODEBUG
  #define realloc(x,s) miui_realloc(x,s,__CURR_LINE(), __CURR_FILE())
#else
  #define realloc(x,s) miui_realloc(x,s)
#endif

void miui_free(void ** x);
#define free(x) miui_free((void **) &x)

#endif

#ifndef _MIUI_NODEBUG
  void miui_memory_debug_init();
  void miui_dump_malloc();
#endif

#endif
