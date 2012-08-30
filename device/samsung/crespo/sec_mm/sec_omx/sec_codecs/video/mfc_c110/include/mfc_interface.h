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

#ifndef _MFC_INTERFACE_H_
#define _MFC_INTERFACE_H_

#include "SsbSipMfcApi.h"

#define IOCTL_MFC_DEC_INIT                     0x00800001
#define IOCTL_MFC_ENC_INIT                     0x00800002
#define IOCTL_MFC_DEC_EXE                      0x00800003
#define IOCTL_MFC_ENC_EXE                      0x00800004

#define IOCTL_MFC_GET_IN_BUF                   0x00800010
#define IOCTL_MFC_FREE_BUF                     0x00800011
#define IOCTL_MFC_GET_PHYS_ADDR                0x00800012

#define IOCTL_MFC_SET_CONFIG                   0x00800101
#define IOCTL_MFC_GET_CONFIG                   0x00800102

#define IOCTL_MFC_BUF_CACHE                    0x00801000

/* MFC H/W support maximum 32 extra DPB */
#define MFC_MAX_EXTRA_DPB                      5

#define ENC_PROFILE_LEVEL(profile, level)      ((profile) | ((level) << 8))

#define ENC_PROFILE_MPEG4_SP                   0
#define ENC_PROFILE_MPEG4_ASP                  1
#define ENC_PROFILE_H264_BP                    0
#define ENC_PROFILE_H264_MAIN                  1
#define ENC_PROFILE_H264_HIGH                  2

#define ENC_RC_DISABLE                         0
#define ENC_RC_ENABLE_MACROBLOCK               1
#define ENC_RC_ENABLE_FRAME                    2

#define ENC_RC_QBOUND(min_qp, max_qp)          ((min_qp) | ((max_qp) << 8))
#define ENC_RC_MB_CTRL_DARK_DISABLE            (1 << 3)
#define ENC_RC_MB_CTRL_SMOOTH_DISABLE          (1 << 2)
#define ENC_RC_MB_CTRL_STATIC_DISABLE          (1 << 1)
#define ENC_RC_MB_CTRL_ACTIVITY_DISABLE        (1 << 0)

#define ALIGN_TO_16B(x)   ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_64B(x)   ((((x) + (1 <<  6) - 1) >>  6) <<  6)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)   ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_4KB(x)   ((((x) + (1 << 12) - 1) >> 12) << 12)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)  ((((x) + (1 << 16) - 1) >> 16) << 16)
#define ALIGN_TO_128KB(x) ((((x) + (1 << 17) - 1) >> 17) << 17)

typedef struct {
    int luma0;    // per frame (or top field)
    int chroma0;    // per frame (or top field)
    int luma1;    // per frame (or bottom field)
    int chroma1;    // per frame (or bottom field)
} MFC_CRC_DATA;

typedef enum {
    MFC_USE_NONE = 0x00,
    MFC_USE_YUV_BUFF = 0x01,
    MFC_USE_STRM_BUFF = 0x10
} mfc_interbuff_status;

typedef enum {
    MFC_UNPACKED_PB = 0,
    MFC_PACKED_PB = 1
} mfc_packed_mode;

typedef struct tag_strm_ref_buf_arg {
    unsigned int strm_ref_y;
    unsigned int mv_ref_yc;
} mfc_strm_ref_buf_arg_t;

typedef struct tag_frame_buf_arg {
    unsigned int luma;
    unsigned int chroma;
} mfc_frame_buf_arg_t;

