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
 * @file        SEC_OSAL_Buffer.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.0.2
 * @history
 *   2011.5.15 : Create
 */

#ifndef SEC_OSAL_BUFFER
#define SEC_OSAL_BUFFER

#ifdef __cplusplus
extern "C" {
#endif

#include "OMX_Types.h"

typedef struct {
    void *YPhyAddr;                     // [IN/OUT] physical address of Y
    void *CPhyAddr;                     // [IN/OUT] physical address of CbCr
    void *YVirAddr;                     // [IN/OUT] virtual address of Y
    void *CVirAddr;                     // [IN/OUT] virtual address of CbCr
    int YSize;                          // [IN/OUT] input size of Y data
    int CSize;                          // [IN/OUT] input size of CbCr data
} BUFFER_ADDRESS_INFO;


OMX_ERRORTYPE checkVersionANB(OMX_PTR ComponentParameterStructure);
OMX_U32 checkPortIndexANB(OMX_PTR ComponentParameterStructure);
OMX_U32 getMetadataBufferType(const uint8_t *ptr);
OMX_ERRORTYPE enableAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE getAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE useAndroidNativeBuffer(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure);
OMX_U32 getVADDRfromANB(OMX_PTR pUnreadableBuffer, OMX_U32 Width, OMX_U32 Height, void *vaddress[]);
OMX_U32 putVADDRtoANB(OMX_PTR pUnreadableBuffer);
OMX_ERRORTYPE enableStoreMetaDataInBuffers(OMX_HANDLETYPE hComponent, OMX_PTR ComponentParameterStructure);
OMX_BOOL isMetadataBufferTypeGrallocSource(OMX_BYTE pInputDataBuffer);
OMX_ERRORTYPE preprocessMetaDataInBuffers(OMX_HANDLETYPE hComponent, OMX_BYTE pInputDataBuffer, BUFFER_ADDRESS_INFO *pInputInfo);

#ifdef __cplusplus
}
#endif

#endif

