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
 * @file        SEC_OMX_H264dec.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEC_OMX_Macros.h"
#include "SEC_OMX_Basecomponent.h"
#include "SEC_OMX_Baseport.h"
#include "SEC_OMX_Vdec.h"
#include "library_register.h"
#include "SEC_OMX_H264dec.h"
#include "SsbSipMfcApi.h"
#include "color_space_convertor.h"

#undef  SEC_LOG_TAG
#define SEC_LOG_TAG    "SEC_H264_DEC"
#define SEC_LOG_OFF
#include "SEC_OSAL_Log.h"

//#define ADD_SPS_PPS_I_FRAME
//#define FULL_FRAME_SEARCH

/* H.264 Decoder Supported Levels & profiles */
SEC_OMX_VIDEO_PROFILELEVEL supportedAVCProfileLevels[] ={
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},

    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31}};


static int Check_H264_Frame(OMX_U8 *pInputStream, int buffSize, OMX_U32 flag, OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame)
{
    OMX_U32  preFourByte       = (OMX_U32)-1;
    int      accessUnitSize    = 0;
    int      frameTypeBoundary = 0;
    int      nextNaluSize      = 0;
    int      naluStart         = 0;

    if (bPreviousFrameEOF == OMX_TRUE)
        naluStart = 0;
    else
        naluStart = 1;

    while (1) {
        int inputOneByte = 0;

        if (accessUnitSize == buffSize)
            goto EXIT;

        inputOneByte = *(pInputStream++);
        accessUnitSize += 1;

        if (preFourByte == 0x00000001 || (preFourByte << 8) == 0x00000100) {
            int naluType = inputOneByte & 0x1F;

            SEC_OSAL_Log(SEC_LOG_TRACE, "NaluType : %d", naluType);
            if (naluStart == 0) {
#ifdef ADD_SPS_PPS_I_FRAME
                if (naluType == 1 || naluType == 5)
#else
                if (naluType == 1 || naluType == 5 || naluType == 7 || naluType == 8)
#endif
                    naluStart = 1;
            } else {
#ifdef OLD_DETECT
                frameTypeBoundary = (8 - naluType) & (naluType - 10); //AUD(9)
#else
                if (naluType == 9)
                    frameTypeBoundary = -2;
#endif
                if (naluType == 1 || naluType == 5) {
                    if (accessUnitSize == buffSize) {
                        accessUnitSize--;
                        goto EXIT;
                    }
                    inputOneByte = *pInputStream++;
                    accessUnitSize += 1;

                    if (inputOneByte >= 0x80)
                        frameTypeBoundary = -1;
                }
                if (frameTypeBoundary < 0) {
                    break;
                }
            }

        }
        preFourByte = (preFourByte << 8) + inputOneByte;
    }

    *pbEndOfFrame = OMX_TRUE;
    nextNaluSize = -5;
    if (frameTypeBoundary == -1)
        nextNaluSize = -6;
    if (preFourByte != 0x00000001)
        nextNaluSize++;
    return (accessUnitSize + nextNaluSize);

EXIT:
    *pbEndOfFrame = OMX_FALSE;

    return accessUnitSize;
}

