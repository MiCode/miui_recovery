/*
 * Copyright 2010 Samsung Electronics Co. LTD
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <utils/Log.h>

#include "mfc_interface.h"
#include "SsbSipMfcApi.h"

#define _MFCLIB_MAGIC_NUMBER    0x92241000

#define USR_DATA_START_CODE     (0x000001B2)
#define VOP_START_CODE          (0x000001B6)
#define MP4_START_CODE          (0x000001)

static void getAByte(char *buff, int *code)
{
    int byte;

    *code = (*code << 8);
    byte = (int)*buff;
    byte &= 0xFF;
    *code |= byte;
}

static mfc_packed_mode isPBPacked(_MFCLIB *pCtx, int length)
{
    char *strmBuffer = NULL;
    char *strmBufferEnd = NULL;
    int startCode = 0xFFFFFFFF;

    strmBuffer = (char *)pCtx->virStrmBuf;
    strmBufferEnd = (char *)pCtx->virStrmBuf + length;

    while (1) {
        while (startCode != USR_DATA_START_CODE) {
            if (startCode == VOP_START_CODE) {
                LOGV("isPBPacked: VOP START Found !!.....return\n");
                LOGV("isPBPacked: Non Packed PB\n");
                return MFC_UNPACKED_PB;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++;
            if (strmBuffer >= strmBufferEnd)
                goto out;
        }
        LOGV("isPBPacked: User Data Found !!\n");

        do {
            if (*strmBuffer == 'p') {
                LOGI("isPBPacked: Packed PB\n");
                return MFC_PACKED_PB;
            }
            getAByte(strmBuffer, &startCode);
            strmBuffer++;
            if (strmBuffer >= strmBufferEnd)
                goto out;
        } while ((startCode >> 8) != MP4_START_CODE);
    }

out:
    LOGV("isPBPacked: Non Packed PB\n");
    return MFC_UNPACKED_PB;
}

void *SsbSipMfcDecOpen(void *value)
{
    int hMFCOpen;
    unsigned int mapped_addr;
    _MFCLIB *pCTX;
    mfc_common_args DecArg;
    int ret_code;

    pCTX = (_MFCLIB *)malloc(sizeof(_MFCLIB));
    if (pCTX == NULL) {
        LOGE("SsbSipMfcDecOpen: malloc failed.\n");
        return NULL;
    }
    memset(pCTX, 0, sizeof(_MFCLIB));

    hMFCOpen = open(S5PC110_MFC_DEV_NAME, O_RDWR | O_NDELAY);
    if (hMFCOpen < 0) {
        LOGE("SsbSipMfcDecOpen: MFC Open failure\n");
        return NULL;
    }

    if (*(unsigned int *)value == NO_CACHE ||
        *(unsigned int *)value == CACHE) {
        DecArg.args.buf_type = *(unsigned int *)value;
        ret_code = ioctl(hMFCOpen, IOCTL_MFC_BUF_CACHE, &DecArg);
        if (DecArg.ret_code != MFC_RET_OK) {
            LOGE("SsbSipMfcDecOpenExt: IOCTL_MFC_BUF_CACHE (%d) failed\n", DecArg.ret_code);
        }
    } else {
        LOGE("SsbSipMfcDecOpenExt: value is invalid, value: %d\n", *(int *)value);
    }

    mapped_addr = (unsigned int)mmap(0, MMAP_BUFFER_SIZE_MMAP, PROT_READ | PROT_WRITE, MAP_SHARED, hMFCOpen, 0);
    if (!mapped_addr) {
        LOGE("SsbSipMfcDecOpen: FIMV5.0 driver address mapping failed\n");
        return NULL;
    }

    pCTX->magic = _MFCLIB_MAGIC_NUMBER;
    pCTX->hMFC = hMFCOpen;
    pCTX->mapped_addr = mapped_addr;
    pCTX->inter_buff_status = MFC_USE_NONE;

    return (void *)pCTX;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecInit(void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng)
{
    int ret_code;
    int packedPB = MFC_UNPACKED_PB;
    mfc_common_args DecArg;
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecSetConfig: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    if ((codec_type != MPEG4_DEC)  &&
        (codec_type != H264_DEC)   &&
        (codec_type != H263_DEC)   &&
        (codec_type != MPEG1_DEC)  &&
        (codec_type != MPEG2_DEC)  &&
        (codec_type != FIMV1_DEC)  &&
        (codec_type != FIMV2_DEC)  &&
        (codec_type != FIMV3_DEC)  &&
        (codec_type != FIMV4_DEC)  &&
        (codec_type != XVID_DEC)   &&
        (codec_type != VC1RCV_DEC) &&
        (codec_type != VC1_DEC)) {
        LOGE("SsbSipMfcDecOpen: Undefined codec type.\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX->codec_type = codec_type;

    if ((pCTX->codec_type == MPEG4_DEC)   ||
        (pCTX->codec_type == FIMV1_DEC) ||
        (pCTX->codec_type == FIMV2_DEC) ||
        (pCTX->codec_type == FIMV3_DEC) ||
        (pCTX->codec_type == FIMV4_DEC) ||
        (pCTX->codec_type == XVID_DEC))
        packedPB = isPBPacked(pCTX, Frameleng);

    /* init args */
    DecArg.args.dec_init.in_codec_type = pCTX->codec_type;
    DecArg.args.dec_init.in_strm_size = Frameleng;
    DecArg.args.dec_init.in_strm_buf = pCTX->phyStrmBuf;
    DecArg.args.dec_init.in_packed_PB = packedPB;

    /* mem alloc args */
    DecArg.args.dec_init.in_mapped_addr = pCTX->mapped_addr;

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_DEC_INIT, &DecArg);
    if (DecArg.ret_code != MFC_RET_OK) {
        LOGE("SsbSipMfcDecInit: IOCTL_MFC_DEC_INIT (%d) failed\n", DecArg.ret_code);
        return MFC_RET_DEC_INIT_FAIL;
    }

    pCTX->decOutInfo.img_width = DecArg.args.dec_init.out_img_width;
    pCTX->decOutInfo.img_height = DecArg.args.dec_init.out_img_height;
    pCTX->decOutInfo.buf_width = DecArg.args.dec_init.out_buf_width;
    pCTX->decOutInfo.buf_height = DecArg.args.dec_init.out_buf_height;

    /* by RainAde : crop information */
    pCTX->decOutInfo.crop_top_offset = DecArg.args.dec_init.out_crop_top_offset;
    pCTX->decOutInfo.crop_bottom_offset = DecArg.args.dec_init.out_crop_bottom_offset;
    pCTX->decOutInfo.crop_left_offset = DecArg.args.dec_init.out_crop_left_offset;
    pCTX->decOutInfo.crop_right_offset = DecArg.args.dec_init.out_crop_right_offset;

    pCTX->virFrmBuf.luma = DecArg.args.dec_init.out_u_addr.luma;
    pCTX->virFrmBuf.chroma = DecArg.args.dec_init.out_u_addr.chroma;

    pCTX->phyFrmBuf.luma = DecArg.args.dec_init.out_p_addr.luma;
    pCTX->phyFrmBuf.chroma = DecArg.args.dec_init.out_p_addr.chroma;
    pCTX->sizeFrmBuf.luma = DecArg.args.dec_init.out_frame_buf_size.luma;
    pCTX->sizeFrmBuf.chroma = DecArg.args.dec_init.out_frame_buf_size.chroma;
    pCTX->inter_buff_status |= MFC_USE_YUV_BUFF;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecExe(void *openHandle, int lengthBufFill)
{
    int ret_code;
    int Yoffset;
    int Coffset;
    _MFCLIB *pCTX;
    mfc_common_args DecArg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecExe: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    if ((lengthBufFill < 0) || (lengthBufFill > MAX_DECODER_INPUT_BUFFER_SIZE)) {
        LOGE("SsbSipMfcDecExe: lengthBufFill is invalid. (lengthBufFill=%d)\n", lengthBufFill);
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    DecArg.args.dec_exe.in_codec_type = pCTX->codec_type;
    DecArg.args.dec_exe.in_strm_buf = pCTX->phyStrmBuf;
    DecArg.args.dec_exe.in_strm_size = lengthBufFill;
    DecArg.args.dec_exe.in_frm_buf.luma = pCTX->phyFrmBuf.luma;
    DecArg.args.dec_exe.in_frm_buf.chroma = pCTX->phyFrmBuf.chroma;
    DecArg.args.dec_exe.in_frm_size.luma = pCTX->sizeFrmBuf.luma;
    DecArg.args.dec_exe.in_frm_size.chroma = pCTX->sizeFrmBuf.chroma;
    DecArg.args.dec_exe.in_frametag = pCTX->in_frametag;

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_DEC_EXE, &DecArg);
    if (DecArg.ret_code != MFC_RET_OK) {
        LOGE("SsbSipMfcDecExe: IOCTL_MFC_DEC_EXE failed(ret : %d)\n", DecArg.ret_code);
        return MFC_RET_DEC_EXE_ERR;
    }

    Yoffset = DecArg.args.dec_exe.out_display_Y_addr - DecArg.args.dec_exe.in_frm_buf.luma;
    Coffset = DecArg.args.dec_exe.out_display_C_addr - DecArg.args.dec_exe.in_frm_buf.chroma;

    pCTX->decOutInfo.YPhyAddr = (void *)(DecArg.args.dec_exe.out_display_Y_addr);
    pCTX->decOutInfo.CPhyAddr = (void *)(DecArg.args.dec_exe.out_display_C_addr);
    pCTX->decOutInfo.YVirAddr = (void *)(pCTX->virFrmBuf.luma + Yoffset);
    pCTX->decOutInfo.CVirAddr = (void *)(pCTX->virFrmBuf.chroma + Coffset);
    pCTX->decOutInfo.timestamp_top = DecArg.args.dec_exe.out_timestamp_top;
    pCTX->decOutInfo.timestamp_bottom = DecArg.args.dec_exe.out_timestamp_bottom;
    pCTX->decOutInfo.consumedByte = DecArg.args.dec_exe.out_consume_bytes;
    pCTX->decOutInfo.res_change = DecArg.args.dec_exe.out_res_change;
    pCTX->decOutInfo.crop_top_offset = DecArg.args.dec_exe.out_crop_top_offset;
    pCTX->decOutInfo.crop_bottom_offset = DecArg.args.dec_exe.out_crop_bottom_offset;
    pCTX->decOutInfo.crop_left_offset = DecArg.args.dec_exe.out_crop_left_offset;
    pCTX->decOutInfo.crop_right_offset = DecArg.args.dec_exe.out_crop_right_offset;
    pCTX->out_frametag_top = DecArg.args.dec_exe.out_frametag_top;
    pCTX->out_frametag_bottom = DecArg.args.dec_exe.out_frametag_bottom;
    pCTX->displayStatus = DecArg.args.dec_exe.out_display_status;

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecClose(void *openHandle)
{
    int ret_code;
    _MFCLIB *pCTX;
    mfc_common_args free_arg;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecClose: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;

    if (pCTX->inter_buff_status & MFC_USE_YUV_BUFF) {
        free_arg.args.mem_free.u_addr = pCTX->virFrmBuf.luma;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
        free_arg.args.mem_free.u_addr = pCTX->virFrmBuf.chroma;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }

    if (pCTX->inter_buff_status & MFC_USE_STRM_BUFF) {
        free_arg.args.mem_free.u_addr = pCTX->virStrmBuf;
        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_FREE_BUF, &free_arg);
    }

    pCTX->inter_buff_status = MFC_USE_NONE;

    munmap((void *)pCTX->mapped_addr, MMAP_BUFFER_SIZE_MMAP);
    close(pCTX->hMFC);
    free(pCTX);

    return MFC_RET_OK;
}

