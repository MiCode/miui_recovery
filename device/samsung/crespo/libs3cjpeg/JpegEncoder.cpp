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
 * JPEG DRIVER MODULE (JpegEncoder.cpp)
 * Author  : ge.lee       -- initial version
 * Date    : 03 June 2010
 * Purpose : This file implements the JPEG encoder APIs as needed by Camera HAL
 */
#define LOG_TAG "JpegEncoder"
#define MAIN_DUMP  0
#define THUMB_DUMP 0

#include <utils/Log.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "JpegEncoder.h"

static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };

namespace android {
JpegEncoder::JpegEncoder() : available(false)
{
    mArgs.mmapped_addr = (char *)MAP_FAILED;
    mArgs.enc_param       = NULL;
    mArgs.thumb_enc_param = NULL;

    mDevFd = open(JPG_DRIVER_NAME, O_RDWR);
    if (mDevFd < 0) {
        LOGE("Failed to open the device");
        return;
    }

    mArgs.mmapped_addr = (char *)mmap(0,
                                      JPG_TOTAL_BUF_SIZE,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      mDevFd,
                                      0);

    if (mArgs.mmapped_addr == MAP_FAILED) {
        LOGE("Failed to mmap");
        return;
    }

    mArgs.enc_param = new jpg_enc_proc_param;
    if (mArgs.enc_param == NULL) {
        LOGE("Failed to allocate the memory for enc_param");
        return;
    }
    memset(mArgs.enc_param, 0, sizeof(jpg_enc_proc_param));

    mArgs.thumb_enc_param = new jpg_enc_proc_param;
    if (mArgs.thumb_enc_param == NULL) {
        LOGE("Failed to allocate the memory for thumb_enc_param");
    delete mArgs.enc_param;
        return;
    }
    memset(mArgs.thumb_enc_param, 0, sizeof(jpg_enc_proc_param));

    mArgs.enc_param->sample_mode = JPG_420;
    mArgs.enc_param->enc_type = JPG_MAIN;
    mArgs.thumb_enc_param->sample_mode = JPG_420;
    mArgs.thumb_enc_param->enc_type = JPG_THUMBNAIL;

    available = true;
}

JpegEncoder::~JpegEncoder()
{
    if (mArgs.mmapped_addr != (char*)MAP_FAILED)
        munmap(mArgs.mmapped_addr, JPG_TOTAL_BUF_SIZE);

    delete mArgs.enc_param;

    delete mArgs.thumb_enc_param;

    if (mDevFd > 0)
        close(mDevFd);
}

jpg_return_status JpegEncoder::setConfig(jpeg_conf type, int32_t value)
{
    if (!available)
        return JPG_FAIL;

    jpg_return_status ret = JPG_SUCCESS;

    switch (type) {
    case JPEG_SET_ENCODE_WIDTH:
        if (value < 0 || value > MAX_JPG_WIDTH)
            ret = JPG_FAIL;
        else
            mArgs.enc_param->width = value;
        break;

    case JPEG_SET_ENCODE_HEIGHT:
        if (value < 0 || value > MAX_JPG_HEIGHT)
            ret = JPG_FAIL;
        else
            mArgs.enc_param->height = value;
        break;

    case JPEG_SET_ENCODE_QUALITY:
        if (value < JPG_QUALITY_LEVEL_1 || value > JPG_QUALITY_LEVEL_4)
            ret = JPG_FAIL;
        else
            mArgs.enc_param->quality = (image_quality_type_t)value;
        break;

    case JPEG_SET_ENCODE_IN_FORMAT:
        if (value != JPG_MODESEL_YCBCR && value != JPG_MODESEL_RGB) {
            ret = JPG_FAIL;
        } else {
            mArgs.enc_param->in_format = (in_mode_t)value;
            mArgs.thumb_enc_param->in_format = (in_mode_t)value;
        }
        break;

    case JPEG_SET_SAMPING_MODE:
        if (value != JPG_420 && value != JPG_422) {
            ret = JPG_FAIL;
        } else {
            mArgs.enc_param->sample_mode = (sample_mode_t)value;
            mArgs.thumb_enc_param->sample_mode = (sample_mode_t)value;
        }
        break;

    case JPEG_SET_THUMBNAIL_WIDTH:
        if (value < 0 || value > MAX_JPG_THUMBNAIL_WIDTH)
            ret = JPG_FAIL;
        else
            mArgs.thumb_enc_param->width = value;
        break;

    case JPEG_SET_THUMBNAIL_HEIGHT:
        if (value < 0 || value > MAX_JPG_THUMBNAIL_HEIGHT)
            ret = JPG_FAIL;
        else
            mArgs.thumb_enc_param->height = value;
        break;

    default:
        LOGE("Invalid Config type");
        ret = ERR_UNKNOWN;
    }

    if (ret == JPG_FAIL)
        LOGE("Invalid value(%d) for %d type", value, type);

    return ret;
}

void* JpegEncoder::getInBuf(uint64_t size)
{
    if (!available)
        return NULL;

    if (size > JPG_FRAME_BUF_SIZE) {
        LOGE("The buffer size requested is too large");
        return NULL;
    }
    mArgs.in_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_FRMBUF, mArgs.mmapped_addr);
    return (void *)(mArgs.in_buf);
}

