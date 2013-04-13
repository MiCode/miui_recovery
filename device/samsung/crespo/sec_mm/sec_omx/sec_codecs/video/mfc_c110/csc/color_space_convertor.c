/*
 *
 * Copyright 2011 Samsung Electronics S.LSI Co. LTD
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
 * @file    color_space_convertor.c
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

#include "stdlib.h"
#include "color_space_convertor.h"

#define TILED_SIZE  64*32

/*
 * De-interleaves src to dest1, dest2
 *
 * @param dest1
 *   Address of de-interleaved data[out]
 *
 * @param dest2
 *   Address of de-interleaved data[out]
 *
 * @param src
 *   Address of interleaved data[in]
 *
 * @param src_size
 *   Size of interleaved data[in]
 */
void csc_deinterleave_memcpy(char *dest1, char *dest2, char *src, int src_size)
{
    int i = 0;
    for(i=0; i<src_size/2; i++) {
        dest1[i] = src[i*2];
        dest2[i] = src[i*2+1];
    }
}

/*
 * Interleaves src1, src2 to dest
 *
 * @param dest
 *   Address of interleaved data[out]
 *
 * @param src1
 *   Address of de-interleaved data[in]
 *
 * @param src2
 *   Address of de-interleaved data[in]
 *
 * @param src_size
 *   Size of de-interleaved data[in]
 */
void csc_interleave_memcpy(char *dest, char *src1, char *src2, int src_size)
{
    int i = 0;
    for(i=0; i<src_size; i++) {
        dest[i*2] = src1[i];
        dest[i*2+1] = src2[i];
    }
}

/*
 * Converts tiled data to linear.
 * 1. Y of NV12T to Y of YUV420P
 * 2. Y of NV12T to Y of YUV420S
 * 3. UV of NV12T to UV of YUV420S
 *
 * @param yuv420_dest
 *   Y or UV plane address of YUV420[out]
 *
 * @param nv12t_src
 *   Y or UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 */
