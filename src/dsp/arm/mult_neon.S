/*
 * NEON complex multiplication 
 * Copyright (C) 2012,2013 Thomas Tsou <tom@tsou.cc>
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

	.syntax unified
	.text
	.align 2
	.global neon_cmplx_mul_4n
	.type neon_cmplx_mul_4n, %function
neon_cmplx_mul_4n:
	vpush       {q4-q7}
.loop_mul:
	vld2.32     {q0-q1}, [r1]!
	vld2.32     {q2-q3}, [r2]!
	vmul.f32         q4, q0, q2
	vmul.f32         q5, q1, q3
	vmul.f32         q6, q0, q3
	vmul.f32         q7, q2, q1
	vsub.f32         q8, q4, q5
	vadd.f32         q9, q6, q7
	vst2.32     {q8-q9}, [r0]!
	subs             r3, #1
	bne       .loop_mul
	vpop       {q4-q7}
	bx               lr
	.size neon_cmplx_mul_4n, .-neon_cmplx_mul_4n
	.section .note.GNU-stack,"",%progbits
