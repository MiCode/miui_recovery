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
 * @file    csc_interleave_memcpy.s
 * @brief   SEC_OMX specific define
 * @author  ShinWon Lee (shinwon.lee@samsung.com)
 * @version 1.0
 * @history
 *   2011.7.01 : Create
 */
    .arch armv7-a
    .text
    .global csc_interleave_memcpy
    .type   csc_interleave_memcpy, %function
csc_interleave_memcpy:
    .fnstart

    @r0     dest
    @r1     src1
    @r2     src2
    @r3     src_size
    @r4     i
    @r5     temp1
    @r6     temp2
    @r7     temp3
    @r8     temp2
    @r9     temp3

    stmfd       sp!, {r4-r12,r14}       @ backup registers

    mov         r4, #0
    cmp         r3, #128
    blt         LINEAR_SIZE_64

    bic         r5, r3, #0x2F
LINEAR_SIZE_128_LOOP:
    pld         [r1, #64]
    vld1.8      {q0}, [r1]!
    vld1.8      {q2}, [r1]!
    vld1.8      {q4}, [r1]!
    vld1.8      {q6}, [r1]!
    pld         [r2]
    vld1.8      {q8}, [r1]!
    vld1.8      {q10}, [r1]!
    vld1.8      {q12}, [r1]!
    vld1.8      {q14}, [r1]!
    pld         [r2, #64]
    vld1.8      {q1}, [r2]!
    vld1.8      {q3}, [r2]!
    vld1.8      {q5}, [r2]!
    vld1.8      {q7}, [r2]!
    vld1.8      {q9}, [r2]!
    vld1.8      {q11}, [r2]!
    vld1.8      {q13}, [r2]!
    vld1.8      {q15}, [r2]!

    vst2.8      {q0, q1}, [r0]!
    vst2.8      {q2, q3}, [r0]!
    vst2.8      {q4, q5}, [r0]!
    vst2.8      {q6, q7}, [r0]!
    vst2.8      {q8, q9}, [r0]!
    vst2.8      {q10, q11}, [r0]!
    pld         [r1]
    vst2.8      {q12, q13}, [r0]!
    vst2.8      {q14, q15}, [r0]!

    add         r4, #128
    cmp         r4, r5
    blt         LINEAR_SIZE_128_LOOP

LINEAR_SIZE_64:
    sub         r5, r3, r4
    cmp         r5, #64
    blt         LINEAR_SIZE_2
LINEAR_SIZE_64_LOOP:
    pld         [r2]
    vld1.8      {q0}, [r1]!
    vld1.8      {q2}, [r1]!
    vld1.8      {q4}, [r1]!
    vld1.8      {q6}, [r1]!
    vld1.8      {q1}, [r2]!
    vld1.8      {q3}, [r2]!
    vld1.8      {q5}, [r2]!
    vld1.8      {q7}, [r2]!

    vst2.8      {q0, q1}, [r0]!
    vst2.8      {q2, q3}, [r0]!
    pld         [r1]
    vst2.8      {q4, q5}, [r0]!
    vst2.8      {q6, q7}, [r0]!

    add         r4, #64
    cmp         r4, r3
    blt         LINEAR_SIZE_64_LOOP

LINEAR_SIZE_2:
    sub         r5, r3, r4
    cmp         r5, #2
    blt         RESTORE_REG
LINEAR_SIZE_2_LOOP:
    ldrb        r6, [r1], #1
    ldrb        r7, [r2], #1
    ldrb        r8, [r1], #1
    ldrb        r9, [r2], #1

    strb        r6, [r0], #1
    strb        r7, [r0], #1
    strb        r8, [r0], #1
    strb        r9, [r0], #1

    add         r4, #2
    cmp         r4, r3
    blt         LINEAR_SIZE_2_LOOP

RESTORE_REG:
    ldmfd       sp!, {r4-r12,r15}       @ restore registers
    .fnend