void* JpegEncoder::getOutBuf(uint64_t *size)
{
    if (!available)
        return NULL;

    if (mArgs.enc_param->file_size <= 0) {
        LOGE("The buffer requested doesn't have data");
        return NULL;
    }
    mArgs.out_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_STRBUF, mArgs.mmapped_addr);
    *size = mArgs.enc_param->file_size;
    return (void *)(mArgs.out_buf);
}

void* JpegEncoder::getThumbInBuf(uint64_t size)
{
    if (!available)
        return NULL;

    if (size > JPG_FRAME_THUMB_BUF_SIZE) {
        LOGE("The buffer size requested is too large");
        return NULL;
    }
    mArgs.in_thumb_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_THUMB_FRMBUF, mArgs.mmapped_addr);
    return (void *)(mArgs.in_thumb_buf);
}

void* JpegEncoder::getThumbOutBuf(uint64_t *size)
{
    if (!available)
        return NULL;

    if (mArgs.thumb_enc_param->file_size <= 0) {
        LOGE("The buffer requested doesn't have data");
        return NULL;
    }
    mArgs.out_thumb_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_THUMB_STRBUF, mArgs.mmapped_addr);
    *size = mArgs.thumb_enc_param->file_size;
    return (void *)(mArgs.out_thumb_buf);
}

jpg_return_status JpegEncoder::encode(unsigned int *size, exif_attribute_t *exifInfo)
{
    if (!available)
        return JPG_FAIL;

    LOGD("encode E");

    jpg_return_status ret = JPG_FAIL;
    unsigned char *exifOut = NULL;
    jpg_enc_proc_param *param = mArgs.enc_param;

    ret = checkMcu(param->sample_mode, param->width, param->height, false);
    if (ret != JPG_SUCCESS)
        return ret;

    param->enc_type = JPG_MAIN;
    ret = (jpg_return_status)ioctl(mDevFd, IOCTL_JPG_ENCODE, &mArgs);
    if (ret != JPG_SUCCESS) {
        LOGE("Failed to encode main image");
        return ret;
    }

    mArgs.out_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_STRBUF, mArgs.mmapped_addr);

    if (exifInfo) {
        unsigned int thumbLen, exifLen;

        uint_t bufSize = 0;
        if (exifInfo->enableThumb) {
            ret = encodeThumbImg(&thumbLen);
            if (ret != JPG_SUCCESS) {
                LOGE("Failed to encode for thumbnail image");
                bufSize = EXIF_FILE_SIZE;
                exifInfo->enableThumb = false;
            } else {
                bufSize = EXIF_FILE_SIZE + thumbLen;
            }
        } else {
            bufSize = EXIF_FILE_SIZE;
        }

        if (mArgs.enc_param->file_size + bufSize > JPG_TOTAL_BUF_SIZE)
            return ret;

        exifOut = new unsigned char[bufSize];
        if (exifOut == NULL) {
            LOGE("Failed to allocate for exifOut");
            return ret;
        }
        memset(exifOut, 0, bufSize);

        ret = makeExif (exifOut, exifInfo, &exifLen);
        if (ret != JPG_SUCCESS) {
            LOGE("Failed to make EXIF");
            delete[] exifOut;
            return ret;
        }

        memmove(&mArgs.out_buf[exifLen + 2], &mArgs.out_buf[2], param->file_size - 2);
        memcpy(&mArgs.out_buf[2], exifOut, exifLen);
        param->file_size += exifLen;
    }

    delete[] exifOut;

    *size = param->file_size;

