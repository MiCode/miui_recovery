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
 * @file    csc_yuv420_nv12t_uv_neon.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */

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

    .arch armv7-a
    .text
    .global csc_linear_to_tiled_interleave
    .type   csc_linear_to_tiled_interleave, %function
csc_linear_to_tiled_interleave:
    .fnstart

    @r0     tiled_dest
    @r1     linear_src_u
    @r2     linear_src_v
    @r3     linear_x_size
    @r4     linear_y_size
    @r5     j
    @r6     i
    @r7     tiled_addr
    @r8     linear_addr
    @r9     aligned_x_size
    @r10    aligned_y_size
    @r11    temp1
    @r12    temp2
    @r14    temp3

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    ldr         r4, [sp, #40]           @ load linear_y_size to r4

    bic         r10, r4, #0x1F          @ aligned_y_size = (linear_y_size>>5)<<5
    bic         r9, r3, #0x3F           @ aligned_x_size = (linear_x_size>>6)<<6

    mov         r6, #0                  @ i = 0
LOOP_ALIGNED_Y_SIZE:

    mov         r5, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE:

    bl          GET_TILED_OFFSET

    mov         r11, r3, asr #1         @ temp1 = linear_x_size/2
    mul         r11, r11, r6            @ temp1 = temp1*(i)
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    mov         r12, r3, asr #1         @ temp2 = linear_x_size/2
    sub         r12, r12, #16           @ temp2 = linear_x_size-16

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!
    vst2.8      {q14, q15}, [r7]!

    add         r5, r5, #64             @ j = j+64
    cmp         r5, r9                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE

    add         r6, r6, #32             @ i = i+32
    cmp         r6, r10                 @ i<aligned_y_size
    blt         LOOP_ALIGNED_Y_SIZE

    ldr         r4, [sp, #40]           @ load linear_y_size to r4
    cmp         r6, r4
    beq         LOOP_LINEAR_Y_SIZE_2_START

LOOP_LINEAR_Y_SIZE_1:

    mov         r5, #0                  @ j = 0
LOOP_ALIGNED_X_SIZE_1:

    bl          GET_TILED_OFFSET

    mov         r11, r3, asr #1         @ temp1 = linear_x_size/2
    mul         r11, r11, r6            @ temp1 = temp1*(i)
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    mov         r12, r3, asr #1         @ temp2 = linear_x_size/2
    sub         r12, r12, #16           @ temp2 = linear_x_size-16

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr
    and         r14, r6, #0x1F          @ temp3 = i&0x1F@
    mov         r14, r14, lsl #6        @ temp3 = temp3*64
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3

    pld         [r8, r3]
    vld1.8      {q0}, [r8]!
    vld1.8      {q2}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q1}, [r11]!
    vld1.8      {q3}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q4}, [r8]!
    vld1.8      {q6}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q5}, [r11]!
    vld1.8      {q7}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q8}, [r8]!
    vld1.8      {q10}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q9}, [r11]!
    vld1.8      {q11}, [r11], r12
    pld         [r8, r3]
    vld1.8      {q12}, [r8]!
    vld1.8      {q14}, [r8], r12
    pld         [r11, r3]
    vld1.8      {q13}, [r11]!
    vld1.8      {q15}, [r11], r12

    vst2.8      {q0, q1}, [r7]!         @ store {tiled_addr}
    vst2.8      {q2, q3}, [r7]!
    vst2.8      {q4, q5}, [r7]!         @ store {tiled_addr+64*1}
    vst2.8      {q6, q7}, [r7]!
    vst2.8      {q8, q9}, [r7]!         @ store {tiled_addr+64*2}
    vst2.8      {q10, q11}, [r7]!
    vst2.8      {q12, q13}, [r7]!       @ store {tiled_addr+64*3}
    vst2.8      {q14, q15}, [r7]!

    add         r5, r5, #64             @ j = j+64
    cmp         r5, r9                  @ j<aligned_x_size
    blt         LOOP_ALIGNED_X_SIZE_1

    add         r6, r6, #4              @ i = i+4
    cmp         r6, r4                  @ i<linear_y_size
    blt         LOOP_LINEAR_Y_SIZE_1

LOOP_LINEAR_Y_SIZE_2_START:
    cmp         r5, r3
    beq         RESTORE_REG

    mov         r6, #0                  @ i = 0
LOOP_LINEAR_Y_SIZE_2:

    mov         r5, r9                  @ j = aligned_x_size
LOOP_LINEAR_X_SIZE_2:

    bl          GET_TILED_OFFSET

    mov         r11, r3, asr #1         @ temp1 = linear_x_size/2
    mul         r11, r11, r6            @ temp1 = temp1*(i)
    add         r11, r11, r5, asr #1    @ temp1 = temp1+j/2
    mov         r12, r3, asr #1         @ temp2 = linear_x_size/2
    sub         r12, r12, #1            @ temp2 = linear_x_size-1

    add         r8, r1, r11             @ linear_addr = linear_src_u+temp1
    add         r11, r2, r11            @ temp1 = linear_src_v+temp1
    add         r7, r0, r7              @ tiled_addr = tiled_dest+tiled_addr
    and         r14, r6, #0x1F          @ temp3 = i&0x1F@
    mov         r14, r14, lsl #6        @ temp3 = temp3*64
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3
    and         r14, r5, #0x3F          @ temp3 = j&0x3F
    add         r7, r7, r14             @ tiled_addr = tiled_addr+temp3

    ldrb        r10, [r8], #1
    ldrb        r14, [r11], #1
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #2
    ldrb        r10, [r8], r12
    ldrb        r14, [r11], r12
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #62

    ldrb        r10, [r8], #1
    ldrb        r14, [r11], #1
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #2
    ldrb        r10, [r8], r12
    ldrb        r14, [r11], r12
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #62

    ldrb        r10, [r8], #1
    ldrb        r14, [r11], #1
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #2
    ldrb        r10, [r8], r12
    ldrb        r14, [r11], r12
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #62

    ldrb        r10, [r8], #1
    ldrb        r14, [r11], #1
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #2
    ldrb        r10, [r8], r12
    ldrb        r14, [r11], r12
    mov         r14, r14, lsl #8
    orr         r10, r10, r14
    strh        r10, [r7], #62

    add         r5, r5, #4              @ j = j+4
    cmp         r5, r3                  @ j<linear_x_size
    blt         LOOP_LINEAR_X_SIZE_2

    add         r6, r6, #4              @ i = i+4
    cmp         r6, r4                  @ i<linear_y_size
    blt         LOOP_LINEAR_Y_SIZE_2

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers

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

