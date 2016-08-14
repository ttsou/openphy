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
	.global norm4_neon
	.type norm4_neon, %function
norm4_neon:
	veor           q15,  q15
.loop_norm4:
	vld2.32     {q0-q1},  [r0]!
	vmul.f32         q2,  q0, q0
	vmul.f32         q3,  q1, q1
	vadd.f32         q8,  q2, q3
	vadd.f32        q15, q15, q8
	subs             r2,  #1
	bne       .loop_norm4

	vpadd.f32        d0, d30, d31
	veor             q1, q1
	vadd.f32         s4, s0, s1
	vst1.64          d2, [r1]
	bx               lr
	.size norm4_neon, .-norm4_neon
	.section .note.GNU-stack,"",%progbits
