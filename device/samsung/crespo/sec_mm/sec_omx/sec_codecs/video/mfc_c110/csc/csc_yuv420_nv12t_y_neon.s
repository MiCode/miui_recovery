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
 * @file    csc_yuv420_nv12t_y_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

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

    .arch armv7-a
    .text
    .global csc_linear_to_tiled
    .type   csc_linear_to_tiled, %function
csc_linear_to_tiled:
    .fnstart

    @r0     tiled_dest
    @r1     linear_src
    @r2     linear_x_size
    @r3     linear_y_size
    @r4     j
    @r5     i
    @r6     nn(tiled_addr)
    @r7     mm(linear_addr)
    @r8     aligned_x_size
    @r9     aligned_y_size
    @r10    temp1
    @r11    temp2
    @r12    temp3
    @r14    temp4

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    bic         r9, r3, #0x1F           @ aligned_y_size = (linear_y_size>>5)<<5
    bic         r8, r2, #0x3F           @ aligned_x_size = (linear_x_size>>6)<<6

    mov         r5, #0                  @ i = 0
LOOP_ALIGNED_Y_SIZE:

    mov         r4, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE:

    bl          GET_TILED_OFFSET

    mul         r10, r2, r5             @ temp1 = linear_x_size*(i)
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    sub         r10, r2, #32

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    add         r4, r4, #64             @ j = j+64
    cmp         r4, r8                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE

    add         r5, r5, #32             @ i = i+32
    cmp         r5, r9                  @ i<aligned_y_size
    blt         LOOP_ALIGNED_Y_SIZE

    cmp         r5, r3
    beq         LOOP_LINEAR_Y_SIZE_2_START

LOOP_LINEAR_Y_SIZE_1:

    mov         r4, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE_1:

    bl          GET_TILED_OFFSET

    mul         r10, r2, r5             @ temp1 = linear_x_size*(i)
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1
    sub         r10, r2, #32            @ temp1 = linear_x_size-32

    pld         [r7, r2, lsl #1]
    vld1.8      {q0, q1}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q2, q3}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q4, q5}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q6, q7}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q8, q9}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q10, q11}, [r7], r10
    pld         [r7, r2, lsl #1]
    vld1.8      {q12, q13}, [r7]!
    pld         [r7, r2, lsl #1]
    vld1.8      {q14, q15}, [r7], r10

    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr
    and         r11, r5, #0x1F          @ temp2 = i&0x1F
    mov         r11, r11, lsl #6        @ temp2 = 64*temp2
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2

    vst1.8      {q0, q1}, [r6]!
    vst1.8      {q2, q3}, [r6]!
    vst1.8      {q4, q5}, [r6]!
    vst1.8      {q6, q7}, [r6]!
    vst1.8      {q8, q9}, [r6]!
    vst1.8      {q10, q11}, [r6]!
    vst1.8      {q12, q13}, [r6]!
    vst1.8      {q14, q15}, [r6]!

    add         r4, r4, #64             @ j = j+64
    cmp         r4, r8                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE_1

    add         r5, r5, #4              @ i = i+4
    cmp         r5, r3                  @ i<linear_y_size
    blt         LOOP_LINEAR_Y_SIZE_1

LOOP_LINEAR_Y_SIZE_2_START:
    cmp         r4, r2
    beq         RESTORE_REG

    mov         r5, #0                  @ i = 0
LOOP_LINEAR_Y_SIZE_2:

    mov         r4, r8                  @ j = aligned_x_size
LOOP_LINEAR_X_SIZE_2:

    bl          GET_TILED_OFFSET

    mul         r10, r2, r5             @ temp1 = linear_x_size*(i)
    add         r7, r1, r4              @ linear_addr = linear_src+j
    add         r7, r7, r10             @ linear_addr = linear_addr+temp1

    add         r6, r0, r6              @ tiled_addr = tiled_dest+tiled_addr
    and         r11, r5, #0x1F          @ temp2 = i&0x1F
    mov         r11, r11, lsl #6        @ temp2 = 64*temp2
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2
    and         r11, r4, #0x3F          @ temp2 = j&0x3F
    add         r6, r6, r11             @ tiled_addr = tiled_addr+temp2

    ldr         r10, [r7], r2
    ldr         r11, [r7], r2
    ldr         r12, [r7], r2
    ldr         r14, [r7], r2
    str         r10, [r6], #64
    str         r11, [r6], #64
    str         r12, [r6], #64
    str         r14, [r6], #64

    add         r4, r4, #4              @ j = j+4
    cmp         r4, r2                  @ j<linear_x_size
    blt         LOOP_LINEAR_X_SIZE_2

    add         r5, r5, #4              @ i = i+4
    cmp         r5, r3                  @ i<linear_y_size
    blt         LOOP_LINEAR_Y_SIZE_2

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

GET_TILED_OFFSET:

    mov         r11, r5, asr #5         @ temp2 = i>>5
    mov         r10, r4, asr #6         @ temp1 = j>>6

    and         r12, r11, #0x1          @ if (temp2 & 0x1)
    cmp         r12, #0x1
    bne         GET_TILED_OFFSET_EVEN_FORMULA_1

GET_TILED_OFFSET_ODD_FORMULA:
    sub         r6, r11, #1             @ tiled_addr = temp2-1
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F         @ temp3 = (temp3 >>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r6, r6, r12             @ tiled_addr = tiled_addr*temp3
    add         r6, r6, r10             @ tiled_addr = tiled_addr+temp1
    add         r6, r6, #2              @ tiled_addr = tiled_addr+2
    bic         r12, r10, #0x3          @ temp3 = (temp1>>2)<<2
    add         r6, r6, r12             @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11         @ tiled_addr = tiled_addr<<11
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_1:
    add         r12, r3, #31            @ temp3 = linear_y_size+31
    bic         r12, r12, #0x1F         @ temp3 = (temp3>>5)<<5
    sub         r12, r12, #32           @ temp3 = temp3 - 32
    cmp         r5, r12                 @ if (i<(temp3-32)) {
    bge         GET_TILED_OFFSET_EVEN_FORMULA_2
    add         r12, r10, #2            @ temp3 = temp1+2
    bic         r12, r12, #3            @ temp3 = (temp3>>2)<<2
    add         r6, r10, r12            @ tiled_addr = temp1+temp3
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r11, r11, r12           @ tiled_y_index = tiled_y_index*temp3
    add         r6, r6, r11             @ tiled_addr = tiled_addr+tiled_y_index
    mov         r6, r6, lsl #11         @
    b           GET_TILED_OFFSET_RETURN

GET_TILED_OFFSET_EVEN_FORMULA_2:
    add         r12, r2, #127           @ temp3 = linear_x_size+127
    bic         r12, r12, #0x7F         @ temp3 = (temp3>>7)<<7
    mov         r12, r12, asr #6        @ temp3 = temp3>>6
    mul         r6, r11, r12            @ tiled_addr = temp2*temp3
    add         r6, r6, r10             @ tiled_addr = tiled_addr+temp3
    mov         r6, r6, lsl #11         @ tiled_addr = tiled_addr<<11@

GET_TILED_OFFSET_RETURN:
    mov         pc, lr
    .fnend

