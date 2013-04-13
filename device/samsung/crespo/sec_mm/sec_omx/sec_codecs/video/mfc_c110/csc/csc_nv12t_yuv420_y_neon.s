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
 * @file    csc_nv12t_yuv420_y_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

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

    .arch armv7-a
    .text
    .global csc_tiled_to_linear
    .type   csc_tiled_to_linear, %function
csc_tiled_to_linear:
    .fnstart

    @r0         linear_dest
    @r1         tiled_src
    @r2         linear_x_size
    @r3         linear_y_size
    @r4         j
    @r5         i
    @r6         tiled_addr
    @r7         linear_addr
    @r8         aligned_x_size
    @r9         aligned_y_size
    @r10        temp1
    @r11        temp2
    @r12        temp3
    @r14        temp4

    stmfd       sp!, {r4-r12,r14}               @ backup registers

    mov         r8, #0
    cmp         r2, #1024
    blt         LINEAR_X_SIZE_512

LINEAR_X_SIZE_1024:

    mov         r5, #0
LINEAR_X_SIZE_1024_LOOP:
    mov         r6, #0                  @ tiled_offset = 0@
    mov         r4, r5, asr #5          @ tiled_y_index = i>>5@
    and         r10, r4, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_1024_LOOP_EVEN
LINEAR_X_SIZE_1024_LOOP_ODD:
    sub         r6, r4, #1              @ tiled_offset = tiled_y_index-1@
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r6, r6, r10
    mov         r4, #8
    mov         r4, r4, lsl #11
    sub         r4, r4, #32
    add         r6, r6, #2              @ tiled_offset = tiled_offset+2@
    mov         r6, r6, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r6, #2048
    add         r12, r6, #4096
    add         r14, r6, #6144
    b           LINEAR_X_SIZE_1024_LOOP_MEMCPY

LINEAR_X_SIZE_1024_LOOP_EVEN:
    add         r11, r3, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r6, r4, r10
    add         r12, r5, #32
    cmp         r12, r11
    mov         r6, r6, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r11, r6, #2048
    movlt       r4, #8
    addlt       r12, r6, #12288
    addlt       r14, r6, #14336
    movge       r4, #4
    addge       r12, r6, #4096
    addge       r14, r6, #6144
    mov         r4, r4, lsl #11
    sub         r4, r4, #32

LINEAR_X_SIZE_1024_LOOP_MEMCPY:
    and         r10, r5, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r1, r10

    add         r6, r6, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld1.8      {q2, q3}, [r6], r4
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld1.8      {q6, q7}, [r11], r4
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    mul         r7, r2, r5
    vld1.8      {q10, q11}, [r12], r4
    add         r7, r7, r0
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14], r4

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    pld         [r6]
    vst1.8      {q12, q13}, [r7]!
    pld         [r6, #32]
    vst1.8      {q14, q15}, [r7]!

    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    vld1.8      {q2, q3}, [r6], r4

    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld1.8      {q6, q7}, [r11], r4
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld1.8      {q10, q11}, [r12], r4
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14], r4

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    pld         [r6]
    vst1.8      {q12, q13}, [r7]!
    pld         [r6, #32]
    vst1.8      {q14, q15}, [r7]!

    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    vld1.8      {q2, q3}, [r6], r4
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld1.8      {q6, q7}, [r11], r4
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld1.8      {q10, q11}, [r12], r4
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14], r4

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    pld         [r6]
    vst1.8      {q12, q13}, [r7]!
    pld         [r6, #32]
    vst1.8      {q14, q15}, [r7]!

    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    vld1.8      {q2, q3}, [r6]
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld1.8      {q6, q7}, [r11]
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld1.8      {q10, q11}, [r12]
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14]

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    add         r5, #1
    vst1.8      {q12, q13}, [r7]!
    cmp         r5, r3
    vst1.8      {q14, q15}, [r7]!

    blt         LINEAR_X_SIZE_1024_LOOP

    mov         r8, #1024

LINEAR_X_SIZE_512:

    sub         r14, r2, r8
    cmp         r14, #512
    blt         LINEAR_X_SIZE_256

    mov         r5, #0
LINEAR_X_SIZE_512_LOOP:
    mov         r6, #0
    mov         r4, r5, asr #5          @ tiled_y_index = i>>5
    and         r10, r4, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_512_LOOP_EVEN

LINEAR_X_SIZE_512_LOOP_ODD:
    sub         r6, r4, #1
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r6, r6, r10
    mov         r4, #8
    mov         r4, r4, lsl #11
    sub         r4, r4, #32
    add         r6, r6, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    add         r6, r6, r10             @ tiled_offset = tiled_offset+temp1@
    mov         r6, r6, lsl #11
    add         r11, r6, #2048
    add         r12, r6, #4096
    add         r14, r6, #6144
    b           LINEAR_X_SIZE_512_LOOP_MEMCPY

