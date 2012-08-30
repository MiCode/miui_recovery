/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        SEC_OSAL_ETC.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OSAL_ETC
#define SEC_OSAL_ETC

#include "OMX_Types.h"


#ifdef __cplusplus
extern "C" {
#endif

OMX_PTR SEC_OSAL_Strcpy(OMX_PTR dest, OMX_PTR src);
OMX_PTR SEC_OSAL_Strncpy(OMX_PTR dest, OMX_PTR src, size_t num);
OMX_S32 SEC_OSAL_Strcmp(OMX_PTR str1, OMX_PTR str2);
OMX_PTR SEC_OSAL_Strcat(OMX_PTR dest, OMX_PTR src);
size_t SEC_OSAL_Strlen(const char *str);
ssize_t getline(char **ppLine, size_t *len, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