OMX_BOOL Check_H264_StartCode(OMX_U8 *pInputStream, OMX_U32 streamSize)
{
    if (streamSize < 4) {
        return OMX_FALSE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] == 0x00) &&
              (pInputStream[3] != 0x00) &&
              ((pInputStream[3] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] != 0x00) &&
              ((pInputStream[2] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}

OMX_ERRORTYPE SEC_MFC_H264Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_StateInvalid;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstAVCComponent->nPortIndex];

        SEC_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        SEC_OSAL_Strcpy((char *)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        SEC_OMX_VIDEO_PROFILELEVEL *pProfileLevel = NULL;
        OMX_U32 maxProfileLevelNum = 0;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pProfileLevel = supportedAVCProfileLevels;
        maxProfileLevelNum = sizeof(supportedAVCProfileLevels) / sizeof(SEC_OMX_VIDEO_PROFILELEVEL);

        if (pDstProfileLevel->nProfileIndex >= maxProfileLevelNum) {
            ret = OMX_ErrorNoMore;
            goto EXIT;
        }

        pProfileLevel += pDstProfileLevel->nProfileIndex;
        pDstProfileLevel->eProfile = pProfileLevel->profile;
        pDstProfileLevel->eLevel = pProfileLevel->level;
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        pSrcAVCComponent = &pH264Dec->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pSECComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_StateInvalid;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcAVCComponent->nPortIndex];

        SEC_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = SEC_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!SEC_OSAL_Strcmp((char*)pComponentRole->cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE)) {
            pSECComponent->pSECPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                       portIndex = pPortDefinition->nPortIndex;
        SEC_OMX_BASEPORT             *pSECPort;
        OMX_U32 width, height, size;

        if (portIndex >= pSECComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = SEC_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pSECPort = &pSECComponent->pSECPort[portIndex];

        if ((pSECComponent->currentState != OMX_StateLoaded) && (pSECComponent->currentState != OMX_StateWaitForResources)) {
            if (pSECPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }
        if(pPortDefinition->nBufferCountActual < pSECPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        SEC_OSAL_Memcpy(&pSECPort->portDefinition, pPortDefinition, pPortDefinition->nSize);

        width = ((pSECPort->portDefinition.format.video.nFrameWidth + 15) & (~15));
        height = ((pSECPort->portDefinition.format.video.nFrameHeight + 15) & (~15));
        size = (width * height * 3) / 2;
        pSECPort->portDefinition.format.video.nStride = width;
        pSECPort->portDefinition.format.video.nSliceHeight = height;
        pSECPort->portDefinition.nBufferSize = (size > pSECPort->portDefinition.nBufferSize) ? size : pSECPort->portDefinition.nBufferSize;

        if (portIndex == INPUT_PORT_INDEX) {
            SEC_OMX_BASEPORT *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
            pSECOutputPort->portDefinition.format.video.nFrameWidth = pSECPort->portDefinition.format.video.nFrameWidth;
            pSECOutputPort->portDefinition.format.video.nFrameHeight = pSECPort->portDefinition.format.video.nFrameHeight;
            pSECOutputPort->portDefinition.format.video.nStride = width;
            pSECOutputPort->portDefinition.format.video.nSliceHeight = height;

            switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
                pSECOutputPort->portDefinition.nBufferSize = (width * height * 3) / 2;
                break;
            default:
                SEC_OSAL_Log(SEC_LOG_ERROR, "Color format is not support!! use default YUV size!!");
                ret = OMX_ErrorUnsupportedSetting;
                break;
            }
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;

        pDstAVCComponent = &pH264Dec->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        SEC_H264DEC_HANDLE      *pH264Dec = NULL;

        ret = SEC_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        pDstErrorCorrectionType = &pH264Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = SEC_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        SEC_H264DEC_HANDLE  *pH264Dec = NULL;
        OMX_CONFIG_RECTTYPE *pSrcRectType = NULL;
        OMX_CONFIG_RECTTYPE *pDstRectType = NULL;
        pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;

        if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
            ret = OMX_ErrorNotReady;
            break;
        }

        pDstRectType = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

        if ((pDstRectType->nPortIndex != INPUT_PORT_INDEX) &&
            (pDstRectType->nPortIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        SEC_OMX_BASEPORT *pSECPort = &pSECComponent->pSECPort[pDstRectType->nPortIndex];

        pSrcRectType = &(pSECPort->cropRectangle);

        pDstRectType->nTop = pSrcRectType->nTop;
        pDstRectType->nLeft = pSrcRectType->nLeft;
        pDstRectType->nHeight = pSrcRectType->nHeight;
        pDstRectType->nWidth = pSrcRectType->nWidth;
    }
        break;
    default:
        ret = SEC_OMX_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexVendorThumbnailMode:
    {
        SEC_H264DEC_HANDLE   *pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;

        pH264Dec->hMFCH264Handle.bThumbnailMode = *((OMX_BOOL *)pComponentConfigStructure);

        ret = OMX_ErrorNone;
    }
        break;
    default:
        ret = SEC_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pSECComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_ENABLE_THUMBNAIL) == 0) {
        SEC_H264DEC_HANDLE *pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
        *pIndexType = OMX_IndexVendorThumbnailMode;
        ret = OMX_ErrorNone;
#ifdef USE_ANDROID_EXTENSION
    } else if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_ENABLE_ANB) == 0) {
        *pIndexType = OMX_IndexParamEnableAndroidBuffers;
        ret = OMX_ErrorNone;
    } else if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_GET_ANB) == 0) {
        *pIndexType = OMX_IndexParamGetAndroidNativeBuffer;
        ret = OMX_ErrorNone;
    } else if (SEC_OSAL_Strcmp(cParameterName, SEC_INDEX_PARAM_USE_ANB) == 0) {
        *pIndexType = OMX_IndexParamUseAndroidNativeBuffer;
        ret = OMX_ErrorNone;
#endif
    } else {
        ret = SEC_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264Dec_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        SEC_OSAL_Strcpy((char *)cRole, SEC_OMX_COMPONENT_H264_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_DecodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    SEC_OMX_BASECOMPONENT *pSECComponent = NULL;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;

    while (pH264Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
        SEC_OSAL_SemaphoreWait(pH264Dec->NBDecThread.hDecFrameStart);

        if (pH264Dec->NBDecThread.bExitDecodeThread == OMX_FALSE) {
            pH264Dec->hMFCH264Handle.returnCodec = SsbSipMfcDecExe(pH264Dec->hMFCH264Handle.hMFCHandle, pH264Dec->NBDecThread.oneFrameSize);
            SEC_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameEnd);
        }
    }

EXIT:
    SEC_OSAL_ThreadExit(NULL);
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE SEC_MFC_H264Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;

    OMX_PTR hMFCHandle       = NULL;
    OMX_PTR pStreamBuffer    = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;

    pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
    pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
    pSECComponent->bUseFlagEOF = OMX_FALSE;
    pSECComponent->bSaveFlagEOS = OMX_FALSE;

    /* MFC(Multi Function Codec) decoder and CMM(Codec Memory Management) driver open */
    SSBIP_MFC_BUFFER_TYPE buf_type = CACHE;
    hMFCHandle = (OMX_PTR)SsbSipMfcDecOpen(&buf_type);
    if (hMFCHandle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pH264Dec->hMFCH264Handle.hMFCHandle = hMFCHandle;

    /* Allocate decoder's input buffer */
    pStreamBuffer = SsbSipMfcDecGetInBuf(hMFCHandle, &pStreamPhyBuffer, DEFAULT_MFC_INPUT_BUFFER_SIZE * MFC_INPUT_BUFFER_NUM_MAX);
    if (pStreamBuffer == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264Dec->MFCDecInputBuffer[0].VirAddr = pStreamBuffer;
    pH264Dec->MFCDecInputBuffer[0].PhyAddr = pStreamPhyBuffer;
    pH264Dec->MFCDecInputBuffer[0].bufferSize = DEFAULT_MFC_INPUT_BUFFER_SIZE;
    pH264Dec->MFCDecInputBuffer[0].dataSize = 0;
    pH264Dec->MFCDecInputBuffer[1].VirAddr = (unsigned char *)pStreamBuffer + pH264Dec->MFCDecInputBuffer[0].bufferSize;
    pH264Dec->MFCDecInputBuffer[1].PhyAddr = (unsigned char *)pStreamPhyBuffer + pH264Dec->MFCDecInputBuffer[0].bufferSize;
    pH264Dec->MFCDecInputBuffer[1].bufferSize = DEFAULT_MFC_INPUT_BUFFER_SIZE;
    pH264Dec->MFCDecInputBuffer[1].dataSize = 0;
    pH264Dec->indexInputBuffer = 0;

    pH264Dec->bFirstFrame = OMX_TRUE;

    pH264Dec->NBDecThread.bExitDecodeThread = OMX_FALSE;
    pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
    pH264Dec->NBDecThread.oneFrameSize = 0;
    SEC_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameStart));
    SEC_OSAL_SemaphoreCreate(&(pH264Dec->NBDecThread.hDecFrameEnd));
    if (OMX_ErrorNone == SEC_OSAL_ThreadCreate(&pH264Dec->NBDecThread.hNBDecodeThread,
                                                SEC_MFC_DecodeThread,
                                                pOMXComponent)) {
        pH264Dec->hMFCH264Handle.returnCodec = MFC_RET_OK;
    }

    pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pH264Dec->MFCDecInputBuffer[0].VirAddr;
    pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pH264Dec->MFCDecInputBuffer[0].PhyAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->MFCDecInputBuffer[0].VirAddr;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize  = pH264Dec->MFCDecInputBuffer[0].bufferSize;

    SEC_OSAL_Memset(pSECComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    SEC_OSAL_Memset(pSECComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Dec->hMFCH264Handle.indexTimestamp = 0;
    pSECComponent->getAllDelayBuffer = OMX_FALSE;

EXIT:
    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE SEC_MFC_H264Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_H264DEC_HANDLE    *pH264Dec = NULL;
    OMX_PTR                hMFCHandle = NULL;

    FunctionIn();

    pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
    hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle;

    pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = NULL;
    pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pSECComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pH264Dec->NBDecThread.hNBDecodeThread != NULL) {
        pH264Dec->NBDecThread.bExitDecodeThread = OMX_TRUE;
        SEC_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameStart);
        SEC_OSAL_ThreadTerminate(pH264Dec->NBDecThread.hNBDecodeThread);
        pH264Dec->NBDecThread.hNBDecodeThread = NULL;
    }

    if(pH264Dec->NBDecThread.hDecFrameEnd != NULL) {
        SEC_OSAL_SemaphoreTerminate(pH264Dec->NBDecThread.hDecFrameEnd);
        pH264Dec->NBDecThread.hDecFrameEnd = NULL;
    }

    if(pH264Dec->NBDecThread.hDecFrameStart != NULL) {
        SEC_OSAL_SemaphoreTerminate(pH264Dec->NBDecThread.hDecFrameStart);
        pH264Dec->NBDecThread.hDecFrameStart = NULL;
    }

    if (hMFCHandle != NULL) {
        SsbSipMfcDecClose(hMFCHandle);
        hMFCHandle = pH264Dec->hMFCH264Handle.hMFCHandle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_MFC_H264_Decode(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE              ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT     *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_H264DEC_HANDLE        *pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
    OMX_U32                    oneFrameSize = pInputData->dataLen;
    SSBSIP_MFC_DEC_OUTPUT_INFO outputInfo;
    OMX_S32                    setConfVal = 0;
    int                        bufWidth = 0;
    int                        bufHeight = 0;
    OMX_BOOL                   outputDataValid = OMX_FALSE;

    FunctionIn();

    if (pH264Dec->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        SSBSIP_MFC_CODEC_TYPE eCodecType = H264_DEC;

        if ((oneFrameSize <= 0) && (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
            ret = OMX_ErrorNone;
            goto EXIT;
        }

        setConfVal = 0;
        SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);

        /* Default number in the driver is optimized */
        if (pH264Dec->hMFCH264Handle.bThumbnailMode == OMX_TRUE) {
            setConfVal = 1;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        } else {
            setConfVal = 8;
            SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
        }

        pH264Dec->hMFCH264Handle.returnCodec = SsbSipMfcDecInit(pH264Dec->hMFCH264Handle.hMFCHandle, eCodecType, oneFrameSize);
        if (pH264Dec->hMFCH264Handle.returnCodec == MFC_RET_OK) {
            SSBSIP_MFC_IMG_RESOLUTION imgResol;
            SSBSIP_MFC_CROP_INFORMATION cropInfo;
            SEC_OMX_BASEPORT *secInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
            SEC_OMX_BASEPORT *secOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT, &imgResol);
            SEC_OSAL_Log(SEC_LOG_TRACE, "set width height information : %d, %d",
                            secInputPort->portDefinition.format.video.nFrameWidth,
                            secInputPort->portDefinition.format.video.nFrameHeight);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc width height information : %d, %d",
                            imgResol.width, imgResol.height);

            SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_CROP_INFO, &cropInfo);
            SEC_OSAL_Log(SEC_LOG_TRACE, "mfc crop_top crop_bottom crop_left crop_right :  %d, %d, %d, %d",
                            cropInfo.crop_top_offset , cropInfo.crop_bottom_offset ,
                            cropInfo.crop_left_offset , cropInfo.crop_right_offset);

            secOutputPort->cropRectangle.nTop    = cropInfo.crop_top_offset;
            secOutputPort->cropRectangle.nLeft   = cropInfo.crop_left_offset;
            secOutputPort->cropRectangle.nWidth  = imgResol.width - cropInfo.crop_left_offset - cropInfo.crop_right_offset;
            secOutputPort->cropRectangle.nHeight = imgResol.height - cropInfo.crop_top_offset - cropInfo.crop_bottom_offset;

            pH264Dec->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

            /** Update Frame Size **/
            if ((cropInfo.crop_left_offset != 0) || (cropInfo.crop_right_offset != 0) ||
                (cropInfo.crop_top_offset != 0) || (cropInfo.crop_bottom_offset != 0)) {
                /* change width and height information */
                secInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                secInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                secInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                secInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send crop info call back */
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       OMX_IndexConfigCommonOutputCrop,
                       NULL);
            } else if((secInputPort->portDefinition.format.video.nFrameWidth != imgResol.width) ||
                      (secInputPort->portDefinition.format.video.nFrameHeight != imgResol.height)) {
                SEC_OSAL_Log(SEC_LOG_TRACE, "change width height information : OMX_EventPortSettingsChanged");
                /* change width and height information */
                secInputPort->portDefinition.format.video.nFrameWidth = imgResol.width;
                secInputPort->portDefinition.format.video.nFrameHeight = imgResol.height;
                secInputPort->portDefinition.format.video.nStride      = ((imgResol.width + 15) & (~15));
                secInputPort->portDefinition.format.video.nSliceHeight = ((imgResol.height + 15) & (~15));

                SEC_UpdateFrameSize(pOMXComponent);

                /** Send Port Settings changed call back */
                (*(pSECComponent->pCallbacks->EventHandler))
                      (pOMXComponent,
                       pSECComponent->callbackData,
                       OMX_EventPortSettingsChanged, /* The command was completed */
                       OMX_DirOutput, /* This is the port index */
                       0,
                       NULL);
            }

#ifdef ADD_SPS_PPS_I_FRAME
            ret = OMX_ErrorInputDataDecodeYet;
#else
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;

            ret = OMX_ErrorNone;
#endif
            goto EXIT;
        } else {
            ret = OMX_ErrorMFCInit;
            goto EXIT;
        }
    }