#if MAIN_DUMP
    FILE *fout = NULL;
    char file_name[50] = "/data/main.jpg";
    fout = fopen(file_name, "wb");
    if (!fout)
        perror(&file_name[0]);
    size_t nwrite = fwrite(mArgs.out_buf, sizeof(char), param->file_size, fout);
    fclose(fout);
#endif

    LOGD("encode X");

    return ret;
}

jpg_return_status JpegEncoder::encodeThumbImg(unsigned int *size, bool useMain)
{
    if (!available)
        return JPG_FAIL;

    LOGD("encodeThumbImg E");

    jpg_return_status ret = JPG_FAIL;
    jpg_enc_proc_param *param = mArgs.thumb_enc_param;

    if (useMain) {
        mArgs.in_thumb_buf = (char *)getThumbInBuf(param->width*param->height*2);
        if (mArgs.in_thumb_buf == NULL) {
            LOGE("Failed to get the buffer for thumbnail");
            return JPG_FAIL;
        }

        ret = (jpg_return_status)scaleDownYuv422(mArgs.in_buf,
                                                 mArgs.enc_param->width,
                                                 mArgs.enc_param->height,
                                                 mArgs.in_thumb_buf,
                                                 param->width,
                                                 param->height);
        if (ret != JPG_SUCCESS)
            return JPG_FAIL;
    }

    ret = checkMcu(param->sample_mode, param->width, param->height, true);
    if (ret != JPG_SUCCESS)
        return JPG_FAIL;

    mArgs.enc_param->enc_type = JPG_THUMBNAIL;
    ret = (jpg_return_status)ioctl(mDevFd, IOCTL_JPG_ENCODE, &mArgs);
    if (ret != JPG_SUCCESS) {
        LOGE("Failed to encode for thumbnail");
        return JPG_FAIL;
    }

    mArgs.out_thumb_buf = (char *)ioctl(mDevFd, IOCTL_JPG_GET_THUMB_STRBUF, mArgs.mmapped_addr);

#if THUMB_DUMP
    FILE *fout = NULL;
    char file_name[50] = "/data/thumb.jpg";
    fout = fopen(file_name, "wb");
    if (!fout)
        perror(&file_name[0]);
    size_t nwrite = fwrite(mArgs.out_thumb_buf, sizeof(char), param->file_size, fout);
    fclose(fout);
#endif

    LOGD("encodeThumbImg X");

    return JPG_SUCCESS;
}

