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
 * @file        SEC_OMX_Vdec.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#ifndef SEC_OMX_VIDEO_DECODE
#define SEC_OMX_VIDEO_DECODE

#include "OMX_Component.h"
#include "SEC_OMX_Def.h"
#include "SEC_OSAL_Queue.h"
#include "SEC_OMX_Baseport.h"

#define MAX_VIDEO_INPUTBUFFER_NUM    5
#define MAX_VIDEO_OUTPUTBUFFER_NUM   2

#define DEFAULT_FRAME_WIDTH          176
#define DEFAULT_FRAME_HEIGHT         144

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    ((DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) * 2)
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   ((DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2)

#define MFC_INPUT_BUFFER_NUM_MAX         2
#define DEFAULT_MFC_INPUT_BUFFER_SIZE    ((1280 * 720 * 3) / 2)

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX    1
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX   3

#ifdef USE_ANDROID_EXTENSION
#define ANDROID_MAX_VIDEO_OUTPUTBUFFER_NUM   1
#endif

typedef struct
{
    void *pAddrY;
    void *pAddrC;
} MFC_DEC_ADDR_INFO;

typedef struct _SEC_MFC_NBDEC_THREAD
{
    OMX_HANDLETYPE  hNBDecodeThread;
    OMX_HANDLETYPE  hDecFrameStart;
    OMX_HANDLETYPE  hDecFrameEnd;
    OMX_BOOL        bExitDecodeThread;
    OMX_BOOL        bDecoderRun;

    OMX_U32         oneFrameSize;
} SEC_MFC_NBDEC_THREAD;

typedef struct _MFC_DEC_INPUT_BUFFER
{
    void *PhyAddr;      // physical address
    void *VirAddr;      // virtual address
    int   bufferSize;   // input buffer alloc size
    int   dataSize;     // Data length
} MFC_DEC_INPUT_BUFFER;

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
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN OMX_U32         nPort,
    OMX_IN OMX_HANDLETYPE  hTunneledComp,
    OMX_IN OMX_U32         nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup);
OMX_ERRORTYPE SEC_OMX_BufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE SEC_OMX_VideoDecodeGetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_VideoDecodeSetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure);
OMX_ERRORTYPE SEC_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif

#endif
