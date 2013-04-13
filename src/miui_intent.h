/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  * See the License for the specific language governing permissions and * limitations under the License.
 */
#ifndef _MIUI_INTENT_H
#define _MIUI_INTENT_H


typedef enum _intentType{
    INTENT_MOUNT,
    INTENT_ISMOUNT,
    INTENT_UNMOUNT,
    INTENT_REBOOT,
    INTENT_POWEROFF,
    INTENT_INSTALL,
    INTENT_WIPE,
    INTENT_TOGGLE,
    INTENT_FORMAT,
    INTENT_RESTORE,
    INTENT_BACKUP,
    INTENT_ADVANCED_BACKUP,
    INTENT_SYSTEM,
    INTENT_COPY
}intentType;

#define INTENT_RESULT_LEN 16
typedef struct _intentResult{
    int ret;
    char result[INTENT_RESULT_LEN];
}intentResult, pintentResult;

typedef intentResult * (*intentFunction)(int argc, char *argv[]);
typedef struct _intentData{
    intentType type;
    intentFunction function;
}intentData, *pintentData;
typedef struct _miuiIntent{
    struct _intentData  *data;
    int alloc;
    int size;
}miuiIntent, *pmiuiIntent;
extern struct _intentResult intent_result;
int miuiIntent_init(int size);
int miuiIntent_register(intentType type, intentFunction function);
intentResult * miuiIntent_send(intentType type, int argc, char *args, ...);
intentResult*  miuiIntent_result_set(int ret, char *str);
char* miuiIntent_result_get_string();
int miuiIntent_result_get_int();
intentResult* intent_toggle(int argc, char *argv[]);
#endif