jpg_return_status JpegEncoder::makeExif (unsigned char *exifOut,
                                        exif_attribute_t *exifInfo,
                                        unsigned int *size,
                                        bool useMainbufForThumb)
{
    if (!available)
        return JPG_FAIL;

    LOGD("makeExif E");

    unsigned char *pCur, *pApp1Start, *pIfdStart, *pGpsIfdPtr, *pNextIfdOffset;
    unsigned int tmp, LongerTagOffest = 0;
    pApp1Start = pCur = exifOut;

    //2 Exif Identifier Code & TIFF Header
    pCur += 4;  // Skip 4 Byte for APP1 marker and length
    unsigned char ExifIdentifierCode[6] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
    memcpy(pCur, ExifIdentifierCode, 6);
    pCur += 6;

    /* Byte Order - little endian, Offset of IFD - 0x00000008.H */
    unsigned char TiffHeader[8] = { 0x49, 0x49, 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00 };
    memcpy(pCur, TiffHeader, 8);
    pIfdStart = pCur;
    pCur += 8;

    //2 0th IFD TIFF Tags
    if (exifInfo->enableGps)
        tmp = NUM_0TH_IFD_TIFF;
    else
        tmp = NUM_0TH_IFD_TIFF - 1;

    memcpy(pCur, &tmp, NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += 8 + NUM_SIZE + tmp*IFD_SIZE + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_MAKE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->maker) + 1, exifInfo->maker, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_MODEL, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->model) + 1, exifInfo->model, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                 1, exifInfo->orientation);
    writeExifIfd(&pCur, EXIF_TAG_SOFTWARE, EXIF_TYPE_ASCII,
                 strlen((char *)exifInfo->software) + 1, exifInfo->software, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_YCBCR_POSITIONING, EXIF_TYPE_SHORT,
                 1, exifInfo->ycbcr_positioning);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_IFD_POINTER, EXIF_TYPE_LONG,
                 1, LongerTagOffest);
    if (exifInfo->enableGps) {
        pGpsIfdPtr = pCur;
        pCur += IFD_SIZE;   // Skip a ifd size for gps IFD pointer
    }

    pNextIfdOffset = pCur;  // Skip a offset size for next IFD offset
    pCur += OFFSET_SIZE;

    //2 0th IFD Exif Private Tags
    pCur = pIfdStart + LongerTagOffest;

    tmp = NUM_0TH_IFD_EXIF;
    memcpy(pCur, &tmp , NUM_SIZE);
    pCur += NUM_SIZE;

    LongerTagOffest += NUM_SIZE + NUM_0TH_IFD_EXIF*IFD_SIZE + OFFSET_SIZE;

    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_TIME, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->exposure_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_FNUMBER, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->fnumber, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_PROGRAM, EXIF_TYPE_SHORT,
                 1, exifInfo->exposure_program);
    writeExifIfd(&pCur, EXIF_TAG_ISO_SPEED_RATING, EXIF_TYPE_SHORT,
                 1, exifInfo->iso_speed_rating);
    writeExifIfd(&pCur, EXIF_TAG_EXIF_VERSION, EXIF_TYPE_UNDEFINED,
                 4, exifInfo->exif_version);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_ORG, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_DATE_TIME_DIGITIZE, EXIF_TYPE_ASCII,
                 20, exifInfo->date_time, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_SHUTTER_SPEED, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->shutter_speed, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_APERTURE, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->aperture, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_BRIGHTNESS, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->brightness, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_BIAS, EXIF_TYPE_SRATIONAL,
                 1, (rational_t *)&exifInfo->exposure_bias, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_MAX_APERTURE, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->max_aperture, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_METERING_MODE, EXIF_TYPE_SHORT,
                 1, exifInfo->metering_mode);
    writeExifIfd(&pCur, EXIF_TAG_FLASH, EXIF_TYPE_SHORT,
                 1, exifInfo->flash);
    writeExifIfd(&pCur, EXIF_TAG_FOCAL_LENGTH, EXIF_TYPE_RATIONAL,
                 1, &exifInfo->focal_length, &LongerTagOffest, pIfdStart);
    char code[8] = { 0x00, 0x00, 0x00, 0x49, 0x49, 0x43, 0x53, 0x41 };
    int commentsLen = strlen((char *)exifInfo->user_comment) + 1;
    memmove(exifInfo->user_comment + sizeof(code), exifInfo->user_comment, commentsLen);
    memcpy(exifInfo->user_comment, code, sizeof(code));
    writeExifIfd(&pCur, EXIF_TAG_USER_COMMENT, EXIF_TYPE_UNDEFINED,
                 commentsLen + sizeof(code), exifInfo->user_comment, &LongerTagOffest, pIfdStart);
    writeExifIfd(&pCur, EXIF_TAG_COLOR_SPACE, EXIF_TYPE_SHORT,
                 1, exifInfo->color_space);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_X_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->width);
    writeExifIfd(&pCur, EXIF_TAG_PIXEL_Y_DIMENSION, EXIF_TYPE_LONG,
                 1, exifInfo->height);
    writeExifIfd(&pCur, EXIF_TAG_EXPOSURE_MODE, EXIF_TYPE_LONG,
                 1, exifInfo->exposure_mode);
    writeExifIfd(&pCur, EXIF_TAG_WHITE_BALANCE, EXIF_TYPE_LONG,
                 1, exifInfo->white_balance);
    writeExifIfd(&pCur, EXIF_TAG_SCENCE_CAPTURE_TYPE, EXIF_TYPE_LONG,
                 1, exifInfo->scene_capture_type);
    tmp = 0;
    memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
    pCur += OFFSET_SIZE;

    //2 0th IFD GPS Info Tags
    if (exifInfo->enableGps) {
        writeExifIfd(&pGpsIfdPtr, EXIF_TAG_GPS_IFD_POINTER, EXIF_TYPE_LONG,
                     1, LongerTagOffest); // GPS IFD pointer skipped on 0th IFD

        pCur = pIfdStart + LongerTagOffest;

        if (exifInfo->gps_processing_method[0] == 0) {
            // don't create GPS_PROCESSING_METHOD tag if there isn't any
            tmp = NUM_0TH_IFD_GPS - 1;
        } else {
            tmp = NUM_0TH_IFD_GPS;
        }
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + tmp*IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_GPS_VERSION_ID, EXIF_TYPE_BYTE,
                     4, exifInfo->gps_version_id);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_latitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LATITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_latitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE_REF, EXIF_TYPE_ASCII,
                     2, exifInfo->gps_longitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_LONGITUDE, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_longitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE_REF, EXIF_TYPE_BYTE,
                     1, exifInfo->gps_altitude_ref);
        writeExifIfd(&pCur, EXIF_TAG_GPS_ALTITUDE, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->gps_altitude, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_GPS_TIMESTAMP, EXIF_TYPE_RATIONAL,
                     3, exifInfo->gps_timestamp, &LongerTagOffest, pIfdStart);
        tmp = strlen((char*)exifInfo->gps_processing_method);
	if (tmp > 0) {
            if (tmp > 100) {
                tmp = 100;
            }
            unsigned char tmp_buf[100+sizeof(ExifAsciiPrefix)];
            memcpy(tmp_buf, ExifAsciiPrefix, sizeof(ExifAsciiPrefix));
            memcpy(&tmp_buf[sizeof(ExifAsciiPrefix)], exifInfo->gps_processing_method, tmp);
            writeExifIfd(&pCur, EXIF_TAG_GPS_PROCESSING_METHOD, EXIF_TYPE_UNDEFINED,
                         tmp+sizeof(ExifAsciiPrefix), tmp_buf, &LongerTagOffest, pIfdStart);
        }
        writeExifIfd(&pCur, EXIF_TAG_GPS_DATESTAMP, EXIF_TYPE_ASCII,
                     11, exifInfo->gps_datestamp, &LongerTagOffest, pIfdStart);
        tmp = 0;
        memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
        pCur += OFFSET_SIZE;
    }

    //2 1th IFD TIFF Tags
    char *thumbBuf;
    int thumbSize;

    if (useMainbufForThumb) {
        thumbBuf = mArgs.out_buf;
        thumbSize = mArgs.enc_param->file_size;
    } else {
        thumbBuf = mArgs.out_thumb_buf;
        thumbSize = mArgs.thumb_enc_param->file_size;
    }

    if (exifInfo->enableThumb && (thumbBuf != NULL) && (thumbSize > 0)) {
        tmp = LongerTagOffest;
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);  // NEXT IFD offset skipped on 0th IFD

        pCur = pIfdStart + LongerTagOffest;

        tmp = NUM_1TH_IFD_TIFF;
        memcpy(pCur, &tmp, NUM_SIZE);
        pCur += NUM_SIZE;

        LongerTagOffest += NUM_SIZE + NUM_1TH_IFD_TIFF*IFD_SIZE + OFFSET_SIZE;

        writeExifIfd(&pCur, EXIF_TAG_IMAGE_WIDTH, EXIF_TYPE_LONG,
                     1, exifInfo->widthThumb);
        writeExifIfd(&pCur, EXIF_TAG_IMAGE_HEIGHT, EXIF_TYPE_LONG,
                     1, exifInfo->heightThumb);
        writeExifIfd(&pCur, EXIF_TAG_COMPRESSION_SCHEME, EXIF_TYPE_SHORT,
                     1, exifInfo->compression_scheme);
        writeExifIfd(&pCur, EXIF_TAG_ORIENTATION, EXIF_TYPE_SHORT,
                     1, exifInfo->orientation);
        writeExifIfd(&pCur, EXIF_TAG_X_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->x_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_Y_RESOLUTION, EXIF_TYPE_RATIONAL,
                     1, &exifInfo->y_resolution, &LongerTagOffest, pIfdStart);
        writeExifIfd(&pCur, EXIF_TAG_RESOLUTION_UNIT, EXIF_TYPE_SHORT,
                     1, exifInfo->resolution_unit);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT, EXIF_TYPE_LONG,
                     1, LongerTagOffest);
        writeExifIfd(&pCur, EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LEN, EXIF_TYPE_LONG,
                     1, thumbSize);

        tmp = 0;
        memcpy(pCur, &tmp, OFFSET_SIZE); // next IFD offset
        pCur += OFFSET_SIZE;

        memcpy(pIfdStart + LongerTagOffest,
               thumbBuf, thumbSize);
        LongerTagOffest += thumbSize;
    } else {
        tmp = 0;
        memcpy(pNextIfdOffset, &tmp, OFFSET_SIZE);  // NEXT IFD offset skipped on 0th IFD
    }

    unsigned char App1Marker[2] = { 0xff, 0xe1 };
    memcpy(pApp1Start, App1Marker, 2);
    pApp1Start += 2;

    *size = 10 + LongerTagOffest;
    tmp = *size - 2;    // APP1 Maker isn't counted
    unsigned char size_mm[2] = {(tmp >> 8) & 0xFF, tmp & 0xFF};
    memcpy(pApp1Start, size_mm, 2);

    LOGD("makeExif X");

    return JPG_SUCCESS;
}

