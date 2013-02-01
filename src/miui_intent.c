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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "miui/src/miui.h"
#include "miui_intent.h"

struct _intentResult intent_result;
struct _intentResult* const pret = &intent_result;
static struct _miuiIntent miui_intent = {
                                     .alloc = 0,
                                     .size = 0
};
static struct _miuiIntent * const pint = &miui_intent;
#define INTENT_ALLOC 20
int miuiIntent_init(int size)
{
    if (size < INTENT_ALLOC)
        pint->alloc = INTENT_ALLOC;
    else pint->alloc = size;
    pint->size = 0;
    pint->data =  malloc(pint->alloc * sizeof(struct _intentData));
    return_val_if_fail(pint->data != NULL, -1);
    return 0;
}
int miuiIntent_register(intentType type, intentFunction function)
{
    if (pint->size >= pint->alloc) {
        pint->alloc = 2 * pint->alloc;
        pint->data = malloc(pint->alloc * sizeof(struct _intentData));
        assert_if_fail(pint->data != NULL);
    }
    int i = 0;
    for (i = 0; i < pint->size; i++)
    {
        return_val_if_fail(pint->data[i].type != type, -1);	
    }
    pint->data[pint->size].type = type;
    pint->data[pint->size].function = function;
    pint->size++;
    return 0;
}

//miui_send intent args must >= 1,

intentResult* miuiIntent_send(intentType type, int argc, char *format, ...)
{
    int i = 0;
    for (i = 0; i < pint->size; i++)
    {
        if (pint->data[i].type == type)
            break;
    }
    struct _intentResult *result;
    int args_i =0; 
    va_list arg_ptr; 
    char **args = (char**)malloc(argc * sizeof(char *)); 
    va_start(arg_ptr, format);
    args[0] = format;
    for (args_i = 1; args_i < argc; args_i++)
        args[args_i] = va_arg(arg_ptr, char*);
    result = pint->data[i].function(argc, args);
    va_end(arg_ptr); 
    free(args);
   return result;
}

intentResult* miuiIntent_result_set(int ret, char *str)
{
    pret->ret = ret;
    if (str != NULL) snprintf(pret->result, INTENT_RESULT_LEN, "%s", str);
    else pret->result[0] = '\0';
    return pret;
}

char* miuiIntent_result_get_string()
{
    return pret->result;
}
int miuiIntent_result_get_int()
{
    return pret->ret;
}