void csc_tiled_to_linear(char *yuv420_dest, char *nv12t_src, int yuv420_width, int yuv420_height)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0;
    unsigned int tiled_offset = 0, tiled_offset1 = 0, tiled_offset2 = 0, tiled_offset3 = 0;
    unsigned int temp1 = 0, temp2 = 0;

    if (yuv420_width >= 1024) {
        for (i=0; i<yuv420_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
                temp2 = 8;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                    temp2 = 8;
                } else {
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                    temp2 = 4;
                }
            }
            temp1 = i&0x1F;
            memcpy(yuv420_dest+yuv420_width*(i), nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*1, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*2, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*3, nv12t_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            memcpy(yuv420_dest+yuv420_width*(i)+64*4, nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*5, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*6, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*7, nv12t_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            memcpy(yuv420_dest+yuv420_width*(i)+64*8, nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*9, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*10, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*11, nv12t_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            memcpy(yuv420_dest+yuv420_width*(i)+64*12, nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*13, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*14, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+yuv420_width*(i)+64*15, nv12t_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = 1024;
    }

    if ((yuv420_width-aligned_x_size) >= 512) {
        for (i=0; i<yuv420_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
                temp2 = 8;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                    temp2 = 8;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                    temp2 = 4;
                }
            }
            temp1 = i&0x1F;
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i), nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*1, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*2, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*3, nv12t_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*4, nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*5, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*6, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*7, nv12t_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = aligned_x_size+512;
    }

    if ((yuv420_width-aligned_x_size) >= 256) {
        for (i=0; i<yuv420_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                }
            }
            temp1 = i&0x1F;
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i), nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*1, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*2, nv12t_src+tiled_offset2+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64*3, nv12t_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = aligned_x_size+256;
    }

    if ((yuv420_width-aligned_x_size) >= 128) {
        for (i=0; i<yuv420_height; i=i+2) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                }
            }
            temp1 = i&0x1F;
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i), nv12t_src+tiled_offset+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i)+64, nv12t_src+tiled_offset1+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i+1), nv12t_src+tiled_offset+64*(temp1+1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i+1)+64, nv12t_src+tiled_offset1+64*(temp1+1), 64);
        }
        aligned_x_size = aligned_x_size+128;
    }

    if ((yuv420_width-aligned_x_size) >= 64) {
        for (i=0; i<yuv420_height; i=i+4) {
            tiled_offset = 0;
            tiled_x_index = aligned_x_size>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            temp1 = i&0x1F;
            temp2 = aligned_x_size&0x3F;
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i), nv12t_src+tiled_offset+temp2+64*(temp1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i+1), nv12t_src+tiled_offset+temp2+64*(temp1+1), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i+2), nv12t_src+tiled_offset+temp2+64*(temp1+2), 64);
            memcpy(yuv420_dest+aligned_x_size+yuv420_width*(i+3), nv12t_src+tiled_offset+temp2+64*(temp1+3), 64);
        }
        aligned_x_size = aligned_x_size+64;
    }

    if (yuv420_width != aligned_x_size) {
        for (i=0; i<yuv420_height; i=i+4) {
            for (j=aligned_x_size; j<yuv420_width; j=j+4) {
                tiled_offset = 0;
                tiled_x_index = j>>6;
                tiled_y_index = i>>5;
                if (tiled_y_index & 0x1) {
                    /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                    tiled_offset = tiled_y_index-1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset+2;
                    temp1 = (tiled_x_index>>2)<<2;
                    tiled_offset = tiled_offset+temp1;
                    tiled_offset = tiled_offset<<11;
                } else {
                    temp2 = ((yuv420_height+31)>>5)<<5;
                    if ((i+32)<temp2) {
                        /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                        temp1 = tiled_x_index+2;
                        temp1 = (temp1>>2)<<2;
                        tiled_offset = tiled_x_index+temp1;
                        temp1 = ((yuv420_width+127)>>7)<<7;
                        tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                        tiled_offset = tiled_offset<<11;
                    } else {
                        /* even2 fomula: x+x_block_num*y */
                        temp1 = ((yuv420_width+127)>>7)<<7;
                        tiled_offset = tiled_y_index*(temp1>>6);
                        tiled_offset = tiled_offset+tiled_x_index;
                        tiled_offset = tiled_offset<<11;
                    }
                }

                temp1 = i&0x1F;
                temp2 = j&0x3F;
                memcpy(yuv420_dest+j+yuv420_width*(i), nv12t_src+tiled_offset+temp2+64*(temp1), 4);
                memcpy(yuv420_dest+j+yuv420_width*(i+1), nv12t_src+tiled_offset+temp2+64*(temp1+1), 4);
                memcpy(yuv420_dest+j+yuv420_width*(i+2), nv12t_src+tiled_offset+temp2+64*(temp1+2), 4);
                memcpy(yuv420_dest+j+yuv420_width*(i+3), nv12t_src+tiled_offset+temp2+64*(temp1+3), 4);
            }
        }
    }
}

/*
 * Converts and Deinterleaves tiled data to linear
 * 1. UV of NV12T to UV of YUV420P
 *
 * @param yuv420_u_dest
 *   U plane address of YUV420P[out]
 *
 * @param yuv420_v_dest
 *   V plane address of YUV420P[out]
 *
 * @param nv12t_src
 *   UV plane address of NV12T[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 */