typedef struct {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN] codec type                                              */
    int in_width;                        /* [IN] width of YUV420 frame to be encoded                     */
    int in_height;                       /* [IN] height of YUV420 frame to be encoded                    */
    int in_profile_level;                /* [IN] profile & level                                         */
    int in_gop_num;                      /* [IN] GOP Number (interval of I-frame)                        */
    int in_frame_qp;                     /* [IN] the quantization parameter of the frame                 */
    int in_frame_P_qp;                   /* [IN] the quantization parameter of the P frame               */
    int in_frame_B_qp;                   /* [IN] the quantization parameter of the B frame               */

    int in_RC_frm_enable;                /* [IN] RC enable (0:disable, 1:frame level RC)                 */
    int in_RC_framerate;                 /* [IN] RC parameter (framerate)                                */
    int in_RC_bitrate;                   /* [IN] RC parameter (bitrate in kbps)                          */
    int in_RC_qbound;                    /* [IN] RC parameter (Q bound)                                  */
    int in_RC_rpara;                     /* [IN] RC parameter (Reaction Coefficient)                     */

    int in_MS_mode;                      /* [IN] Multi-slice mode (0:single, 1:multiple)                 */
    int in_MS_size;                      /* [IN] Multi-slice size (in num. of mb or byte)                */
    int in_mb_refresh;                   /* [IN] Macroblock refresh                                      */
    int in_interlace_mode;               /* [IN] interlace mode(0:progressive, 1:interlace)              */
    int in_BframeNum;                    /* [IN] B frame number                                          */

    int in_pad_ctrl_on;                  /* [IN] Enable (1) / Disable (0) padding                        */
    int in_luma_pad_val;                 /* [IN] pad value if pad_ctrl_on is Enable                      */
    int in_cb_pad_val;
    int in_cr_pad_val;

    int in_frame_map;                    /* [IN] Encoding input NV12 type linear(0) TILE(1)              */

    unsigned int in_mapped_addr;
    mfc_strm_ref_buf_arg_t out_u_addr;
    mfc_strm_ref_buf_arg_t out_p_addr;
    mfc_strm_ref_buf_arg_t out_buf_size;
    unsigned int out_header_size;

    /* MPEG4 Only */
    int in_qpelME_enable;                /* [IN] Quarter-pel MC enable(1:enable, 0:disable)              */
    int in_time_increament_res;          /* [IN] time increment resolution                               */
    int in_time_vop_time_increament;     /* [IN] time increment                                          */
} mfc_enc_init_mpeg4_arg_t;

typedef mfc_enc_init_mpeg4_arg_t mfc_enc_init_h263_arg_t;

typedef struct {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN] codec type                                              */
    int in_width;                        /* [IN] width  of YUV420 frame to be encoded                    */
    int in_height;                       /* [IN] height of YUV420 frame to be encoded                    */
    int in_profile_level;                /* [IN] profile & level                                         */
    int in_gop_num;                      /* [IN] GOP Number (interval of I-frame)                        */
    int in_frame_qp;                     /* [IN] the quantization parameter of the frame                 */
    int in_frame_P_qp;                   /* [IN] the quantization parameter of the P frame               */
    int in_frame_B_qp;                   /* [IN] the quantization parameter of the B frame               */

    int in_RC_frm_enable;                /* [IN] RC enable (0:disable, 1:frame level RC)                 */
    int in_RC_framerate;                 /* [IN]  RC parameter (framerate)                               */
    int in_RC_bitrate;                   /* [IN]  RC parameter (bitrate in kbps)                         */
    int in_RC_qbound;                    /* [IN]  RC parameter (Q bound)                                 */
    int in_RC_rpara;                     /* [IN]  RC parameter (Reaction Coefficient)                    */

    int in_MS_mode;                      /* [IN] Multi-slice mode (0:single, 1:multiple)                 */
    int in_MS_size;                      /* [IN] Multi-slice size (in num. of mb or byte)                */
    int in_mb_refresh;                   /* [IN] Macroblock refresh                                      */
    int in_interlace_mode;               /* [IN] interlace mode(0:progressive, 1:interlace)              */
    int in_BframeNum;

    int in_pad_ctrl_on;                  /* [IN] Enable padding control                                  */
    int in_luma_pad_val;                 /* [IN] Luma pel value used to fill padding area                */
    int in_cb_pad_val;                   /* [IN] CB pel value used to fill padding area                  */
    int in_cr_pad_val;                   /* [IN] CR pel value used to fill padding area                  */

    int in_frame_map;                    /* [IN] Encoding input NV12 type linear(0) TILE(1)              */

    unsigned int in_mapped_addr;
    mfc_strm_ref_buf_arg_t out_u_addr;
    mfc_strm_ref_buf_arg_t out_p_addr;
    mfc_strm_ref_buf_arg_t out_buf_size;
    unsigned int out_header_size;

    /* H264 Only */
    int in_RC_mb_enable;                 /* [IN] RC enable (0:disable, 1:MB level RC)                    */
    int in_reference_num;                /* [IN] The number of reference pictures used                   */
    int in_ref_num_p;                    /* [IN] The number of reference pictures used for P pictures    */
    int in_RC_mb_dark_disable;           /* [IN] Disable adaptive rate control on dark region            */
    int in_RC_mb_smooth_disable;         /* [IN] Disable adaptive rate control on smooth region          */
    int in_RC_mb_static_disable;         /* [IN] Disable adaptive rate control on static region          */
    int in_RC_mb_activity_disable;       /* [IN] Disable adaptive rate control on static region          */
    int in_deblock_filt;                 /* [IN] disable the loop filter                                 */
    int in_deblock_alpha_C0;             /* [IN] Alpha & C0 offset for H.264 loop filter                 */
    int in_deblock_beta;                 /* [IN] Beta offset for H.264 loop filter                       */
    int in_symbolmode;                   /* [IN] The mode of entropy coding(CABAC, CAVLC)                */
    int in_transform8x8_mode;            /* [IN] Allow 8x8 transform(only for high profile)              */
    int in_md_interweight_pps;           /* [IN] Inter weighted parameter for mode decision              */
    int in_md_intraweight_pps;           /* [IN] Intra weighted parameter for mode decision              */
} mfc_enc_init_h264_arg_t;