void *SsbSipMfcDecGetInBuf(void *openHandle, void **phyInBuf, int inputBufferSize)
{
    int ret_code;
    _MFCLIB *pCTX;
    mfc_common_args user_addr_arg, phys_addr_arg;

    if (inputBufferSize < 0) {
        LOGE("SsbSipMfcDecGetInBuf: inputBufferSize = %d is invalid\n", inputBufferSize);
        return NULL;
    }

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetInBuf: openHandle is NULL\n");
        return NULL;
    }

    pCTX = (_MFCLIB *)openHandle;

    user_addr_arg.args.mem_alloc.codec_type = pCTX->codec_type;
    user_addr_arg.args.mem_alloc.buff_size = inputBufferSize;
    user_addr_arg.args.mem_alloc.mapped_addr = pCTX->mapped_addr;
    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_IN_BUF, &user_addr_arg);
    if (ret_code < 0) {
        LOGE("SsbSipMfcDecGetInBuf: IOCTL_MFC_GET_IN_BUF failed\n");
        return NULL;
    }
    pCTX->virStrmBuf = user_addr_arg.args.mem_alloc.out_uaddr;
    pCTX->phyStrmBuf = user_addr_arg.args.mem_alloc.out_paddr;
    pCTX->sizeStrmBuf = inputBufferSize;
    pCTX->inter_buff_status |= MFC_USE_STRM_BUFF;

    *phyInBuf = (void *)pCTX->phyStrmBuf;

    return (void *)pCTX->virStrmBuf;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetInBuf(void *openHandle, void *phyInBuf, void *virInBuf, int inputBufferSize)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecSetInBuf: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX  = (_MFCLIB *)openHandle;

    pCTX->phyStrmBuf = (int)phyInBuf;
    pCTX->virStrmBuf = (int)virInBuf;
    pCTX->sizeStrmBuf = inputBufferSize;
    return MFC_RET_OK;
}