void csc_tiled_to_linear_deinterleave(char *yuv420_u_dest, char *yuv420_v_dest, char *nv12t_uv_src, int yuv420_width, int yuv420_uv_height)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0;
    unsigned int tiled_offset = 0, tiled_offset1 = 0, tiled_offset2 = 0, tiled_offset3 = 0;
    unsigned int temp1 = 0, temp2 = 0;

    if (yuv420_width >= 1024) {
        for (i=0; i<yuv420_uv_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
                temp2 = 8;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                    temp2 = 8;
                } else {
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                    temp2 = 4;
                }
            }
            temp1 = i&0x1F;
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i), yuv420_v_dest+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*1, yuv420_v_dest+yuv420_width/2*(i)+32*1, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*2, yuv420_v_dest+yuv420_width/2*(i)+32*2, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*3, yuv420_v_dest+yuv420_width/2*(i)+32*3, nv12t_uv_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*4, yuv420_v_dest+yuv420_width/2*(i)+32*4, nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*5, yuv420_v_dest+yuv420_width/2*(i)+32*5, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*6, yuv420_v_dest+yuv420_width/2*(i)+32*6, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*7, yuv420_v_dest+yuv420_width/2*(i)+32*7, nv12t_uv_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*8, yuv420_v_dest+yuv420_width/2*(i)+32*8, nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*9, yuv420_v_dest+yuv420_width/2*(i)+32*9, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*10, yuv420_v_dest+yuv420_width/2*(i)+32*10, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*11, yuv420_v_dest+yuv420_width/2*(i)+32*11, nv12t_uv_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*12, yuv420_v_dest+yuv420_width/2*(i)+32*12, nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*13, yuv420_v_dest+yuv420_width/2*(i)+32*13, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*14, yuv420_v_dest+yuv420_width/2*(i)+32*14, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+yuv420_width/2*(i)+32*15, yuv420_v_dest+yuv420_width/2*(i)+32*15, nv12t_uv_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = 1024;
    }

    if ((yuv420_width-aligned_x_size) >= 512) {
        for (i=0; i<yuv420_uv_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
                temp2 = 8;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                    temp2 = 8;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                    temp2 = 4;
                }
            }
            temp1 = i&0x1F;
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*2, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*2, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*3, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*3, nv12t_uv_src+tiled_offset3+64*(temp1), 64);

            tiled_offset = tiled_offset+temp2*2048;
            tiled_offset1 = tiled_offset1+temp2*2048;
            tiled_offset2 = tiled_offset2+temp2*2048;
            tiled_offset3 = tiled_offset3+temp2*2048;
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*4, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*4, nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*5, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*5, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*6, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*6, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*7, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*7, nv12t_uv_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = aligned_x_size+512;
    }

    if ((yuv420_width-aligned_x_size) >= 256) {
        for (i=0; i<yuv420_uv_height; i=i+1) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
                tiled_offset2 = tiled_offset+2048*2;
                tiled_offset3 = tiled_offset+2048*3;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*6;
                    tiled_offset3 = tiled_offset+2048*7;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                    tiled_offset2 = tiled_offset+2048*2;
                    tiled_offset3 = tiled_offset+2048*3;
                }
            }
            temp1 = i&0x1F;
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*2, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*2, nv12t_uv_src+tiled_offset2+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*3, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*3, nv12t_uv_src+tiled_offset3+64*(temp1), 64);
        }
        aligned_x_size = aligned_x_size+256;
    }

    if ((yuv420_width-aligned_x_size) >= 128) {
        for (i=0; i<yuv420_uv_height; i=i+2) {
            tiled_offset = 0;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+2;
                temp1 = aligned_x_size>>5;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
                tiled_offset1 = tiled_offset+2048*1;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                /* even fomula: x_block_num*y */
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_y_index*(temp1>>6);
                tiled_offset = tiled_offset<<11;
                if ((i+32)<temp2) {
                    temp1 = aligned_x_size>>5;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                } else {
                    temp1 = aligned_x_size>>6;
                    tiled_offset = tiled_offset+(temp1<<11);
                    tiled_offset1 = tiled_offset+2048*1;
                }
            }
            temp1 = i&0x1F;
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i)+32*1, nv12t_uv_src+tiled_offset1+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i+1), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i+1), nv12t_uv_src+tiled_offset+64*(temp1+1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i+1)+32*1, yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i+1)+32*1, nv12t_uv_src+tiled_offset1+64*(temp1+1), 64);
        }
        aligned_x_size = aligned_x_size+128;
    }

    if ((yuv420_width-aligned_x_size) >= 64) {
        for (i=0; i<yuv420_uv_height; i=i+2) {
            tiled_offset = 0;
            tiled_x_index = aligned_x_size>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            temp1 = i&0x1F;
            temp2 = aligned_x_size&0x3F;
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+64*(temp1), 64);
            csc_deinterleave_memcpy(yuv420_u_dest+aligned_x_size/2+yuv420_width/2*(i+1), yuv420_v_dest+aligned_x_size/2+yuv420_width/2*(i+1), nv12t_uv_src+tiled_offset+64*(temp1+1), 64);
        }
        aligned_x_size = aligned_x_size+64;
    }

    if (yuv420_width != aligned_x_size) {
        for (i=0; i<yuv420_uv_height; i=i+2) {
            for (j=aligned_x_size; j<yuv420_width; j=j+4) {
                tiled_offset = 0;
                tiled_x_index = j>>6;
                tiled_y_index = i>>5;
                if (tiled_y_index & 0x1) {
                    /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                    tiled_offset = tiled_y_index-1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset+2;
                    temp1 = (tiled_x_index>>2)<<2;
                    tiled_offset = tiled_offset+temp1;
                    tiled_offset = tiled_offset<<11;
                } else {
                    temp2 = ((yuv420_uv_height+31)>>5)<<5;
                    if ((i+32)<temp2) {
                        /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                        temp1 = tiled_x_index+2;
                        temp1 = (temp1>>2)<<2;
                        tiled_offset = tiled_x_index+temp1;
                        temp1 = ((yuv420_width+127)>>7)<<7;
                        tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                        tiled_offset = tiled_offset<<11;
                    } else {
                        /* even2 fomula: x+x_block_num*y */
                        temp1 = ((yuv420_width+127)>>7)<<7;
                        tiled_offset = tiled_y_index*(temp1>>6);
                        tiled_offset = tiled_offset+tiled_x_index;
                        tiled_offset = tiled_offset<<11;
                    }
                }
                temp1 = i&0x1F;
                temp2 = j&0x3F;
                csc_deinterleave_memcpy(yuv420_u_dest+j/2+yuv420_width/2*(i), yuv420_v_dest+j/2+yuv420_width/2*(i), nv12t_uv_src+tiled_offset+temp2+64*(temp1), 4);
                csc_deinterleave_memcpy(yuv420_u_dest+j/2+yuv420_width/2*(i+1), yuv420_v_dest+j/2+yuv420_width/2*(i+1), nv12t_uv_src+tiled_offset+temp2+64*(temp1+1), 4);
            }
        }
    }
}