typedef struct {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN]  codec type                                             */
    unsigned int in_Y_addr;              /* [IN]  In-buffer addr of Y component                          */
    unsigned int in_CbCr_addr;           /* [IN]  In-buffer addr of CbCr component                       */
    unsigned int in_Y_addr_vir;          /* [IN]  In-buffer addr of Y component                          */
    unsigned int in_CbCr_addr_vir;       /* [IN]  In-buffer addr of CbCr component                       */
    unsigned int in_strm_st;             /* [IN]  Out-buffer start addr of encoded strm                  */
    unsigned int in_strm_end;            /* [IN]  Out-buffer end addr of encoded strm                    */
    int in_frametag;                     /* [IN]  unique frame ID                                        */

    unsigned int out_frame_type;         /* [OUT] frame type                                             */
    int out_encoded_size;                /* [OUT] Length of Encoded video stream                         */
    unsigned int out_encoded_Y_paddr;    /* [OUT] physical Y address which is flushed                    */
    unsigned int out_encoded_C_paddr;    /* [OUT] physical C address which is flushed                    */
    int out_frametag_top;                /* [OUT] unique frame ID of an output frame or top field        */
    int out_frametag_bottom;             /* [OUT] unique frame ID of bottom field                        */
} mfc_enc_exe_arg;

typedef struct {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN]  codec type                                             */
    unsigned int in_strm_buf;            /* [IN]  the physical address of STRM_BUF                       */
    int in_strm_size;                    /* [IN]  size of video stream filled in STRM_BUF                */
    int in_packed_PB;                    /* [IN]  Is packed PB frame or not, 1: packedPB  0: unpacked    */

    int out_img_width;                   /* [OUT] width  of YUV420 frame                                 */
    int out_img_height;                  /* [OUT] height of YUV420 frame                                 */
    int out_buf_width;                   /* [OUT] width  of YUV420 frame                                 */
    int out_buf_height;                  /* [OUT] height of YUV420 frame                                 */
    int out_dpb_cnt;                     /* [OUT] the number of buffers which is nessary during decoding */

    int out_crop_top_offset;             /* [OUT] crop information, top offset                           */
    int out_crop_bottom_offset;          /* [OUT] crop information, bottom offset                        */
    int out_crop_left_offset;            /* [OUT] crop information, left offset                          */
    int out_crop_right_offset;           /* [OUT] crop information, right offset                         */

    mfc_frame_buf_arg_t in_frm_buf;      /* [IN] the address of dpb FRAME_BUF                            */
    mfc_frame_buf_arg_t in_frm_size;     /* [IN] size of dpb FRAME_BUF                                   */
    unsigned int in_mapped_addr;

    mfc_frame_buf_arg_t out_u_addr;
    mfc_frame_buf_arg_t out_p_addr;
    mfc_frame_buf_arg_t out_frame_buf_size;
} mfc_dec_init_arg_t;

