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
 * @file    csc_nv12t_yuv420_uv_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

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

    .arch armv7-a
    .text
    .global csc_tiled_to_linear_deinterleave
    .type   csc_tiled_to_linear_deinterleave, %function
csc_tiled_to_linear_deinterleave:
    .fnstart

    @r0         linear_u_dest
    @r1         linear_v_dest
    @r2         tiled_uv_src
    @r3         linear_x_size
    @r4         linear_y_size
    @r5         j
    @r6         i
    @r7         tiled_addr
    @r8         linear_addr
    @r9         aligned_x_size
    @r10        temp1
    @r11        temp2
    @r12        temp3
    @r14        temp4

    stmfd       sp!, {r4-r12,r14}               @ backup registers

    ldr         r4, [sp, #40]           @ load linear_y_size to r4

    mov         r9, #0

LINEAR_X_SIZE_1024:
    cmp         r3, #1024
    blt         LINEAR_X_SIZE_512

    mov         r6, #0
LINEAR_X_SIZE_1024_LOOP:
    mov         r7, #0                  @ tiled_offset = 0@
    mov         r5, r6, asr #5          @ tiled_y_index = i>>5@
    and         r10, r5, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_1024_LOOP_EVEN
LINEAR_X_SIZE_1024_LOOP_ODD:
    sub         r7, r5, #1              @ tiled_offset = tiled_y_index-1@
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r7, r7, r10
    mov         r5, #8
    mov         r5, r5, lsl #11
    sub         r5, r5, #32
    add         r7, r7, #2              @ tiled_offset = tiled_offset+2@
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r7, #2048
    add         r12, r7, #4096
    add         r14, r7, #6144
    b           LINEAR_X_SIZE_1024_LOOP_MEMCPY

LINEAR_X_SIZE_1024_LOOP_EVEN:
    add         r11, r4, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r7, r5, r10
    add         r12, r6, #32
    cmp         r12, r11
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r7, #2048
    movlt       r5, #8
    addlt       r12, r7, #12288
    addlt       r14, r7, #14336
    movge       r5, #4
    addge       r12, r7, #2048
    addge       r14, r7, #2048
    mov         r5, r5, lsl #11
    sub         r5, r5, #32

LINEAR_X_SIZE_1024_LOOP_MEMCPY:
    and         r10, r6, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r2, r10

    add         r7, r7, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld2.8      {q2, q3}, [r7], r5
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld2.8      {q6, q7}, [r11], r5
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    mov         r10, r3, asr #1
    vld2.8      {q10, q11}, [r12], r5
    mul         r10, r10, r6
    vld2.8      {q12, q13}, [r14]!
    vld2.8      {q14, q15}, [r14], r5

    add         r8, r0, r10
    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    add         r10, r1, r10
    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    pld         [r7]
    vst1.8      {q13}, [r10]!
    pld         [r7, #32]
    vst1.8      {q15}, [r10]!

    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    vld2.8      {q2, q3}, [r7], r5
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld2.8      {q6, q7}, [r11], r5
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld2.8      {q10, q11}, [r12], r5
    vld2.8      {q12, q13}, [r14]!
    vld2.8      {q14, q15}, [r14], r5

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    pld         [r7]
    vst1.8      {q13}, [r10]!
    pld         [r7, #32]
    vst1.8      {q15}, [r10]!

    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    vld2.8      {q2, q3}, [r7], r5
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld2.8      {q6, q7}, [r11], r5
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld2.8      {q10, q11}, [r12], r5
    vld2.8      {q12, q13}, [r14]!
    vld2.8      {q14, q15}, [r14], r5

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    pld         [r7]
    vst1.8      {q13}, [r10]!
    pld         [r7, #32]
    vst1.8      {q15}, [r10]!

    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    vld2.8      {q2, q3}, [r7]
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld2.8      {q6, q7}, [r11]
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld2.8      {q10, q11}, [r12]
    vld2.8      {q12, q13}, [r14]!
    vld2.8      {q14, q15}, [r14]

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    add         r6, #1
    vst1.8      {q13}, [r10]!
    cmp         r6, r4
    vst1.8      {q15}, [r10]!

    blt         LINEAR_X_SIZE_1024_LOOP

    mov         r9, #1024

LINEAR_X_SIZE_512:
    sub         r10, r3, r9
    cmp         r10, #512
    blt         LINEAR_X_SIZE_256

    mov         r6, #0
LINEAR_X_SIZE_512_LOOP:
    mov         r7, #0                  @ tiled_offset = 0@
    mov         r5, r6, asr #5          @ tiled_y_index = i>>5@
    and         r10, r5, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_512_LOOP_EVEN
LINEAR_X_SIZE_512_LOOP_ODD:
    sub         r7, r5, #1              @ tiled_offset = tiled_y_index-1@
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r7, r7, r10
    mov         r5, #8
    mov         r5, r5, lsl #11
    add         r7, r7, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r9, asr #5
    add         r7, r7, r10
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r7, #2048
    add         r12, r7, #4096
    add         r14, r7, #6144
    sub         r5, r5, #32
    b           LINEAR_X_SIZE_512_LOOP_MEMCPY

LINEAR_X_SIZE_512_LOOP_EVEN:
    add         r11, r4, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r7, r5, r10
    add         r12, r6, #32
    cmp         r12, r11
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    movlt       r5, #8
    movlt       r10, r9, asr #5
    movge       r10, r9, asr #6
    add         r7, r7, r10, lsl #11
    add         r11, r7, #2048
    addlt       r12, r7, #12288
    addlt       r14, r7, #14336
    movge       r5, #4
    addge       r12, r7, #4096
    addge       r14, r7, #6144
    mov         r5, r5, lsl #11
    sub         r5, r5, #32

LINEAR_X_SIZE_512_LOOP_MEMCPY:
    and         r10, r6, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r2, r10

    add         r7, r7, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld2.8      {q2, q3}, [r7], r5
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld2.8      {q6, q7}, [r11], r5
    pld         [r14]
    mov         r10, r3, asr #1
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    mul         r10, r10, r6
    vld2.8      {q10, q11}, [r12], r5
    add         r8, r0, r10
    vld2.8      {q12, q13}, [r14]!
    add         r8, r8, r9, asr #1
    vld2.8      {q14, q15}, [r14], r5

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    add         r10, r1, r10
    vst1.8      {q14}, [r8]!

    add         r10, r10, r9, asr #1
    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    pld         [r7]
    vst1.8      {q13}, [r10]!
    pld         [r7, #32]
    vst1.8      {q15}, [r10]!

    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    vld2.8      {q2, q3}, [r7]
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld2.8      {q6, q7}, [r11]
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld2.8      {q10, q11}, [r12]
    vld2.8      {q12, q13}, [r14]!
    vld2.8      {q14, q15}, [r14]

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    add         r6, #1
    vst1.8      {q13}, [r10]!
    cmp         r6, r4
    vst1.8      {q15}, [r10]!

    blt         LINEAR_X_SIZE_512_LOOP

    add         r9, r9, #512

LINEAR_X_SIZE_256:
    sub         r10, r3, r9
    cmp         r10, #256
    blt         LINEAR_X_SIZE_128

    mov         r6, #0
LINEAR_X_SIZE_256_LOOP:
    mov         r7, #0                  @ tiled_offset = 0@
    mov         r5, r6, asr #5          @ tiled_y_index = i>>5@
    and         r10, r5, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_256_LOOP_EVEN
LINEAR_X_SIZE_256_LOOP_ODD:
    sub         r7, r5, #1              @ tiled_offset = tiled_y_index-1@
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r7, r7, r10
    add         r7, r7, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r9, asr #5
    add         r7, r7, r10
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r7, #2048
    add         r12, r7, #4096
    add         r14, r7, #6144
    b           LINEAR_X_SIZE_256_LOOP_MEMCPY

LINEAR_X_SIZE_256_LOOP_EVEN:
    add         r11, r4, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r7, r5, r10
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r12, r6, #32
    cmp         r12, r11
    movlt       r10, r9, asr #5
    addlt       r7, r7, r10, lsl #11
    addlt       r11, r7, #2048
    addlt       r12, r7, #12288
    addlt       r14, r7, #14336
    movge       r10, r9, asr #6
    addge       r7, r7, r10, lsl #11
    addge       r11, r7, #2048
    addge       r12, r7, #4096
    addge       r14, r7, #6144

LINEAR_X_SIZE_256_LOOP_MEMCPY:
    and         r10, r6, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r2, r10

    add         r7, r7, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld2.8      {q2, q3}, [r7]
    pld         [r12]
    vld2.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld2.8      {q6, q7}, [r11]
    pld         [r14]
    vld2.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    mov         r10, r3, asr #1
    vld2.8      {q10, q11}, [r12]
    mul         r10, r10, r6
    vld2.8      {q12, q13}, [r14]!
    add         r8, r0, r10
    vld2.8      {q14, q15}, [r14]

    add         r8, r8, r9, asr #1
    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8]!
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    add         r10, r1, r10
    vst1.8      {q14}, [r8]!

    add         r10, r10, r9, asr #1
    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10]!
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    add         r6, #1
    vst1.8      {q13}, [r10]!
    cmp         r6, r4
    vst1.8      {q15}, [r10]!
    blt         LINEAR_X_SIZE_256_LOOP

    add         r9, r9, #256

LINEAR_X_SIZE_128:
    sub         r10, r3, r9
    cmp         r10, #128
    blt         LINEAR_X_SIZE_64

    mov         r6, #0
LINEAR_X_SIZE_128_LOOP:
    mov         r7, #0                  @ tiled_offset = 0@
    mov         r5, r6, asr #5          @ tiled_y_index = i>>5@
    and         r10, r5, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_128_LOOP_EVEN
LINEAR_X_SIZE_128_LOOP_ODD:
    sub         r7, r5, #1              @ tiled_offset = tiled_y_index-1@
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r7, r7, r10
    add         r7, r7, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r9, asr #5
    add         r7, r7, r10
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r7, #2048
    b           LINEAR_X_SIZE_128_LOOP_MEMCPY

LINEAR_X_SIZE_128_LOOP_EVEN:
    add         r11, r4, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r3, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r7, r5, r10
    mov         r7, r7, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r12, r6, #32
    cmp         r12, r11
    movlt       r10, r9, asr #5
    movge       r10, r9, asr #6
    add         r7, r7, r10, lsl #11
    add         r11, r7, #2048

LINEAR_X_SIZE_128_LOOP_MEMCPY:
    and         r10, r6, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r2, r10

    add         r7, r7, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld2.8      {q0, q1}, [r7]!
    pld         [r11, #32]
    vld2.8      {q2, q3}, [r7]!
    pld         [r7]
    vld2.8      {q4, q5}, [r11]!
    mov         r10, r3, asr #1
    pld         [r7, #32]
    vld2.8      {q6, q7}, [r11]!
    mul         r10, r10, r6
    pld         [r11]
    vld2.8      {q8, q9}, [r7]!
    add         r10, r10, r9, asr #1
    pld         [r11, #32]
    vld2.8      {q10, q11}, [r7]!
    add         r8, r0, r10
    vld2.8      {q12, q13}, [r11]!
    mov         r14, r3, asr #1
    vld2.8      {q14, q15}, [r11]!

    sub         r14, r14, #48
    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8]!
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8], r14
    vst1.8      {q8}, [r8]!
    vst1.8      {q10}, [r8]!
    vst1.8      {q12}, [r8]!
    vst1.8      {q14}, [r8]!

    add         r10, r1, r10
    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10]!
    vst1.8      {q5}, [r10]!
    vst1.8      {q7}, [r10], r14
    vst1.8      {q9}, [r10]!
    vst1.8      {q11}, [r10]!
    add         r6, #2
    vst1.8      {q13}, [r10]!
    cmp         r6, r4
    vst1.8      {q15}, [r10]!

    blt         LINEAR_X_SIZE_128_LOOP

    add         r9, r9, #128

LINEAR_X_SIZE_64:
    sub         r10, r3, r9
    cmp         r10, #64
    blt         LINEAR_X_SIZE_4

    mov         r5, r9
    mov         r6, #0

LINEAR_X_SIZE_64_LOOP:
    bl          GET_TILED_OFFSET

LINEAR_X_SIZE_64_LOOP_MEMCPY:
    and         r10, r6, #0x1F
    mov         r14, r3, asr #1
    mov         r10, r10, lsl #6
    sub         r14, r14, #16
    add         r10, r2, r10

    add         r7, r7, r10             @ tiled_addr = tiled_src+64*(temp1)
    pld         [r7, #64]
    vld2.8      {q0, q1}, [r7]!
    mov         r10, r3, asr #1
    pld         [r7, #64]
    vld2.8      {q2, q3}, [r7]!
    mul         r10, r10, r6
    vld2.8      {q4, q5}, [r7]!
    add         r10, r10, r9, asr #1
    vld2.8      {q6, q7}, [r7]!
    add         r8, r0, r10

    vst1.8      {q0}, [r8]!
    vst1.8      {q2}, [r8], r14
    vst1.8      {q4}, [r8]!
    vst1.8      {q6}, [r8], r14

    add         r10, r1, r10
    vst1.8      {q1}, [r10]!
    vst1.8      {q3}, [r10], r14
    add         r6, #2
    vst1.8      {q5}, [r10]!
    cmp         r6, r4
    vst1.8      {q7}, [r10], r14

    blt         LINEAR_X_SIZE_64_LOOP

    add         r9, r9, #64

LINEAR_X_SIZE_4:
    cmp         r9, r3
    beq         RESTORE_REG

    mov         r6, #0                  @ i = 0
LINEAR_Y_SIZE_4_LOOP:

    mov         r5, r9                  @ j = aligned_x_size
LINEAR_X_SIZE_4_LOOP:

    bl          GET_TILED_OFFSET

    mov         r11, r3, asr #1         @ temp1 = linear_x_size/2
    mul         r11, r11, r6            @ temp1 = temp1*(i)
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    mov         r12, r3, asr #1         @ temp2 = linear_x_size/2
    sub         r12, r12, #1            @ temp2 = linear_x_size-1

    add         r8, r0, r11             @ linear_addr = linear_dest_u+temp1
    add         r11, r1, r11            @ temp1 = linear_dest_v+temp1
    add         r7, r2, r7              @ tiled_addr = tiled_src+tiled_addr
    and         r14, r6, #0x1F          @ temp3 = i&0x1F@
    mov         r14, r14, lsl #6        @ temp3 = temp3*64
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3
    and         r14, r5, #0x3F          @ temp3 = j&0x3F
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3

    ldrh        r10, [r7], #2
    ldrh        r14, [r7], #62
    strb        r10, [r8], #1
    mov         r10, r10, asr #8
    strb        r10, [r11], #1
    strb        r14, [r8], r12
    mov         r14, r14, asr #8
    strb        r14, [r11], r12

    ldrh        r10, [r7], #2
    ldrh        r14, [r7], #62
    strb        r10, [r8], #1
    mov         r10, r10, asr #8
    strb        r10, [r11], #1
    strb        r14, [r8], r12
    mov         r14, r14, asr #8
    strb        r14, [r11], r12

    add         r5, r5, #4              @ j = j+4
    cmp         r5, r3                  @ j<linear_x_size
    blt         LINEAR_X_SIZE_4_LOOP

    add         r6, r6, #2              @ i = i+4
    cmp         r6, r4                  @ i<linear_y_size
    blt         LINEAR_Y_SIZE_4_LOOP

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}               @ restore registers

GET_TILED_OFFSET:
    stmfd       sp!, {r14}

    mov         r12, r6, asr #5         @ temp2 = i>>5
    mov         r11, r5, asr #6         @ temp1 = j>>6

    and         r14, r12, #0x1          @ if (temp2 & 0x1)
    cmp         r14, #0x1
    bne         GET_TILED_OFFSET_EVEN_FORMULA_1

GET_TILED_OFFSET_ODD_FORMULA:
    sub         r7, r12, #1             @ tiled_addr = temp2-1
    add         r14, r3, #127           @ temp3 = linear_x_size+127
    bic         r14, r14, #0x7F         @ temp3 = (temp3 >>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    mul         r7, r7, r14             @ tiled_addr = tiled_addr*temp3
    add         r7, r7, r11             @ tiled_addr = tiled_addr+temp1
    add         r7, r7, #2              @ tiled_addr = tiled_addr+2
    bic         r14, r11, #0x3          @ temp3 = (temp1>>2)<<2
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3
    mov         r7, r7, lsl #11         @ tiled_addr = tiled_addr<<11
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_1:
    add         r14, r4, #31            @ temp3 = linear_y_size+31
    bic         r14, r14, #0x1F         @ temp3 = (temp3>>5)<<5
    sub         r14, r14, #32           @ temp3 = temp3 - 32
    cmp         r6, r14                 @ if (i<(temp3-32)) {
    bge         GET_TILED_OFFSET_EVEN_FORMULA_2
    add         r14, r11, #2            @ temp3 = temp1+2
    bic         r14, r14, #3            @ temp3 = (temp3>>2)<<2
    add         r7, r11, r14            @ tiled_addr = temp1+temp3
    add         r14, r3, #127           @ temp3 = linear_x_size+127
    bic         r14, r14, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    mul         r12, r12, r14           @ tiled_y_index = tiled_y_index*temp3
    add         r7, r7, r12             @ tiled_addr = tiled_addr+tiled_y_index
    mov         r7, r7, lsl #11         @
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_2:
    add         r14, r3, #127           @ temp3 = linear_x_size+127
    bic         r14, r14, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r14, r14, asr #6        @ temp3 = temp3>>6
    mul         r7, r12, r14            @ tiled_addr = temp2*temp3
    add         r7, r7, r11             @ tiled_addr = tiled_addr+temp3
    mov         r7, r7, lsl #11         @ tiled_addr = tiled_addr<<11@

GET_TILED_OFFSET_RETURN:
    ldmfd       sp!, {r15}              @ restore registers
    .fnend
