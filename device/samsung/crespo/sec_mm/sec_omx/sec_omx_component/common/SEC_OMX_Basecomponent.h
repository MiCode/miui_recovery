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
 * @file       SEC_OMX_Basecomponent.h
 * @brief
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    1.0
 * @history
 *    2010.7.15 : Create
 */

#ifndef SEC_OMX_BASECOMP
#define SEC_OMX_BASECOMP

#include "SEC_OMX_Def.h"
#include "OMX_Component.h"
#include "SEC_OSAL_Queue.h"
#include "SEC_OMX_Baseport.h"


typedef struct _SEC_OMX_MESSAGE
{
    OMX_U32 messageType;
    OMX_U32 messageParam;
    OMX_PTR pCmdData;
} SEC_OMX_MESSAGE;

typedef struct _SEC_OMX_DATABUFFER
{
    OMX_HANDLETYPE        bufferMutex;
    OMX_BUFFERHEADERTYPE* bufferHeader;
    OMX_BOOL              dataValid;
    OMX_U32               allocSize;
    OMX_U32               dataLen;
    OMX_U32               usedDataLen;
    OMX_U32               remainDataLen;
    OMX_U32               nFlags;
    OMX_TICKS             timeStamp;
} SEC_OMX_DATABUFFER;

typedef struct _SEC_BUFFER_HEADER{
    void *YPhyAddr; // [IN/OUT] physical address of Y
    void *CPhyAddr; // [IN/OUT] physical address of CbCr
    void *YVirAddr; // [IN/OUT] virtual address of Y
    void *CVirAddr; // [IN/OUT] virtual address of CbCr
    int YSize;      // [IN/OUT] input size of Y data
    int CSize;      // [IN/OUT] input size of CbCr data
} SEC_BUFFER_HEADER;

typedef struct _SEC_OMX_DATA
{
    OMX_BYTE  dataBuffer;
    OMX_U32   allocSize;
    OMX_U32   dataLen;
    OMX_U32   usedDataLen;
    OMX_U32   remainDataLen;
    OMX_U32   previousDataLen;
    OMX_U32   nFlags;
    OMX_TICKS timeStamp;
    SEC_BUFFER_HEADER specificBufferHeader;
} SEC_OMX_DATA;

/* for Check TimeStamp after Seek */
typedef struct _SEC_OMX_TIMESTAPM
{
    OMX_BOOL  needSetStartTimeStamp;
    OMX_BOOL  needCheckStartTimeStamp;
    OMX_TICKS startTimeStamp;
    OMX_U32   nStartFlags;
} SEC_OMX_TIMESTAMP;

typedef struct _SEC_OMX_BASECOMPONENT
{
    OMX_STRING               componentName;
    OMX_VERSIONTYPE          componentVersion;
    OMX_VERSIONTYPE          specVersion;

    OMX_STATETYPE            currentState;
    SEC_OMX_TRANS_STATETYPE  transientState;

    SEC_CODEC_TYPE           codecType;
    SEC_OMX_PRIORITYMGMTTYPE compPriority;
    OMX_MARKTYPE             propagateMarkType;
    OMX_HANDLETYPE           compMutex;

    OMX_HANDLETYPE           hCodecHandle;

    /* Message Handler */
    OMX_BOOL                 bExitMessageHandlerThread;
    OMX_HANDLETYPE           hMessageHandler;
    OMX_HANDLETYPE           msgSemaphoreHandle;
    SEC_QUEUE                messageQ;

    /* Buffer Process */
    OMX_BOOL                 bExitBufferProcessThread;
    OMX_HANDLETYPE           hBufferProcess;

    /* Buffer */
    SEC_OMX_DATABUFFER       secDataBuffer[2];

    /* Data */
    SEC_OMX_DATA             processData[2];

    /* Port */
    OMX_PORT_PARAM_TYPE      portParam;
    SEC_OMX_BASEPORT        *pSECPort;

    OMX_HANDLETYPE           pauseEvent;

    /* Callback function */
    OMX_CALLBACKTYPE        *pCallbacks;
    OMX_PTR                  callbackData;

    /* Save Timestamp */
    OMX_TICKS                timeStamp[MAX_TIMESTAMP];
    SEC_OMX_TIMESTAMP        checkTimeStamp;

    /* Save Flags */
    OMX_U32                  nFlags[MAX_FLAGS];

    OMX_BOOL                 getAllDelayBuffer;
    OMX_BOOL                 remainOutputData;
    OMX_BOOL                 reInputData;

    /* Android CapabilityFlags */
    OMXComponentCapabilityFlagsType capabilityFlags;

    OMX_BOOL bUseFlagEOF;
    OMX_BOOL bSaveFlagEOS;

    OMX_ERRORTYPE (*sec_mfc_componentInit)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_mfc_componentTerminate)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_mfc_bufferProcess) (OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData);

    OMX_ERRORTYPE (*sec_AllocateTunnelBuffer)(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_FreeTunnelBuffer)(SEC_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_BufferProcess)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*sec_BufferReset)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*sec_InputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);
    OMX_ERRORTYPE (*sec_OutputBufferReturn)(OMX_COMPONENTTYPE *pOMXComponent);

    int (*sec_checkInputFrame)(unsigned char *pInputStream, int buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame);

} SEC_OMX_BASECOMPONENT;


#ifdef __cplusplus
extern "C" {
#endif

    OMX_ERRORTYPE SEC_OMX_Check_SizeVersion(OMX_PTR header, OMX_U32 size);


#ifdef __cplusplus
};
#endif

#endif