#ifndef FULL_FRAME_SEARCH
    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pSECComponent->bUseFlagEOF == OMX_FALSE))
        pSECComponent->bUseFlagEOF = OMX_TRUE;
#endif

    pSECComponent->timeStamp[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
    pSECComponent->nFlags[pH264Dec->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Dec->hMFCH264Handle.returnCodec == MFC_RET_OK) &&
        (pH264Dec->bFirstFrame == OMX_FALSE)) {
        SSBSIP_MFC_DEC_OUTBUF_STATUS status;
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc decode done */
        if (pH264Dec->NBDecThread.bDecoderRun == OMX_TRUE) {
            SEC_OSAL_SemaphoreWait(pH264Dec->NBDecThread.hDecFrameEnd);
            pH264Dec->NBDecThread.bDecoderRun = OMX_FALSE;
        }

        status = SsbSipMfcDecGetOutBuf(pH264Dec->hMFCH264Handle.hMFCHandle, &outputInfo);
        bufWidth  = (outputInfo.img_width + 15) & (~15);
        bufHeight = (outputInfo.img_height + 15) & (~15);

        if ((SsbSipMfcDecGetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_GETCONF_FRAME_TAG, &indexTimestamp) != MFC_RET_OK) ||
            (((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)))) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pSECComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pSECComponent->nFlags[indexTimestamp];
        }
        SEC_OSAL_Log(SEC_LOG_TRACE, "timestamp %lld us (%.2f secs)", pOutputData->timeStamp, pOutputData->timeStamp / 1E6);

        if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
            (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
            outputDataValid = OMX_TRUE;
        }
        if (pOutputData->nFlags & OMX_BUFFERFLAG_EOS)
            outputDataValid = OMX_FALSE;

        if ((status == MFC_GETOUTBUF_DISPLAY_ONLY) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE))
            ret = OMX_ErrorInputDataDecodeYet;

        if(status == MFC_GETOUTBUF_DECODING_ONLY) {
            if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
                (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
                pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pSECComponent->getAllDelayBuffer = OMX_TRUE;
                ret = OMX_ErrorInputDataDecodeYet;
            } else {
                ret = OMX_ErrorNone;
            }
            outputDataValid = OMX_FALSE;
        }