/*
 * Converts linear data to tiled.
 * 1. Y of YUV420P to Y of NV12T
 * 2. Y of YUV420S to Y of NV12T
 * 3. UV of YUV420S to UV of NV12T
 *
 * @param nv12t_dest
 *   Y or UV plane address of NV12T[out]
 *
 * @param yuv420_src
 *   Y or UV plane address of YUV420P(S)[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_height
 *   Y: Height of YUV420, UV: Height/2 of YUV420[in]
 */
void csc_linear_to_tiled(char *nv12t_dest, char *yuv420_src, int yuv420_width, int yuv420_height)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0, aligned_y_size = 0;
    unsigned int tiled_offset = 0;
    unsigned int temp1 = 0, temp2 = 0;

    aligned_y_size = (yuv420_height>>5)<<5;
    aligned_x_size = (yuv420_width>>6)<<6;

    for (i=0; i<aligned_y_size; i=i+32) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            memcpy(nv12t_dest+tiled_offset, yuv420_src+j+yuv420_width*(i), 64);
            memcpy(nv12t_dest+tiled_offset+64*1, yuv420_src+j+yuv420_width*(i+1), 64);
            memcpy(nv12t_dest+tiled_offset+64*2, yuv420_src+j+yuv420_width*(i+2), 64);
            memcpy(nv12t_dest+tiled_offset+64*3, yuv420_src+j+yuv420_width*(i+3), 64);
            memcpy(nv12t_dest+tiled_offset+64*4, yuv420_src+j+yuv420_width*(i+4), 64);
            memcpy(nv12t_dest+tiled_offset+64*5, yuv420_src+j+yuv420_width*(i+5), 64);
            memcpy(nv12t_dest+tiled_offset+64*6, yuv420_src+j+yuv420_width*(i+6), 64);
            memcpy(nv12t_dest+tiled_offset+64*7, yuv420_src+j+yuv420_width*(i+7), 64);
            memcpy(nv12t_dest+tiled_offset+64*8, yuv420_src+j+yuv420_width*(i+8), 64);
            memcpy(nv12t_dest+tiled_offset+64*9, yuv420_src+j+yuv420_width*(i+9), 64);
            memcpy(nv12t_dest+tiled_offset+64*10, yuv420_src+j+yuv420_width*(i+10), 64);
            memcpy(nv12t_dest+tiled_offset+64*11, yuv420_src+j+yuv420_width*(i+11), 64);
            memcpy(nv12t_dest+tiled_offset+64*12, yuv420_src+j+yuv420_width*(i+12), 64);
            memcpy(nv12t_dest+tiled_offset+64*13, yuv420_src+j+yuv420_width*(i+13), 64);
            memcpy(nv12t_dest+tiled_offset+64*14, yuv420_src+j+yuv420_width*(i+14), 64);
            memcpy(nv12t_dest+tiled_offset+64*15, yuv420_src+j+yuv420_width*(i+15), 64);
            memcpy(nv12t_dest+tiled_offset+64*16, yuv420_src+j+yuv420_width*(i+16), 64);
            memcpy(nv12t_dest+tiled_offset+64*17, yuv420_src+j+yuv420_width*(i+17), 64);
            memcpy(nv12t_dest+tiled_offset+64*18, yuv420_src+j+yuv420_width*(i+18), 64);
            memcpy(nv12t_dest+tiled_offset+64*19, yuv420_src+j+yuv420_width*(i+19), 64);
            memcpy(nv12t_dest+tiled_offset+64*20, yuv420_src+j+yuv420_width*(i+20), 64);
            memcpy(nv12t_dest+tiled_offset+64*21, yuv420_src+j+yuv420_width*(i+21), 64);
            memcpy(nv12t_dest+tiled_offset+64*22, yuv420_src+j+yuv420_width*(i+22), 64);
            memcpy(nv12t_dest+tiled_offset+64*23, yuv420_src+j+yuv420_width*(i+23), 64);
            memcpy(nv12t_dest+tiled_offset+64*24, yuv420_src+j+yuv420_width*(i+24), 64);
            memcpy(nv12t_dest+tiled_offset+64*25, yuv420_src+j+yuv420_width*(i+25), 64);
            memcpy(nv12t_dest+tiled_offset+64*26, yuv420_src+j+yuv420_width*(i+26), 64);
            memcpy(nv12t_dest+tiled_offset+64*27, yuv420_src+j+yuv420_width*(i+27), 64);
            memcpy(nv12t_dest+tiled_offset+64*28, yuv420_src+j+yuv420_width*(i+28), 64);
            memcpy(nv12t_dest+tiled_offset+64*29, yuv420_src+j+yuv420_width*(i+29), 64);
            memcpy(nv12t_dest+tiled_offset+64*30, yuv420_src+j+yuv420_width*(i+30), 64);
            memcpy(nv12t_dest+tiled_offset+64*31, yuv420_src+j+yuv420_width*(i+31), 64);
        }
    }

    for (i=aligned_y_size; i<yuv420_height; i=i+4) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            temp1 = i&0x1F;
            memcpy(nv12t_dest+tiled_offset+64*(temp1), yuv420_src+j+yuv420_width*(i), 64);
            memcpy(nv12t_dest+tiled_offset+64*(temp1+1), yuv420_src+j+yuv420_width*(i+1), 64);
            memcpy(nv12t_dest+tiled_offset+64*(temp1+2), yuv420_src+j+yuv420_width*(i+2), 64);
            memcpy(nv12t_dest+tiled_offset+64*(temp1+3), yuv420_src+j+yuv420_width*(i+3), 64);
        }
    }

    for (i=0; i<yuv420_height; i=i+4) {
        for (j=aligned_x_size; j<yuv420_width; j=j+4) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }

            temp1 = i&0x1F;
            temp2 = j&0x3F;
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1), yuv420_src+j+yuv420_width*(i), 4);
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1+1), yuv420_src+j+yuv420_width*(i+1), 4);
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1+2), yuv420_src+j+yuv420_width*(i+2), 4);
            memcpy(nv12t_dest+tiled_offset+temp2+64*(temp1+3), yuv420_src+j+yuv420_width*(i+3), 4);
        }
    }
}

