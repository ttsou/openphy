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
	.global mac_cx_neon4
	.type mac_cx_neon4, %function
mac_cx_neon4:
	push           {r4, lr}
	ldr             r4, =32
	veor           q14, q14
	veor           q15, q15
.loop_mac4:
	vld2.32     {q0-q1}, [r0], r4
	vld2.32     {q2-q3}, [r1]!
/*
	vswp             d0, d1
	vswp             d2, d3
	vrev64.32        q0, q0
	vrev64.32        q1, q1
*/
	vmul.f32       q10,  q0, q2
	vmul.f32       q11,  q1, q3
	vmul.f32       q12,  q0, q3
	vmul.f32       q13,  q2, q1
	vsub.f32        q8, q10, q11
	vadd.f32        q9, q12, q13

	vadd.f32        q14, q8
	vadd.f32        q15, q9
	subs             r3, #1
	bne       .loop_mac4

	vld1.64          d0, [r2]
	vpadd.f32       d28, d28, d29
	vpadd.f32       d30, d30, d31
	vpadd.f32        d1, d28, d30
	vadd.f32         d1, d0
	vst1.64          d1, [r2]
        pop             {r4, pc}
	.size mac_cx_neon4, .-mac_cx_neon4
	.section .note.GNU-stack,"",%progbits