#ifdef FULL_FRAME_SEARCH
        if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) &&
            (pSECComponent->bSaveFlagEOS == OMX_TRUE)) {
            pInputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else
#endif
        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pSECComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataDecodeYet;
        } else if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
            ret = OMX_ErrorNone;
        }
    } else {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        if ((pSECComponent->bSaveFlagEOS == OMX_TRUE) ||
            (pSECComponent->getAllDelayBuffer == OMX_TRUE) ||
            (pInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
            pSECComponent->getAllDelayBuffer = OMX_FALSE;
        }
        if ((pH264Dec->bFirstFrame == OMX_TRUE) &&
            ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            pOutputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
        }

        outputDataValid = OMX_FALSE;

        /* ret = OMX_ErrorUndefined; */
        ret = OMX_ErrorNone;
    }

    if (ret == OMX_ErrorInputDataDecodeYet) {
        pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].dataSize = oneFrameSize;
        pH264Dec->indexInputBuffer++;
        pH264Dec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].bufferSize;
        oneFrameSize = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].dataSize;
        //pInputData->dataLen = oneFrameSize;
        //pInputData->remainDataLen = oneFrameSize;
    }

    if ((Check_H264_StartCode(pInputData->dataBuffer, pInputData->dataLen) == OMX_TRUE) &&
        ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        SsbSipMfcDecSetConfig(pH264Dec->hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &(pH264Dec->hMFCH264Handle.indexTimestamp));
        pH264Dec->hMFCH264Handle.indexTimestamp++;
        pH264Dec->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;

        SsbSipMfcDecSetInBuf(pH264Dec->hMFCH264Handle.hMFCHandle,
                             pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer,
                             pH264Dec->hMFCH264Handle.pMFCStreamBuffer,
                             pSECComponent->processData[INPUT_PORT_INDEX].allocSize);

        pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].dataSize = oneFrameSize;
        pH264Dec->NBDecThread.oneFrameSize = oneFrameSize;

        /* mfc decode start */
        SEC_OSAL_SemaphorePost(pH264Dec->NBDecThread.hDecFrameStart);
        pH264Dec->NBDecThread.bDecoderRun = OMX_TRUE;
        pH264Dec->hMFCH264Handle.returnCodec = MFC_RET_OK;

        pH264Dec->indexInputBuffer++;
        pH264Dec->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
        pH264Dec->hMFCH264Handle.pMFCStreamBuffer    = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pH264Dec->hMFCH264Handle.pMFCStreamPhyBuffer = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].PhyAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].dataBuffer = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].VirAddr;
        pSECComponent->processData[INPUT_PORT_INDEX].allocSize = pH264Dec->MFCDecInputBuffer[pH264Dec->indexInputBuffer].bufferSize;
        if (((pH264Dec->hMFCH264Handle.bThumbnailMode == OMX_TRUE) || (pSECComponent->bSaveFlagEOS == OMX_TRUE)) &&
            (pH264Dec->bFirstFrame == OMX_TRUE) &&
            (outputDataValid == OMX_FALSE)) {
            ret = OMX_ErrorInputDataDecodeYet;
        }
        pH264Dec->bFirstFrame = OMX_FALSE;
    }

    /** Fill Output Buffer **/
    if (outputDataValid == OMX_TRUE) {
        SEC_OMX_BASEPORT *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
        SEC_OMX_BASEPORT *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
        void *pOutputBuf[3];

        int frameSize = bufWidth * bufHeight;
        int imageSize = outputInfo.img_width * outputInfo.img_height;

        int actualWidth  = outputInfo.img_width;
        int actualHeight = outputInfo.img_height;
        int actualImageSize = imageSize;

        pOutputBuf[0] = (void *)pOutputData->dataBuffer;
        pOutputBuf[1] = (void *)pOutputData->dataBuffer + actualImageSize;
        pOutputBuf[2] = (void *)pOutputData->dataBuffer + ((actualImageSize * 5) / 4);

#ifdef USE_ANDROID_EXTENSION
        if (pSECOutputPort->bUseAndroidNativeBuffer == OMX_TRUE) {
            OMX_U32 retANB = 0;
            void *pVirAddrs[2];
            actualWidth  = (outputInfo.img_width + 15) & (~15);
            actualImageSize = actualWidth * actualHeight;

            retANB = getVADDRfromANB (pOutputData->dataBuffer,
                            (OMX_U32)pSECInputPort->portDefinition.format.video.nFrameWidth,
                            (OMX_U32)pSECInputPort->portDefinition.format.video.nFrameHeight,
                            pVirAddrs);
            if (retANB != 0) {
                SEC_OSAL_Log(SEC_LOG_ERROR, "Error getVADDRfromANB, Error code:%d", retANB);
                ret = OMX_ErrorOverflow;
                goto EXIT;
            }
            pOutputBuf[0] = pVirAddrs[0];
            pOutputBuf[1] = pVirAddrs[1];
        }
#endif
        if ((pH264Dec->hMFCH264Handle.bThumbnailMode == OMX_FALSE) &&
            (pSECOutputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12TPhysicalAddress))
        {
            /* if use Post copy address structure */
            SEC_OSAL_Memcpy(pOutputBuf[0], &frameSize, sizeof(frameSize));
            SEC_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize), &(outputInfo.YPhyAddr), sizeof(outputInfo.YPhyAddr));
            SEC_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 1), &(outputInfo.CPhyAddr), sizeof(outputInfo.CPhyAddr));
            SEC_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 2), &(outputInfo.YVirAddr), sizeof(outputInfo.YVirAddr));
            SEC_OSAL_Memcpy(pOutputBuf[0] + sizeof(frameSize) + (sizeof(void *) * 3), &(outputInfo.CVirAddr), sizeof(outputInfo.CVirAddr));
            pOutputData->dataLen = (bufWidth * bufHeight * 3) / 2;
        } else {
            switch (pSECOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            {
                SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420P out");
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[0],
                    (unsigned char *)outputInfo.YVirAddr,
                    actualWidth,
                    actualHeight);
                csc_tiled_to_linear_deinterleave(
                    (unsigned char *)pOutputBuf[1],
                    (unsigned char *)pOutputBuf[2],
                    (unsigned char *)outputInfo.CVirAddr,
                    actualWidth,
                    actualHeight >> 1);
                pOutputData->dataLen = actualImageSize * 3 / 2;
            }
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
            case OMX_SEC_COLOR_FormatANBYUV420SemiPlanar:
            default:
            {
                SEC_OSAL_Log(SEC_LOG_TRACE, "YUV420SP out");
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[0],
                    (unsigned char *)outputInfo.YVirAddr,
                    actualWidth,
                    actualHeight);
                csc_tiled_to_linear(
                    (unsigned char *)pOutputBuf[1],
                    (unsigned char *)outputInfo.CVirAddr,
                    actualWidth,
                    actualHeight >> 1);
                pOutputData->dataLen = actualImageSize * 3 / 2;
            }
                break;
            }
        }