LINEAR_X_SIZE_512_LOOP_EVEN:
    add         r11, r3, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r6, r4, r10
    add         r12, r5, #32
    cmp         r12, r11
    mov         r6, r6, lsl #11         @ tiled_offset = tiled_offset<<11@
    movlt       r4, #8
    movlt       r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    movge       r10, r8, asr #6         @ temp1 = aligned_x_size>>6@
    add         r6, r6, r10, lsl #11    @ tiled_offset = tiled_offset+(temp1<<11)@
    add         r11, r6, #2048
    addlt       r12, r6, #12288
    addlt       r14, r6, #14336
    movge       r4, #4
    addge       r12, r6, #4096
    addge       r14, r6, #6144
    mov         r4, r4, lsl #11
    sub         r4, r4, #32

LINEAR_X_SIZE_512_LOOP_MEMCPY:
    and         r10, r5, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r1, r10

    add         r6, r6, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld1.8      {q2, q3}, [r6], r4
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld1.8      {q6, q7}, [r11], r4
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    mul         r7, r2, r5
    vld1.8      {q10, q11}, [r12], r4
    add         r7, r7, r8
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14], r4

    add         r7, r7, r0
    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    pld         [r6]
    vst1.8      {q12, q13}, [r7]!
    pld         [r6, #32]
    vst1.8      {q14, q15}, [r7]!

    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    vld1.8      {q2, q3}, [r6], r4
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    vld1.8      {q6, q7}, [r11], r4
    pld         [r14]
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    vld1.8      {q10, q11}, [r12], r4
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14], r4

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    add         r5, #1
    vst1.8      {q12, q13}, [r7]!
    cmp         r5, r3
    vst1.8      {q14, q15}, [r7]!

    blt         LINEAR_X_SIZE_512_LOOP

    add         r8, r8, #512

LINEAR_X_SIZE_256:

    sub         r14, r2, r8
    cmp         r14, #256
    blt         LINEAR_X_SIZE_128

    mov         r5, #0
LINEAR_X_SIZE_256_LOOP:
    mov         r6, #0
    mov         r4, r5, asr #5          @ tiled_y_index = i>>5
    and         r10, r4, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_256_LOOP_EVEN

LINEAR_X_SIZE_256_LOOP_ODD:
    sub         r6, r4, #1
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r6, r6, r10
    add         r6, r6, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    add         r6, r6, r10             @ tiled_offset = tiled_offset+temp1@
    mov         r6, r6, lsl #11
    add         r11, r6, #2048
    add         r12, r6, #4096
    add         r14, r6, #6144
    b           LINEAR_X_SIZE_256_LOOP_MEMCPY

LINEAR_X_SIZE_256_LOOP_EVEN:
    add         r11, r3, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r6, r4, r10
    mov         r6, r6, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r12, r5, #32
    cmp         r12, r11
    movlt       r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    movge       r10, r8, asr #6         @ temp1 = aligned_x_size>>6@
    add         r6, r6, r10, lsl #11    @ tiled_offset = tiled_offset+(temp1<<11)@
    add         r11, r6, #2048
    addlt       r12, r6, #12288
    addlt       r14, r6, #14336
    addge       r12, r6, #4096
    addge       r14, r6, #6144