SSBSIP_MFC_DEC_OUTBUF_STATUS SsbSipMfcDecGetOutBuf(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info)
{
    _MFCLIB *pCTX;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetOutBuf: openHandle is NULL\n");
        return MFC_GETOUTBUF_DISPLAY_END;
    }

    pCTX = (_MFCLIB *)openHandle;

    output_info->YPhyAddr = pCTX->decOutInfo.YPhyAddr;
    output_info->CPhyAddr = pCTX->decOutInfo.CPhyAddr;

    output_info->YVirAddr = pCTX->decOutInfo.YVirAddr;
    output_info->CVirAddr = pCTX->decOutInfo.CVirAddr;

    output_info->img_width = pCTX->decOutInfo.img_width;
    output_info->img_height= pCTX->decOutInfo.img_height;

    output_info->buf_width = pCTX->decOutInfo.buf_width;
    output_info->buf_height= pCTX->decOutInfo.buf_height;

    /* by RainAde : for crop information */
    output_info->crop_top_offset = pCTX->decOutInfo.crop_top_offset;
    output_info->crop_bottom_offset= pCTX->decOutInfo.crop_bottom_offset;
    output_info->crop_left_offset = pCTX->decOutInfo.crop_left_offset;
    output_info->crop_right_offset= pCTX->decOutInfo.crop_right_offset;

    if (pCTX->displayStatus == 0)
        return MFC_GETOUTBUF_DISPLAY_END;
    else if (pCTX->displayStatus == 1)
        return MFC_GETOUTBUF_DISPLAY_DECODING;
    else if (pCTX->displayStatus == 2)
        return MFC_GETOUTBUF_DISPLAY_ONLY;
    else
        return MFC_GETOUTBUF_DECODING_ONLY;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecSetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    mfc_common_args DecArg;
    SSBSIP_MFC_IMG_RESOLUTION *img_resolution;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecSetConfig: openHandle is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcDecSetConfig: value is NULL\n");
        return MFC_RET_INVALID_PARAM;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    switch (conf_type) {
    case MFC_DEC_SETCONF_POST_ENABLE:
    case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
    case MFC_DEC_SETCONF_DISPLAY_DELAY:
    case MFC_DEC_SETCONF_IS_LAST_FRAME:
    case MFC_DEC_SETCONF_SLICE_ENABLE:
    case MFC_DEC_SETCONF_CRC_ENABLE:
        DecArg.args.set_config.in_config_param = conf_type;
        DecArg.args.set_config.in_config_value[0] = *((unsigned int *)value);
        DecArg.args.set_config.in_config_value[1] = 0;
        break;

    case MFC_DEC_SETCONF_FIMV1_WIDTH_HEIGHT:
        img_resolution = (SSBSIP_MFC_IMG_RESOLUTION *)value;
        DecArg.args.set_config.in_config_param = conf_type;
        DecArg.args.set_config.in_config_value[0] = img_resolution->width;
        DecArg.args.set_config.in_config_value[1] = img_resolution->height;
        break;

    case MFC_DEC_SETCONF_FRAME_TAG:
        pCTX->in_frametag = *((int *)value);
        return MFC_RET_OK;

    default:
        LOGE("SsbSipMfcDecSetConfig: No such conf_type is supported.\n");
        return MFC_RET_INVALID_PARAM;
    }

    ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_SET_CONFIG, &DecArg);
    if (DecArg.ret_code != MFC_RET_OK) {
        LOGE("SsbSipMfcDecSetConfig: IOCTL_MFC_SET_CONFIG failed(ret : %d, conf_type: %d)\n", DecArg.ret_code, conf_type);
        return MFC_RET_DEC_SET_CONF_FAIL;
    }

    return MFC_RET_OK;
}

