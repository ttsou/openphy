/*
 * ARM NEON Scaling
 * Copyright (C) 2013 Thomas Tsou <tom@tsou.cc>
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
	.global neon_scale_4n
	.type neon_scale_4n, %function
neon_scale_4n:
	push            {r4, lr}
	ldr              r4, =32

	vld1.64          d0, [r1]
	vmov.32          s4, s1
	vmov.32          s1, s0
	vmov.64          d1, d0
	vmov.32          s5, s4
	vmov.64          d3, d2
.loop_mul_const:
	vld2.32     {q2-q3}, [r0], r4

	vmul.f32         q8, q0,  q2
	vmul.f32         q9, q1,  q3
	vmul.f32        q10, q0,  q3
	vmul.f32        q11, q1,  q2
	vsub.f32         q8, q8,  q9
	vadd.f32         q9, q10, q11

	vst2.32     {q8-q9}, [r2]!
	subs             r3, #1
	bne              .loop_mul_const
	pop             {r4, pc}
	.size neon_scale_4n, .-neon_scale_4n
	.section .note.GNU-stack,"",%progbits