LINEAR_X_SIZE_256_LOOP_MEMCPY:
    and         r10, r5, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r1, r10

    add         r6, r6, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r11]
    vld1.8      {q0, q1}, [r6]!
    pld         [r11, #32]
    add         r12, r12, r10           @ tiled_addr2 = tiled_src+64*(temp1)
    vld1.8      {q2, q3}, [r6]
    pld         [r12]
    vld1.8      {q4, q5}, [r11]!
    pld         [r12, #32]
    add         r14, r14, r10           @ tiled_addr3 = tiled_src+64*(temp1)
    vld1.8      {q6, q7}, [r11]
    pld         [r14]
    mul         r7, r2, r5
    vld1.8      {q8, q9}, [r12]!
    pld         [r14, #32]
    add         r7, r7, r8
    vld1.8      {q10, q11}, [r12]
    add         r7, r7, r0
    vld1.8      {q12, q13}, [r14]!
    vld1.8      {q14, q15}, [r14]

    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7]!
    add         r5, #1
    vst1.8      {q12, q13}, [r7]!
    cmp         r5, r3
    vst1.8      {q14, q15}, [r7]!

    blt         LINEAR_X_SIZE_256_LOOP

    add         r8, r8, #256

LINEAR_X_SIZE_128:

    sub         r14, r2, r8
    cmp         r14, #128
    blt         LINEAR_X_SIZE_64

    mov         r5, #0
LINEAR_X_SIZE_128_LOOP:
    mov         r6, #0
    mov         r4, r5, asr #5          @ tiled_y_index = i>>5
    and         r10, r4, #0x1
    cmp         r10, #0x1
    bne         LINEAR_X_SIZE_128_LOOP_EVEN

LINEAR_X_SIZE_128_LOOP_ODD:
    sub         r6, r4, #1
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_offset*(temp1>>6)@
    mul         r6, r6, r10
    add         r6, r6, #2              @ tiled_offset = tiled_offset+2@
    mov         r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    add         r6, r6, r10             @ tiled_offset = tiled_offset+temp1@
    mov         r6, r6, lsl #11
    add         r11, r6, #2048
    b           LINEAR_X_SIZE_128_LOOP_MEMCPY

LINEAR_X_SIZE_128_LOOP_EVEN:
    add         r11, r3, #31            @ temp2 = ((linear_y_size+31)>>5)<<5@
    bic         r11, r11, #0x1F
    add         r10, r2, #127           @ temp1 = ((linear_x_size+127)>>7)<<7@
    bic         r10, #0x7F
    mov         r10, r10, asr #6        @ tiled_offset = tiled_y_index*(temp1>>6)@
    mul         r6, r4, r10
    mov         r6, r6, lsl #11         @ tiled_offset = tiled_offset<<11@
    add         r12, r5, #32
    cmp         r12, r11
    movlt       r10, r8, asr #5         @ temp1 = aligned_x_size>>5@
    movge       r10, r8, asr #6         @ temp1 = aligned_x_size>>6@
    add         r6, r6, r10, lsl #11    @ tiled_offset = tiled_offset+(temp1<<11)@
    add         r11, r6, #2048

LINEAR_X_SIZE_128_LOOP_MEMCPY:
    and         r10, r5, #0x1F
    mov         r10, r10, lsl #6
    add         r10, r1, r10

    add         r6, r6, r10             @ tiled_addr = tiled_src+64*(temp1)
    add         r11, r11, r10           @ tiled_addr1 = tiled_src+64*(temp1)
    pld         [r6, #64]
    vld1.8      {q0, q1}, [r6]!
    pld         [r6, #64]
    vld1.8      {q2, q3}, [r6]!
    mul         r7, r2, r5
    pld         [r11]
    vld1.8      {q4, q5}, [r6]!
    add         r7, r7, r8
    pld         [r11, #32]
    vld1.8      {q6, q7}, [r6]
    add         r7, r7, r0
    pld         [r11, #64]
    vld1.8      {q8, q9}, [r11]!
    pld         [r11, #64]
    vld1.8      {q10, q11}, [r11]!
    vld1.8      {q12, q13}, [r11]!
    vld1.8      {q14, q15}, [r11]

    sub         r9, r2, #96
    vst1.8      {q0, q1}, [r7]!
    vst1.8      {q2, q3}, [r7]!
    vst1.8      {q8, q9}, [r7]!
    vst1.8      {q10, q11}, [r7], r9
    vst1.8      {q4, q5}, [r7]!
    vst1.8      {q6, q7}, [r7]!
    add         r5, #2
    vst1.8      {q12, q13}, [r7]!
    cmp         r5, r3
    vst1.8      {q14, q15}, [r7]

    blt         LINEAR_X_SIZE_128_LOOP

    add         r8, r8, #128

LINEAR_X_SIZE_64:

    sub         r14, r2, r8
    cmp         r14, #64
    blt         LINEAR_X_SIZE_4

    mov         r5, #0
    mov         r4, r8

LINEAR_X_SIZE_64_LOOP:

    bl          GET_TILED_OFFSET

    add         r6, r1, r6              @ tiled_addr = tiled_src+tiled_addr
    and         r11, r5, #0x1F          @ temp2 = i&0x1F
    mov         r11, r11, lsl #6        @ temp2 = 64*temp2
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2

    pld         [r6, #64]
    vld1.8      {q0, q1}, [r6]!         @ store {tiled_addr}
    mul         r10, r2, r5             @ temp1 = linear_x_size*(i)
    pld         [r6, #64]
    vld1.8      {q2, q3}, [r6]!
    pld         [r6, #64]
    vld1.8      {q4, q5}, [r6]!         @ store {tiled_addr+64*1}
    pld         [r6, #64]
    vld1.8      {q6, q7}, [r6]!
    pld         [r6, #64]
    vld1.8      {q8, q9}, [r6]!         @ store {tiled_addr+64*2}
    pld         [r6, #64]
    vld1.8      {q10, q11}, [r6]!
    add         r7, r0, r4              @ linear_addr = linear_dest+j
    vld1.8      {q12, q13}, [r6]!       @ store {tiled_addr+64*3}
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    vld1.8      {q14, q15}, [r6]!
    sub         r10, r2, #32            @ temp1 = linear_x_size-32

    vst1.8      {q0, q1}, [r7]!         @ load {linear_src, 64}
    vst1.8      {q2, q3}, [r7], r10
    vst1.8      {q4, q5}, [r7]!         @ load {linear_src+linear_x_size*1, 64}
    vst1.8      {q6, q7}, [r7], r10
    vst1.8      {q8, q9}, [r7]!         @ load {linear_src+linear_x_size*2, 64}
    vst1.8      {q10, q11}, [r7], r10
    add         r5, #4
    vst1.8      {q12, q13}, [r7]!       @ load {linear_src+linear_x_size*3, 64}
    cmp         r5, r3
    vst1.8      {q14, q15}, [r7], r10

    blt         LINEAR_X_SIZE_64_LOOP

    add         r8, r8, #64

LINEAR_X_SIZE_4:
    cmp         r8, r2
    beq         RESTORE_REG

    mov         r5, #0                  @ i = 0
LINEAR_Y_SIZE_4_LOOP:

    mov         r4, r8                  @ j = aligned_x_size
LINEAR_X_SIZE_4_LOOP:

    bl          GET_TILED_OFFSET

    and         r10, r5, #0x1F           @ temp1 = i&0x1F
    and         r11, r4, #0x3F           @ temp2 = j&0x3F

    add         r6, r6, r1
    add         r6, r6, r11
    add         r6, r6, r10, lsl #6

    ldr         r10, [r6], #64
    add         r7, r0, r4
    ldr         r11, [r6], #64
    mul         r9, r2, r5
    ldr         r12, [r6], #64
    add         r7, r7, r9
    ldr         r14, [r6], #64

    str         r10, [r7], r2
    str         r11, [r7], r2
    str         r12, [r7], r2
    str         r14, [r7], r2

    add         r4, r4, #4              @ j = j+4
    cmp         r4, r2                  @ j<linear_x_size
    blt         LINEAR_X_SIZE_4_LOOP

    add         r5, r5, #4              @ i = i+4
    cmp         r5, r3                  @ i<linear_y_size
    blt         LINEAR_Y_SIZE_4_LOOP

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}               @ restore registers

GET_TILED_OFFSET:

    mov         r11, r5, asr #5                 @ temp2 = i>>5
    mov         r10, r4, asr #6                 @ temp1 = j>>6

    and         r12, r11, #0x1                  @ if (temp2 & 0x1)
    cmp         r12, #0x1
    bne         GET_TILED_OFFSET_EVEN_FORMULA_1

GET_TILED_OFFSET_ODD_FORMULA:
    sub         r6, r11, #1                     @ tiled_addr = temp2-1
    add         r12, r2, #127                   @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F                 @ temp3 = (temp3 >>7)<<7
    mov         r12, r12, asr #6                @ temp3 = temp3>>6
    mul         r6, r6, r12                     @ tiled_addr = tiled_addr*temp3
    add         r6, r6, r10                     @ tiled_addr = tiled_addr+temp1
    add         r6, r6, #2                      @ tiled_addr = tiled_addr+2
    bic         r12, r10, #0x3                  @ temp3 = (temp1>>2)<<2
    add         r6, r6, r12                     @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11                 @ tiled_addr = tiled_addr<<11
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_1:
    add         r12, r3, #31                    @ temp3 = linear_y_size+31
    bic         r12, r12, #0x1F                 @ temp3 = (temp3>>5)<<5
    sub         r12, r12, #32                   @ temp3 = temp3 - 32
    cmp         r5, r12                         @ if (i<(temp3-32)) {
    bge         GET_TILED_OFFSET_EVEN_FORMULA_2
    add         r12, r10, #2                    @ temp3 = temp1+2
    bic         r12, r12, #3                    @ temp3 = (temp3>>2)<<2
    add         r6, r10, r12                    @ tiled_addr = temp1+temp3
    add         r12, r2, #127                   @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F                 @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6                @ temp3 = temp3>>6
    mul         r11, r11, r12                   @ tiled_y_index = tiled_y_index*temp3
    add         r6, r6, r11                     @ tiled_addr = tiled_addr+tiled_y_index
    mov         r6, r6, lsl #11                 @
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_2:
    add         r12, r2, #127                   @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F                 @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6                @ temp3 = temp3>>6
    mul         r6, r11, r12                    @ tiled_addr = temp2*temp3
    add         r6, r6, r10                     @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11                 @ tiled_addr = tiled_addr<<11@

GET_TILED_OFFSET_RETURN:
    mov         pc, lr
    .fnend