/*
 * Converts and Interleaves linear to tiled
 * 1. UV of YUV420P to UV of NV12T
 *
 * @param nv12t_uv_dest
 *   UV plane address of NV12T[out]
 *
 * @param yuv420p_u_src
 *   U plane address of YUV420P[in]
 *
 * @param yuv420p_v_src
 *   V plane address of YUV420P[in]
 *
 * @param yuv420_width
 *   Width of YUV420[in]
 *
 * @param yuv420_uv_height
 *   Height/2 of YUV420[in]
 */
void csc_linear_to_tiled_interleave(char *nv12t_uv_dest, char *yuv420p_u_src, char *yuv420p_v_src, int yuv420_width, int yuv420_uv_height)
{
    unsigned int i, j;
    unsigned int tiled_x_index = 0, tiled_y_index = 0;
    unsigned int aligned_x_size = 0, aligned_y_size = 0;
    unsigned int tiled_offset = 0;
    unsigned int temp1 = 0, temp2 = 0;

    aligned_y_size = (yuv420_uv_height>>5)<<5;
    aligned_x_size = ((yuv420_width)>>6)<<6;

    for (i=0; i<aligned_y_size; i=i+32) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset, yuv420p_u_src+j/2+yuv420_width/2*(i), yuv420p_v_src+j/2+yuv420_width/2*(i), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*1, yuv420p_u_src+j/2+yuv420_width/2*(i+1), yuv420p_v_src+j/2+yuv420_width/2*(i+1), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*2, yuv420p_u_src+j/2+yuv420_width/2*(i+2), yuv420p_v_src+j/2+yuv420_width/2*(i+2), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*3, yuv420p_u_src+j/2+yuv420_width/2*(i+3), yuv420p_v_src+j/2+yuv420_width/2*(i+3), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*4, yuv420p_u_src+j/2+yuv420_width/2*(i+4), yuv420p_v_src+j/2+yuv420_width/2*(i+4), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*5, yuv420p_u_src+j/2+yuv420_width/2*(i+5), yuv420p_v_src+j/2+yuv420_width/2*(i+5), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*6, yuv420p_u_src+j/2+yuv420_width/2*(i+6), yuv420p_v_src+j/2+yuv420_width/2*(i+6), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*7, yuv420p_u_src+j/2+yuv420_width/2*(i+7), yuv420p_v_src+j/2+yuv420_width/2*(i+7), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*8, yuv420p_u_src+j/2+yuv420_width/2*(i+8), yuv420p_v_src+j/2+yuv420_width/2*(i+8), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*9, yuv420p_u_src+j/2+yuv420_width/2*(i+9), yuv420p_v_src+j/2+yuv420_width/2*(i+9), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*10, yuv420p_u_src+j/2+yuv420_width/2*(i+10), yuv420p_v_src+j/2+yuv420_width/2*(i+10), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*11, yuv420p_u_src+j/2+yuv420_width/2*(i+11), yuv420p_v_src+j/2+yuv420_width/2*(i+11), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*12, yuv420p_u_src+j/2+yuv420_width/2*(i+12), yuv420p_v_src+j/2+yuv420_width/2*(i+12), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*13, yuv420p_u_src+j/2+yuv420_width/2*(i+13), yuv420p_v_src+j/2+yuv420_width/2*(i+13), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*14, yuv420p_u_src+j/2+yuv420_width/2*(i+14), yuv420p_v_src+j/2+yuv420_width/2*(i+14), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*15, yuv420p_u_src+j/2+yuv420_width/2*(i+15), yuv420p_v_src+j/2+yuv420_width/2*(i+15), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*16, yuv420p_u_src+j/2+yuv420_width/2*(i+16), yuv420p_v_src+j/2+yuv420_width/2*(i+16), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*17, yuv420p_u_src+j/2+yuv420_width/2*(i+17), yuv420p_v_src+j/2+yuv420_width/2*(i+17), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*18, yuv420p_u_src+j/2+yuv420_width/2*(i+18), yuv420p_v_src+j/2+yuv420_width/2*(i+18), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*19, yuv420p_u_src+j/2+yuv420_width/2*(i+19), yuv420p_v_src+j/2+yuv420_width/2*(i+19), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*20, yuv420p_u_src+j/2+yuv420_width/2*(i+20), yuv420p_v_src+j/2+yuv420_width/2*(i+20), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*21, yuv420p_u_src+j/2+yuv420_width/2*(i+21), yuv420p_v_src+j/2+yuv420_width/2*(i+21), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*22, yuv420p_u_src+j/2+yuv420_width/2*(i+22), yuv420p_v_src+j/2+yuv420_width/2*(i+22), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*23, yuv420p_u_src+j/2+yuv420_width/2*(i+23), yuv420p_v_src+j/2+yuv420_width/2*(i+23), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*24, yuv420p_u_src+j/2+yuv420_width/2*(i+24), yuv420p_v_src+j/2+yuv420_width/2*(i+24), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*25, yuv420p_u_src+j/2+yuv420_width/2*(i+25), yuv420p_v_src+j/2+yuv420_width/2*(i+25), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*26, yuv420p_u_src+j/2+yuv420_width/2*(i+26), yuv420p_v_src+j/2+yuv420_width/2*(i+26), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*27, yuv420p_u_src+j/2+yuv420_width/2*(i+27), yuv420p_v_src+j/2+yuv420_width/2*(i+27), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*28, yuv420p_u_src+j/2+yuv420_width/2*(i+28), yuv420p_v_src+j/2+yuv420_width/2*(i+28), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*29, yuv420p_u_src+j/2+yuv420_width/2*(i+29), yuv420p_v_src+j/2+yuv420_width/2*(i+29), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*30, yuv420p_u_src+j/2+yuv420_width/2*(i+30), yuv420p_v_src+j/2+yuv420_width/2*(i+30), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*31, yuv420p_u_src+j/2+yuv420_width/2*(i+31), yuv420p_v_src+j/2+yuv420_width/2*(i+31), 32);
        }
    }

    for (i=aligned_y_size; i<yuv420_uv_height; i=i+4) {
        for (j=0; j<aligned_x_size; j=j+64) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            temp1 = i&0x1F;
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*(temp1), yuv420p_u_src+j/2+yuv420_width/2*(i), yuv420p_v_src+j/2+yuv420_width/2*(i), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*(temp1+1), yuv420p_u_src+j/2+yuv420_width/2*(i+1), yuv420p_v_src+j/2+yuv420_width/2*(i+1), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*(temp1+2), yuv420p_u_src+j/2+yuv420_width/2*(i+2), yuv420p_v_src+j/2+yuv420_width/2*(i+2), 32);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+64*(temp1+3), yuv420p_u_src+j/2+yuv420_width/2*(i+3), yuv420p_v_src+j/2+yuv420_width/2*(i+3), 32);
        }
    }

    for (i=0; i<yuv420_uv_height; i=i+4) {
        for (j=aligned_x_size; j<yuv420_width; j=j+4) {
            tiled_offset = 0;
            tiled_x_index = j>>6;
            tiled_y_index = i>>5;
            if (tiled_y_index & 0x1) {
                /* odd fomula: 2+x+(x>>2)<<2+x_block_num*(y-1) */
                tiled_offset = tiled_y_index-1;
                temp1 = ((yuv420_width+127)>>7)<<7;
                tiled_offset = tiled_offset*(temp1>>6);
                tiled_offset = tiled_offset+tiled_x_index;
                tiled_offset = tiled_offset+2;
                temp1 = (tiled_x_index>>2)<<2;
                tiled_offset = tiled_offset+temp1;
                tiled_offset = tiled_offset<<11;
            } else {
                temp2 = ((yuv420_uv_height+31)>>5)<<5;
                if ((i+32)<temp2) {
                    /* even1 fomula: x+((x+2)>>2)<<2+x_block_num*y */
                    temp1 = tiled_x_index+2;
                    temp1 = (temp1>>2)<<2;
                    tiled_offset = tiled_x_index+temp1;
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_offset+tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset<<11;
                } else {
                    /* even2 fomula: x+x_block_num*y */
                    temp1 = ((yuv420_width+127)>>7)<<7;
                    tiled_offset = tiled_y_index*(temp1>>6);
                    tiled_offset = tiled_offset+tiled_x_index;
                    tiled_offset = tiled_offset<<11;
                }
            }
            temp1 = i&0x1F;
            temp2 = j&0x3F;
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+temp2+64*(temp1), yuv420p_u_src+j/2+yuv420_width/2*(i), yuv420p_v_src+j/2+yuv420_width/2*(i), 2);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+temp2+64*(temp1+1), yuv420p_u_src+j/2+yuv420_width/2*(i+1), yuv420p_v_src+j/2+yuv420_width/2*(i+1), 2);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+temp2+64*(temp1+2), yuv420p_u_src+j/2+yuv420_width/2*(i+2), yuv420p_v_src+j/2+yuv420_width/2*(i+2), 2);
            csc_interleave_memcpy(nv12t_uv_dest+tiled_offset+temp2+64*(temp1+3), yuv420p_u_src+j/2+yuv420_width/2*(i+3), yuv420p_v_src+j/2+yuv420_width/2*(i+3), 2);
        }
    }
}