typedef struct {
    SSBSIP_MFC_CODEC_TYPE in_codec_type; /* [IN]  codec type                                             */
    unsigned int in_strm_buf;            /* [IN]  the physical address of STRM_BUF                       */
    int in_strm_size;                    /* [IN]  Size of video stream filled in STRM_BUF                */
    mfc_frame_buf_arg_t in_frm_buf;      /* [IN]  the address of dpb FRAME_BUF                           */
    mfc_frame_buf_arg_t in_frm_size;     /* [IN]  size of dpb FRAME_BUF                                  */
    int in_frametag;                     /* [IN]  unique frame ID                                        */

    unsigned int out_display_Y_addr;     /* [OUT] the physical address of display buf                    */
    unsigned int out_display_C_addr;     /* [OUT] the physical address of display buf                    */
    int out_display_status;              /* [OUT] whether display frame exist or not.                    */
    int out_timestamp_top;               /* [OUT] presentation time of an output frame or top field      */
    int out_timestamp_bottom;            /* [OUT] presentation time of bottom field                      */
    int out_consume_bytes;               /* [OUT] consumed bytes when decoding finished                  */
    int out_frametag_top;                /* [OUT] unique frame ID of an output frame or top field        */
    int out_frametag_bottom;             /* [OUT] unique frame ID of bottom field                        */
    int out_res_change;                  /* [OUT] whether resolution is changed or not (0, 1, 2)         */
    int out_crop_top_offset;             /* [OUT] crop information, top offset                           */
    int out_crop_bottom_offset;          /* [OUT] crop information, bottom offset                        */
    int out_crop_left_offset;            /* [OUT] crop information, left offset                          */
    int out_crop_right_offset;           /* [OUT] crop information, right offset                         */
} mfc_dec_exe_arg_t;

typedef struct {
    int in_config_param;                 /* [IN]  Configurable parameter type                            */
    int out_config_value[4];             /* [IN]  Values to get for the configurable parameter.          */
} mfc_get_config_arg_t;

typedef struct {
    int in_config_param;                 /* [IN]  Configurable parameter type                            */
    int in_config_value[2];              /* [IN]  Values to be set for the configurable parameter.       */
    int out_config_value_old[2];         /* [OUT] Old values of the configurable parameters              */
} mfc_set_config_arg_t;

typedef struct tag_get_phys_addr_arg
{
    unsigned int u_addr;
    unsigned int p_addr;
} mfc_get_phys_addr_arg_t;

typedef struct tag_mem_alloc_arg
{
    SSBSIP_MFC_CODEC_TYPE codec_type;
    int buff_size;
    unsigned int mapped_addr;
    unsigned int out_uaddr;
    unsigned int out_paddr;
} mfc_mem_alloc_arg_t;

typedef struct tag_mem_free_arg_t
{
    unsigned int u_addr;
} mfc_mem_free_arg_t;

typedef enum {
	MFC_BUFFER_NO_CACHE = 0,
	MFC_BUFFER_CACHE = 1
} mfc_buffer_type;

typedef union {
    mfc_enc_init_mpeg4_arg_t enc_init_mpeg4;
    mfc_enc_init_h263_arg_t enc_init_h263;
    mfc_enc_init_h264_arg_t enc_init_h264;
    mfc_enc_exe_arg enc_exe;

    mfc_dec_init_arg_t dec_init;
    mfc_dec_exe_arg_t dec_exe;

    mfc_get_config_arg_t get_config;
    mfc_set_config_arg_t set_config;

    mfc_mem_alloc_arg_t mem_alloc;
    mfc_mem_free_arg_t mem_free;
    mfc_get_phys_addr_arg_t get_phys_addr;

    mfc_buffer_type buf_type;
} mfc_args;

typedef struct tag_mfc_args {
    SSBSIP_MFC_ERROR_CODE ret_code; /* [OUT] error code */
    mfc_args args;
} mfc_common_args;

typedef struct {
    int magic;
    int hMFC;
    int width;
    int height;
    int sizeStrmBuf;
    mfc_frame_buf_arg_t sizeFrmBuf;
    int displayStatus;
    int inter_buff_status;
    unsigned int virFreeStrmAddr;
    unsigned int phyStrmBuf;
    unsigned int virStrmBuf;
    unsigned int virMvRefYC;
    mfc_frame_buf_arg_t phyFrmBuf;
    mfc_frame_buf_arg_t virFrmBuf;
    unsigned int mapped_addr;
    mfc_common_args MfcArg;
    SSBSIP_MFC_CODEC_TYPE codec_type;
    SSBSIP_MFC_DEC_OUTPUT_INFO decOutInfo;
    unsigned int encodedHeaderSize;
    int encodedDataSize;
    unsigned int encodedframeType;
    int in_frametag;
    int out_frametag_top;
    int out_frametag_bottom;
    unsigned int encoded_Y_paddr;
    unsigned int encoded_C_paddr;
    unsigned int encode_cnt;
} _MFCLIB;

#endif /* _MFC_INTERFACE_H_ */
