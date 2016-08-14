/*
 * NEON Convolution
 * Copyright (C) 2012, 2013 Thomas Tsou <tom@tsou.cc>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

	.syntax unified
	.text
	.align 2
	.global neon_conv_real4
	.type neon_conv_real4, %function
neon_conv_real4:
	push            {r4, lr}
	vpush       {q4-q7}
	vld2.32     {q0-q1}, [r1]
	ldr              r4, =8
.neon_conv_loop4:
	vld2.32     {q2-q3}, [r0], r4
	vmul.f32         q4,   q2, q0
	vmul.f32         q5,   q3, q0
	vpadd.f32       d12,   d8, d9
	vpadd.f32       d13,  d10, d11
	vpadd.f32       d14,  d12, d13
	vst1.64       {d14}, [r2]!
	subs             r3,   r3, #1
	bne          .neon_conv_loop4
	vpop       {q4-q7}
	pop             {r4, pc}
	.size neon_conv_real4, .-neon_conv_real4
	.align 2
	.p2align 4,,15
	.global neon_conv_real8
	.type neon_conv_real8, %function
neon_conv_real8:
	push         {r4-r5, lr}
	vpush        {q4-q7}
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]
	add              r4, r0, #32
	ldr              r5, =8
.neon_conv_loop8:
	vld2.32     {q4-q5}, [r0], r5
	vld2.32     {q6-q7}, [r4], r5
	vmul.f32         q8,   q4, q0
	vmul.f32         q9,   q5, q0
	vmul.f32        q10,   q6, q2
	vmul.f32        q11,   q7, q2

	vadd.f32        q12,   q8, q10
	vadd.f32        q13,   q9, q11

	vpadd.f32       d22,  d24, d25
	vpadd.f32       d23,  d26, d27
	vpadd.f32       d24,  d22, d23
	vst1.64       {d24}, [r2]!
	subs             r3,   r3, #1
	bne          .neon_conv_loop8
	vpop       {q4-q7}
	pop          {r4-r5, pc}
	.size neon_conv_real8, .-neon_conv_real8
	.align 2
	.global neon_conv_real12
	.type neon_conv_real12, %function
neon_conv_real12:
	push         {r4-r6, lr}
	vpush       {q4-q7}
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]!
	vld2.32     {q4-q5}, [r1]!
	add              r4, r0, #32
	add              r5, r0, #64
	ldr              r6, =8
.neon_conv_loop12:
	vld2.32     {q6-q7}, [r0], r6
	vld2.32     {q8-q9}, [r4], r6
	vld2.32   {q10-q11}, [r5], r6
#ifdef HAVE_NEON_FMA
	vfma.f32         q1,   q6, q0
	vfma.f32         q3,   q7, q0
	vfma.f32         q1,   q8, q2
	vfma.f32         q3,   q9, q2
	vfma.f32         q1,  q10, q4
	vfma.f32         q3,  q11, q4
#else
	vmul.f32        q12,   q6, q0
	vmul.f32        q13,   q7, q0
	vmul.f32        q14,   q8, q2
	vmul.f32        q15,   q9, q2
	vmul.f32         q1,  q10, q4
	vmul.f32         q3,  q11, q4

	vadd.f32         q5,  q12, q14
	vadd.f32         q6,  q13, q15
	vadd.f32         q1,   q5, q1
	vadd.f32         q3,   q6, q3
#endif
	vpadd.f32        d2,   d2, d3
	vpadd.f32        d3,   d6, d7
	vpadd.f32        d6,   d2, d3
	vst1.64        {d6}, [r2]!
	subs             r3,   r3, #1
	bne          .neon_conv_loop12
	vpop         {q4-q7}
	pop          {r4-r6, pc}
	.size neon_conv_real12, .-neon_conv_real12
	.align 2
	.global neon_conv_real16
	.type neon_conv_real16, %function
neon_conv_real16:
	push         {r4-r7, lr}
	vpush        {q4-q7}
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]!
	vld2.32     {q4-q5}, [r1]!
	vld2.32     {q6-q7}, [r1]
	add              r4, r0, #32
	add              r5, r0, #64
	add              r6, r0, #96
	ldr              r7, =8
.neon_conv_loop16:
	vld2.32     {q8-q9}, [r0], r7
	vld2.32   {q10-q11}, [r4], r7
	vld2.32   {q12-q13}, [r5], r7
	vld2.32   {q14-q15}, [r6], r7
#ifdef HAVE_NEON_FMA
	vmul.f32         q1,   q8, q0
	vmul.f32         q3,   q9, q0
	vfma.f32         q1,  q10, q2
	vfma.f32         q3,  q11, q2
	vfma.f32         q1,  q12, q4
	vfma.f32         q3,  q13, q4
	vfma.f32         q1,  q14, q6
	vfma.f32         q3,  q15, q6
#else
	vmul.f32         q1,   q8, q0
	vmul.f32         q3,   q9, q0
	vmul.f32         q5,  q10, q2
	vmul.f32         q7,  q11, q2
	vmul.f32         q8,  q12, q4
	vmul.f32         q9,  q13, q4
	vmul.f32        q10,  q14, q6
	vmul.f32        q11,  q15, q6

	vadd.f32         q1,   q1, q5
	vadd.f32         q3,   q3, q7
	vadd.f32         q5,   q8, q10
	vadd.f32         q7,   q9, q11
	vadd.f32         q1,   q1, q5
	vadd.f32         q3,   q3, q7
#endif
	vpadd.f32        d2,   d2, d3
	vpadd.f32        d3,   d6, d7
	vpadd.f32        d6,   d2, d3
	vst1.64        {d6}, [r2]!
	subs             r3,   r3, #1
	bne          .neon_conv_loop16
	vpop       {q4-q7}
	pop          {r4-r7, pc}
	.size neon_conv_real16, .-neon_conv_real16
	.align 2
	.global neon_conv_real20
	.type neon_conv_real20, %function
neon_conv_real20:
	push         {r4-r8, lr}
	vpush        {q4-q7}
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r1]!
	vld2.32     {q4-q5}, [r1]!
	vld2.32     {q6-q7}, [r1]!
	vld2.32     {q8-q9}, [r1]
	add              r4, r0, #32
	add              r5, r0, #64
	add              r6, r0, #96
	add              r7, r0, #128
	ldr              r8, =8
.neon_conv_loop20:
	vld2.32   {q10-q11}, [r0], r8
	vld2.32   {q12-q13}, [r4], r8
	vld2.32   {q14-q15}, [r5], r8
#ifdef HAVE_NEON_FMA
	vmul.f32         q1,  q10, q0
	vfma.f32         q1,  q12, q2
	vfma.f32         q1,  q14, q4
	vmul.f32         q3,  q11, q0
	vfma.f32         q3,  q13, q2
	vfma.f32         q3,  q15, q4

	vld2.32   {q12-q13}, [r6], r8
	vld2.32   {q14-q15}, [r7], r8

	vfma.f32         q1,  q12, q6
	vfma.f32         q3,  q13, q6
	vfma.f32         q1,  q14, q8
	vfma.f32         q3,  q15, q8
#else
	vmul.f32         q1,  q10, q0
	vmul.f32         q3,  q12, q2
	vmul.f32         q5,  q14, q4
	vmul.f32         q7,  q11, q0
	vmul.f32         q9,  q13, q2
	vmul.f32        q10,  q15, q4
	vadd.f32         q1,   q1, q3
	vadd.f32         q3,   q7, q9
	vadd.f32         q9,   q1, q5
	vadd.f32        q10,   q3, q10

	vld2.32   {q12-q13}, [r6], r8
	vld2.32   {q14-q15}, [r7], r8

	vmul.f32         q1,  q12, q6
	vmul.f32         q3,  q13, q6
	vmul.f32         q5,  q14, q8
	vmul.f32         q7,  q15, q8
	vadd.f32        q12,   q1, q9
	vadd.f32        q14,   q3, q10
	vadd.f32         q1,  q12, q5
	vadd.f32         q3,  q14, q7
#endif
	vpadd.f32        d2,   d2, d3
	vpadd.f32        d3,   d6, d7
	vpadd.f32        d6,   d2, d3
	vst1.64        {d6}, [r2]!
	subs             r3,   r3, #1
	bne          .neon_conv_loop20
	vpop       {q4-q7}
	pop          {r4-r8, pc}
	.size neon_conv_real20, .-neon_conv_real20
	.align 2
	.global mac_cx_neon4
	.type mac_cx_neon4, %function
mac_cx_neon4:
	push           {r4, lr}
	ldr             r4, =32
	veor           q14, q14
	veor           q15, q15
.neon_conv_loop_mac4:
	vld2.32     {q0-q1}, [r0], r4
	vld2.32     {q2-q3}, [r1]!

	vmul.f32       q10,  q0, q2
	vmul.f32       q11,  q1, q3
	vmul.f32       q12,  q0, q3
	vmul.f32       q13,  q2, q1
	vsub.f32        q8, q10, q11
	vadd.f32        q9, q12, q13

	vadd.f32        q14, q8
	vadd.f32        q15, q9
	subs             r3, #1
	bne       .neon_conv_loop_mac4

	vld1.64          d0, [r2]
	vpadd.f32       d28, d28, d29
	vpadd.f32       d30, d30, d31
	vpadd.f32        d1, d28, d30
	vadd.f32         d1, d0
	vst1.64          d1, [r2]
        pop             {r4, pc}
	.size mac_cx_neon4, .-mac_cx_neon4
	.section .note.GNU-stack,"",%progbits