#ifdef USE_ANDROID_EXTENSION
        if (pSECOutputPort->bUseAndroidNativeBuffer == OMX_TRUE)
            putVADDRtoANB(pOutputData->dataBuffer);
#endif
    } else {
        pOutputData->dataLen = 0;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Decode */
OMX_ERRORTYPE SEC_MFC_H264Dec_bufferProcess(OMX_COMPONENTTYPE *pOMXComponent, SEC_OMX_DATA *pInputData, SEC_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    SEC_OMX_BASECOMPONENT   *pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    SEC_H264DEC_HANDLE      *pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
    SEC_OMX_BASEPORT        *pSECInputPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    SEC_OMX_BASEPORT        *pSECOutputPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                 endOfFrame = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pSECInputPort)) || (!CHECK_PORT_ENABLED(pSECOutputPort)) ||
            (!CHECK_PORT_POPULATED(pSECInputPort)) || (!CHECK_PORT_POPULATED(pSECOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == SEC_Check_BufferProcess_State(pSECComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = SEC_MFC_H264_Decode(pOMXComponent, pInputData, pOutputData);
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataDecodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pSECComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                    pSECComponent->callbackData,
                                                    OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->previousDataLen = pInputData->dataLen;
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        pOutputData->usedDataLen = 0;
        pOutputData->remainDataLen = pOutputData->dataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE SEC_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    SEC_OMX_BASEPORT        *pSECPort = NULL;
    SEC_H264DEC_HANDLE      *pH264Dec = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (SEC_OSAL_Strcmp(SEC_OMX_COMPONENT_H264_DEC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = SEC_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pSECComponent->codecType = HW_VIDEO_CODEC;

    pSECComponent->componentName = (OMX_STRING)SEC_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pSECComponent->componentName == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pSECComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Dec = SEC_OSAL_Malloc(sizeof(SEC_H264DEC_HANDLE));
    if (pH264Dec == NULL) {
        SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        SEC_OSAL_Log(SEC_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    SEC_OSAL_Memset(pH264Dec, 0, sizeof(SEC_H264DEC_HANDLE));
    pSECComponent->hCodecHandle = (OMX_HANDLETYPE)pH264Dec;

    SEC_OSAL_Strcpy(pSECComponent->componentName, SEC_OMX_COMPONENT_H264_DEC);
    /* Set componentVersion */
    pSECComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pSECComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pSECComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pSECComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pSECComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Android CapabilityFlags */
    pSECComponent->capabilityFlags.iIsOMXComponentMultiThreaded                   = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc  = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsMovableInputBuffers       = OMX_FALSE;
    pSECComponent->capabilityFlags.iOMXComponentSupportsPartialFrames             = OMX_FALSE;
    pSECComponent->capabilityFlags.iOMXComponentUsesNALStartCodes                 = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentCanHandleIncompleteFrames         = OMX_TRUE;
    pSECComponent->capabilityFlags.iOMXComponentUsesFullAVCFrames                 = OMX_TRUE;

    /* Input port */
    pSECPort = &pSECComponent->pSECPort[INPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "video/avc");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pSECPort = &pSECComponent->pSECPort[OUTPUT_PORT_INDEX];
    pSECPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pSECPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pSECPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pSECPort->portDefinition.format.video.nSliceHeight = 0;
    pSECPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pSECPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    SEC_OSAL_Memset(pSECPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    SEC_OSAL_Strcpy(pSECPort->portDefinition.format.video.cMIMEType, "raw/video");
    pSECPort->portDefinition.format.video.pNativeRender = 0;
    pSECPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pSECPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pSECPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Dec->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Dec->AVCComponent[i].nPortIndex = i;
        pH264Dec->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Dec->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel4;
    }

    pOMXComponent->GetParameter      = &SEC_MFC_H264Dec_GetParameter;
    pOMXComponent->SetParameter      = &SEC_MFC_H264Dec_SetParameter;
    pOMXComponent->GetConfig         = &SEC_MFC_H264Dec_GetConfig;
    pOMXComponent->SetConfig         = &SEC_MFC_H264Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &SEC_MFC_H264Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &SEC_MFC_H264Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &SEC_OMX_ComponentDeinit;

    pSECComponent->sec_mfc_componentInit      = &SEC_MFC_H264Dec_Init;
    pSECComponent->sec_mfc_componentTerminate = &SEC_MFC_H264Dec_Terminate;
    pSECComponent->sec_mfc_bufferProcess      = &SEC_MFC_H264Dec_bufferProcess;
    pSECComponent->sec_checkInputFrame        = &Check_H264_Frame;

    pSECComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE SEC_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE       *pOMXComponent = NULL;
    SEC_OMX_BASECOMPONENT   *pSECComponent = NULL;
    SEC_H264DEC_HANDLE      *pH264Dec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pSECComponent = (SEC_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    SEC_OSAL_Free(pSECComponent->componentName);
    pSECComponent->componentName = NULL;

    pH264Dec = (SEC_H264DEC_HANDLE *)pSECComponent->hCodecHandle;
    if (pH264Dec != NULL) {
        SEC_OSAL_Free(pH264Dec);
        pH264Dec = pSECComponent->hCodecHandle = NULL;
    }

    ret = SEC_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if(ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