jpg_return_status JpegEncoder::checkMcu(sample_mode_t sampleMode,
                                        uint32_t width, uint32_t height, bool isThumb)
{
    if (!available)
        return JPG_FAIL;

    uint32_t expectedWidth = width;
    uint32_t expectedHeight = height;

    switch (sampleMode){
    case JPG_422:
        if (width % 16 != 0)
            expectedWidth = width + 16 - (width % 16);
        if (height % 8 != 0)
            expectedHeight = height + 8 - (height % 8);
        break;

    case JPG_420:
        if (width % 16 != 0)
            expectedWidth = width + 16 - (width % 16);
        if (height % 16 != 0)
            expectedHeight = height + 16 - (height % 16);
        break;

    default:
        LOGE("Invaild sample mode");
        return JPG_FAIL;
    }

    if (expectedWidth == width && expectedHeight == height)
        return JPG_SUCCESS;

    LOGW("The image is not matched for MCU");

    uint32_t size = width*height * 2;
    char *srcBuf, *dstBuf;

    if ((srcBuf = new char[size]) == NULL) {
        LOGE("Failed to allocate for srcBuf");
        return JPG_FAIL;
    }

    if (!isThumb)
        dstBuf = mArgs.in_buf;
    else
        dstBuf = mArgs.in_thumb_buf;

    memcpy(srcBuf, dstBuf, size);
    bool ret = pad(srcBuf, width, height, dstBuf, expectedWidth, expectedHeight);

    delete[] srcBuf;

    return JPG_SUCCESS;
}

