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
 * @file        SEC_OMX_Venc.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OMX_VIDEO_ENCODE
#define SEC_OMX_VIDEO_ENCODE

#include "OMX_Component.h"
#include "SEC_OMX_Def.h"
#include "SEC_OSAL_Queue.h"
#include "SEC_OMX_Baseport.h"

#define MAX_VIDEO_INPUTBUFFER_NUM    5
#define MAX_VIDEO_OUTPUTBUFFER_NUM   4

#define DEFAULT_FRAME_WIDTH          176
#define DEFAULT_FRAME_HEIGHT         144

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    ALIGN_TO_8KB(ALIGN_TO_128B(DEFAULT_FRAME_WIDTH) * ALIGN_TO_32B(DEFAULT_FRAME_HEIGHT)) \
                                           + ALIGN_TO_8KB(ALIGN_TO_128B(DEFAULT_FRAME_WIDTH) * ALIGN_TO_32B(DEFAULT_FRAME_HEIGHT / 2))
                                           /* (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2 */
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   DEFAULT_VIDEO_INPUT_BUFFER_SIZE

#define MFC_INPUT_BUFFER_NUM_MAX            2

#ifdef USE_ANDROID_EXTENSION
#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    4
#else
#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    3
#endif
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   1

#ifdef USE_ANDROID_EXTENSION
// The largest metadata buffer size advertised
// when metadata buffer mode is used for video encoding
#define  MAX_INPUT_METADATA_BUFFER_SIZE (64)
#endif

typedef struct
{
    void *pAddrY;
    void *pAddrC;
} MFC_ENC_ADDR_INFO;

typedef struct _SEC_MFC_NBENC_THREAD
{
    OMX_HANDLETYPE  hNBEncodeThread;
    OMX_HANDLETYPE  hEncFrameStart;
    OMX_HANDLETYPE  hEncFrameEnd;
    OMX_BOOL        bExitEncodeThread;
    OMX_BOOL        bEncoderRun;
} SEC_MFC_NBENC_THREAD;

typedef struct _MFC_ENC_INPUT_BUFFER
{
    void *YPhyAddr; // physical address of Y
    void *CPhyAddr; // physical address of CbCr
    void *YVirAddr; // virtual address of Y
    void *CVirAddr; // virtual address of CbCr
    int YBufferSize; // input buffer alloc size of Y
    int CBufferSize; // input buffer alloc size of CbCr
    int YDataSize;  // input size of Y data
    int CDataSize;  // input size of CbCr data
} MFC_ENC_INPUT_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE SEC_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes,
    OMX_IN OMX_U8                   *pBuffer);
OMX_ERRORTYPE SEC_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE            hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32                   nPortIndex,
    OMX_IN OMX_PTR                   pAppPrivate,
    OMX_IN OMX_U32                   nSizeBytes);
OMX_ERRORTYPE SEC_OMX_FreeBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32        nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr);
OMX_ERRORTYPE SEC_OMX_AllocateTunnelBuffer(
    SEC_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE SEC_OMX_FreeTunnelBuffer(
    SEC_OMX_BASEPORT *pOMXBasePort,
    OMX_U32           nPortIndex);
OMX_ERRORTYPE SEC_OMX_ComponentTunnelRequest(
    OMX_IN OMX_HANDLETYPE  hComp,
    OMX_IN OMX_U32         nPort,
    OMX_IN OMX_HANDLETYPE  hTunneledComp,
    OMX_IN OMX_U32         nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup);
OMX_ERRORTYPE SEC_OMX_BufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_VideoEncodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_VideoEncodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_VideoEncodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif

#endif
