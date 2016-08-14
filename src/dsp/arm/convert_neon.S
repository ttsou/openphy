/*
 * NEON type conversions
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

	.syntax unified
	.text
	.align 2
	.global neon_convert_ps_si16_4n
	.type neon_convert_ps_si16_4n, %function
neon_convert_ps_si16_4n:
	vld1.32          {q1}, [r2]
.loop_fltint:
	vld1.64       {d0-d1}, [r1]!
	vmul.f32           q0, q1
	vcvt.s32.f32       q2, q0
	vqmovn.s32         d0, q2
	vst1.64          {d0}, [r0]!
	subs               r3, #1
	bne          .loop_fltint
	bx                 lr
	.size neon_convert_ps_si16_4n, .-neon_convert_ps_si16_4n
	.text
	.align 2
	.global neon_convert_si16_ps_4n
	.type neon_convert_si16_ps_4n, %function
neon_convert_si16_ps_4n:
.loop_intflt:
	vld1.64          {d0}, [r1]!
	vmovl.s16          q1, d0
	vcvt.f32.s32       q0, q1
	vst1.64          {q0}, [r0]!
	subs               r2, #1
	bne          .loop_intflt
        bx                 lr
	.size neon_convert_si16_ps_4n, .-neon_convert_si16_ps_4n
	.section .note.GNU-stack,"",%progbits