SSBSIP_MFC_ERROR_CODE SsbSipMfcDecGetConfig(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value)
{
    int ret_code;
    _MFCLIB *pCTX;
    mfc_common_args DecArg;

    SSBSIP_MFC_IMG_RESOLUTION *img_resolution;
    SSBSIP_MFC_CROP_INFORMATION *crop_information;
    MFC_CRC_DATA *crc_data;

    if (openHandle == NULL) {
        LOGE("SsbSipMfcDecGetConfig: openHandle is NULL\n");
        return MFC_RET_FAIL;
    }

    if (value == NULL) {
        LOGE("SsbSipMfcDecGetConfig: value is NULL\n");
        return MFC_RET_FAIL;
    }

    pCTX = (_MFCLIB *)openHandle;
    memset(&DecArg, 0x00, sizeof(DecArg));

    switch (conf_type) {
    case MFC_DEC_GETCONF_BUF_WIDTH_HEIGHT:
        img_resolution = (SSBSIP_MFC_IMG_RESOLUTION *)value;
        img_resolution->width = pCTX->decOutInfo.img_width;
        img_resolution->height = pCTX->decOutInfo.img_height;
        img_resolution->buf_width = pCTX->decOutInfo.buf_width;
        img_resolution->buf_height = pCTX->decOutInfo.buf_height;
        break;

    /* Added by RainAde */
    case MFC_DEC_GETCONF_CROP_INFO:
        crop_information = (SSBSIP_MFC_CROP_INFORMATION*)value;
        crop_information->crop_top_offset = pCTX->decOutInfo.crop_top_offset;
        crop_information->crop_bottom_offset= pCTX->decOutInfo.crop_bottom_offset;
        crop_information->crop_left_offset = pCTX->decOutInfo.crop_left_offset;
        crop_information->crop_right_offset= pCTX->decOutInfo.crop_right_offset;
        break;

    case MFC_DEC_GETCONF_CRC_DATA:
        crc_data = (MFC_CRC_DATA *)value;

        DecArg.args.get_config.in_config_param = conf_type;

        ret_code = ioctl(pCTX->hMFC, IOCTL_MFC_GET_CONFIG, &DecArg);
        if (DecArg.ret_code != MFC_RET_OK) {
            LOGE("SsbSipMfcDecGetConfig: IOCTL_MFC_GET_CONFIG failed(ret : %d, conf_type: %d)\n", DecArg.ret_code, conf_type);
            return MFC_RET_DEC_GET_CONF_FAIL;
        }
        crc_data->luma0 = DecArg.args.get_config.out_config_value[0];
        crc_data->chroma0 = DecArg.args.get_config.out_config_value[1];
        break;

    case MFC_DEC_GETCONF_FRAME_TAG:
        *((unsigned int *)value) = pCTX->out_frametag_top;
        break;

    default:
        LOGE("SsbSipMfcDecGetConfig: No such conf_type is supported.\n");
        return MFC_RET_INVALID_PARAM;
    }

    return MFC_RET_OK;
}