bool JpegEncoder::pad(char *srcBuf, uint32_t srcWidth, uint32_t srcHight,
                        char *dstBuf, uint32_t dstWidth, uint32_t dstHight)
{
    if (!available)
        return false;

    if (srcBuf == NULL || dstBuf == NULL) {
        LOGE("srcBuf or dstBuf is NULL");
        return false;
    }

    int padW = dstWidth - srcWidth;
    int padH = dstHight - srcHight;

    if ((int)(dstWidth - srcWidth) < 0 ||
            (int)(dstHight - srcHight) < 0) {
        LOGE("dstSize is smaller than srcSize");
        return false;
    }
    memset(dstBuf, 0, dstWidth*dstHight * 2);

    for (uint32_t i = 0; i < srcHight; i++)
        memcpy(dstBuf + i * dstWidth * 2, srcBuf + i * srcWidth * 2, srcWidth * 2);

    return true;
}

bool JpegEncoder::scaleDownYuv422(char *srcBuf, uint32_t srcWidth, uint32_t srcHight,
                                  char *dstBuf, uint32_t dstWidth, uint32_t dstHight)
{
    if (!available)
        return false;

    int32_t step_x, step_y;
    int32_t iXsrc, iXdst;
    int32_t x, y, src_y_start_pos, dst_pos, src_pos;

    if (dstWidth % 2 != 0 || dstHight % 2 != 0){
        LOGE("scale_down_yuv422: invalid width, height for scaling");
        return false;
    }

    step_x = srcWidth / dstWidth;
    step_y = srcHight / dstHight;

    dst_pos = 0;
    for (uint32_t y = 0; y < dstHight; y++) {
        src_y_start_pos = (y * step_y * (srcWidth * 2));

        for (uint32_t x = 0; x < dstWidth; x += 2) {
            src_pos = src_y_start_pos + (x * (step_x * 2));

            dstBuf[dst_pos++] = srcBuf[src_pos    ];
            dstBuf[dst_pos++] = srcBuf[src_pos + 1];
            dstBuf[dst_pos++] = srcBuf[src_pos + 2];
            dstBuf[dst_pos++] = srcBuf[src_pos + 3];
        }
    }

    return true;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         uint32_t value)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, &value, 4);
    *pCur += 4;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         unsigned char *pValue)
{
    char buf[4] = { 0,};

    memcpy(buf, pValue, count);
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, buf, 4);
    *pCur += 4;
}


inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         unsigned char *pValue,
                                         unsigned int *offset,
                                         unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, count);
    *offset += count;
}

inline void JpegEncoder::writeExifIfd(unsigned char **pCur,
                                         unsigned short tag,
                                         unsigned short type,
                                         unsigned int count,
                                         rational_t *pValue,
                                         unsigned int *offset,
                                         unsigned char *start)
{
    memcpy(*pCur, &tag, 2);
    *pCur += 2;
    memcpy(*pCur, &type, 2);
    *pCur += 2;
    memcpy(*pCur, &count, 4);
    *pCur += 4;
    memcpy(*pCur, offset, 4);
    *pCur += 4;
    memcpy(start + *offset, pValue, 8 * count);
    *offset += 8 * count;
}

};
