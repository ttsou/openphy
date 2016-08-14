/*
 * Copyright (C) 2012 Thomas Tsou <tom@tsou.cc>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */

	.syntax unified
	.arch armv7-a
	.fpu neon
	.text
	.align 2
	.p2align 4,,15
	.arm
	.global mul_const_neon4
	.type mul_const_neon4, %function
mul_const_neon4:
	push            {r4, lr}
	ldr              r4, =-32

	vld1.64          d0, [r1]
	vmov.32          s4, s1
	vmov.32          s1, s0
	vmov.64          d1, d0
	vmov.32          s5, s4
	vmov.64          d3, d2
.loop_mul_const:
	vld2.32     {q2-q3}, [r0], r4
	vld2.32   {q12-q13}, [r2]
	vswp             d4, d5
	vswp             d6, d7
	vrev64.32        q2, q2
	vrev64.32        q3, q3

	vmul.f32         q8, q0,  q2
	vmul.f32         q9, q1,  q3
	vmul.f32        q10, q0,  q3
	vmul.f32        q11, q2,  q1
	vsub.f32         q8, q8,  q9
	vadd.f32         q9, q10, q11

	vadd.f32         q8, q8, q12
	vadd.f32         q9, q9, q13
	vst2.32     {q8-q9}, [r2]!
	subs             r3, #1
	bne              .loop_mul_const
        pop             {r4, pc}
	.size mul_const_neon4, .-mul_const_neon4
	.section .note.GNU-stack,"",%progbits
