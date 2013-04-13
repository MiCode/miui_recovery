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
 * @file    color_space_convertor.h
 * @brief   SEC_OMX specific define.
 *   NV12T(tiled) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   uv pixel block is interleaved as u v u v u v ...
 *   y1    y2    y7    y8    y9    y10   y15   y16
 *   y3    y4    y5    y6    y11   y12   y13   y14
 *   y17   y18   y23   y24   y25   y26   y31   y32
 *   y19   y20   y21   y22   y27   y28   y29   y30
 *   uv1   uv2   uv7   uv8   uv9   uv10  uv15  uv16
 *   uv3   uv4   uv5   uv6   uv11  uv12  uv13  uv14
 *   YUV420Planar(linear) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   y1    y2    y3    y4    y5    y6    y7    y8
 *   y9    y10   y11   y12   y13   y14   y15   y16
 *   y17   y18   y19   y20   y21   y22   y23   y24
 *   y25   y26   y27   y28   y29   y30   y31   y32
 *   u1    u2    u3    u4    u5    u6    u7    u8
 *   v1    v2    v3    v4    v5    v6    v7    v8
 *   YUV420Semiplanar(linear) layout:
 *   Each element is not pixel. It is 64x32 pixel block.
 *   uv pixel block is interleaved as u v u v u v ...
 *   y1    y2    y3    y4    y5    y6    y7    y8
 *   y9    y10   y11   y12   y13   y14   y15   y16
 *   y17   y18   y19   y20   y21   y22   y23   y24
 *   y25   y26   y27   y28   y29   y30   y31   y32
 *   uv1   uv2   uv3   uv4   uv5   uv6   uv7   uv8
 *   uv9   uv10  uv11  uv12  uv13  uv14  uv15  uv16
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

#ifndef COLOR_SPACE_CONVERTOR_H_
#define COLOR_SPACE_CONVERTOR_H_

/*--------------------------------------------------------------------------------*/
/* Format Conversion API                                                          */
/*--------------------------------------------------------------------------------*/
/* C Code */
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
void csc_deinterleave_memcpy(char *dest1, char *dest2, char *src, int src_size);

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
void csc_interleave_memcpy(char *dest, char *src1, char *src2, int src_size);

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
void csc_tiled_to_linear(char *yuv420p_y_dest, char *nv12t_y_src, int yuv420p_width, int yuv420p_y_height);

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
void csc_tiled_to_linear_deinterleave(char *yuv420p_u_dest, char *yuv420p_v_dest, char *nv12t_uv_src, int yuv420p_width, int yuv420p_uv_height);

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
void csc_linear_to_tiled(char *nv12t_dest, char *yuv420p_src, int yuv420p_width, int yuv420p_y_height);

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
void csc_linear_to_tiled_interleave(char *nv12t_uv_dest, char *yuv420p_u_src, char *yuv420p_v_src, int yuv420p_width, int yuv420p_uv_height);

#endif /*COLOR_SPACE_CONVERTOR_H_*/
