/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * JPEG DRIVER MODULE (JpegEncoder.h)
 * Author  : ge.lee       -- initial version
 * Date    : 03 June 2010
 * Purpose : This file implements the JPEG encoder APIs as needed by Camera HAL
 */
#ifndef __JPG_API_H__
#define __JPG_API_H__

#include <stdint.h>
#include <sys/ioctl.h>

#include "Exif.h"

namespace android {
#define MAX_JPG_WIDTH                   800
#define MAX_JPG_HEIGHT                  480
#define MAX_JPG_RESOLUTION              (MAX_JPG_WIDTH * MAX_JPG_HEIGHT)

#define MAX_JPG_THUMBNAIL_WIDTH         320
#define MAX_JPG_THUMBNAIL_HEIGHT        240
#define MAX_JPG_THUMBNAIL_RESOLUTION    (MAX_JPG_THUMBNAIL_WIDTH *  \
                                            MAX_JPG_THUMBNAIL_HEIGHT)

#define MAX_RGB_WIDTH                   800
#define MAX_RGB_HEIGHT                  480
#define MAX_RGB_RESOLUTION              (MAX_RGB_WIDTH * MAX_RGB_HEIGHT)

/*******************************************************************************/
/* define JPG & image memory */
/* memory area is 4k(PAGE_SIZE) aligned because of VirtualCopyEx() */
#define JPG_STREAM_BUF_SIZE     \
        (MAX_JPG_RESOLUTION / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_STREAM_THUMB_BUF_SIZE   \
        (MAX_JPG_THUMBNAIL_RESOLUTION / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_FRAME_BUF_SIZE  \
        ((MAX_JPG_RESOLUTION * 3) / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_FRAME_THUMB_BUF_SIZE    \
        ((MAX_JPG_THUMBNAIL_RESOLUTION * 3) / PAGE_SIZE + 1) * PAGE_SIZE
#define JPG_RGB_BUF_SIZE    \
        ((MAX_RGB_RESOLUTION * 4) / PAGE_SIZE + 1) * PAGE_SIZE

#define JPG_TOTAL_BUF_SIZE  (JPG_STREAM_BUF_SIZE + \
                             JPG_STREAM_THUMB_BUF_SIZE + \
                             JPG_FRAME_BUF_SIZE + \
                             JPG_FRAME_THUMB_BUF_SIZE + \
                             JPG_RGB_BUF_SIZE)

#define JPG_MAIN_START      0x00
#define JPG_THUMB_START     JPG_STREAM_BUF_SIZE
#define IMG_MAIN_START      (JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE)
#define IMG_THUMB_START     (IMG_MAIN_START + JPG_FRAME_BUF_SIZE)
/*******************************************************************************/

#define JPG_DRIVER_NAME     "/dev/s3c-jpg"

#define JPEG_IOCTL_MAGIC                'J'
#define IOCTL_JPG_DECODE                _IO(JPEG_IOCTL_MAGIC, 1)
#define IOCTL_JPG_ENCODE                _IO(JPEG_IOCTL_MAGIC, 2)
#define IOCTL_JPG_GET_STRBUF            _IO(JPEG_IOCTL_MAGIC, 3)
#define IOCTL_JPG_GET_FRMBUF            _IO(JPEG_IOCTL_MAGIC, 4)
#define IOCTL_JPG_GET_THUMB_STRBUF      _IO(JPEG_IOCTL_MAGIC, 5)
#define IOCTL_JPG_GET_THUMB_FRMBUF      _IO(JPEG_IOCTL_MAGIC, 6)
#define IOCTL_JPG_GET_PHY_FRMBUF        _IO(JPEG_IOCTL_MAGIC, 7)
#define IOCTL_JPG_GET_PHY_THUMB_FRMBUF  _IO(JPEG_IOCTL_MAGIC, 8)

typedef enum {
    JPEG_SET_ENCODE_WIDTH,
    JPEG_SET_ENCODE_HEIGHT,
    JPEG_SET_ENCODE_QUALITY,
    JPEG_SET_ENCODE_IN_FORMAT,
    JPEG_SET_SAMPING_MODE,
    JPEG_SET_THUMBNAIL_WIDTH,
    JPEG_SET_THUMBNAIL_HEIGHT
} jpeg_conf;

typedef enum {
    JPG_FAIL,
    JPG_SUCCESS,
    OK_HD_PARSING,
    ERR_HD_PARSING,
    OK_ENC_OR_DEC,
    ERR_ENC_OR_DEC,
    ERR_UNKNOWN
} jpg_return_status;

typedef enum {
    JPG_RGB16,
    JPG_YCBYCR,
    JPG_TYPE_UNKNOWN
} image_type_t;

typedef enum {
    JPG_444,
    JPG_422,
    JPG_420,
    JPG_400,
    RESERVED1,
    RESERVED2,
    JPG_411,
    JPG_SAMPLE_UNKNOWN
} sample_mode_t;

typedef enum {
    YCBCR_422,
    YCBCR_420,
    YCBCR_SAMPLE_UNKNOWN
} out_mode_t;

typedef enum {
    JPG_MODESEL_YCBCR = 1,
    JPG_MODESEL_RGB,
    JPG_MODESEL_UNKNOWN
} in_mode_t;

typedef enum {
    JPG_MAIN,
    JPG_THUMBNAIL
} encode_type_t;

typedef enum {
    JPG_QUALITY_LEVEL_1,        /* high */
    JPG_QUALITY_LEVEL_2,
    JPG_QUALITY_LEVEL_3,
    JPG_QUALITY_LEVEL_4         /* low */
} image_quality_type_t;

typedef struct {
    sample_mode_t   sample_mode;
    encode_type_t   dec_type;
    out_mode_t      out_format;
    uint32_t        width;
    uint32_t        height;
    uint32_t        data_size;
    uint32_t        file_size;
} jpg_dec_proc_param;

typedef struct {
    sample_mode_t       sample_mode;
    encode_type_t       enc_type;
    in_mode_t           in_format;
    image_quality_type_t quality;
    uint32_t            width;
    uint32_t            height;
    uint32_t            data_size;
    uint32_t            file_size;
    uint32_t            set_framebuf;
} jpg_enc_proc_param;

typedef struct {
    char    *in_buf;
    char    *phy_in_buf;
    int     in_buf_size;
    char    *out_buf;
    char    *phy_out_buf;
    int     out_buf_size;
    char    *in_thumb_buf;
    char    *phy_in_thumb_buf;
    int     in_thumb_buf_size;
    char    *out_thumb_buf;
    char    *phy_out_thumb_buf;
    int     out_thumb_buf_size;
    char    *mmapped_addr;
    jpg_dec_proc_param  *dec_param;
    jpg_enc_proc_param  *enc_param;
    jpg_enc_proc_param  *thumb_enc_param;
} jpg_args;

class JpegEncoder {
public:
    JpegEncoder();
    virtual ~JpegEncoder();

    int openHardware();
    jpg_return_status setConfig(jpeg_conf type, int32_t value);
    void *getInBuf(uint64_t size);
    void *getOutBuf(uint64_t *size);
    void *getThumbInBuf(uint64_t size);
    void *getThumbOutBuf(uint64_t *size);
    jpg_return_status encode(unsigned int *size, exif_attribute_t *exifInfo);
    jpg_return_status encodeThumbImg(unsigned int *size, bool useMain = true);
    jpg_return_status makeExif(unsigned char *exifOut,
                               exif_attribute_t *exifIn,
                               unsigned int *size,
                               bool useMainbufForThumb = false);

private:
    jpg_return_status checkMcu(sample_mode_t sampleMode, uint32_t width, uint32_t height, bool isThumb);
    bool pad(char *srcBuf, uint32_t srcWidth, uint32_t srcHight,
             char *dstBuf, uint32_t dstWidth, uint32_t dstHight);
    bool scaleDownYuv422(char *srcBuf, uint32_t srcWidth, uint32_t srcHight,
                         char *dstBuf, uint32_t dstWidth, uint32_t dstHight);

    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 uint32_t value);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 rational_t *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);
    inline void writeExifIfd(unsigned char **pCur,
                                 unsigned short tag,
                                 unsigned short type,
                                 unsigned int count,
                                 unsigned char *pValue,
                                 unsigned int *offset,
                                 unsigned char *start);
    int mDevFd;
    jpg_args mArgs;

    bool available;

};
};
#endif /* __JPG_API_H__ */
