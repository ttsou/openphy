/*
 * LTE Quadrature Amplitude Modulation (QAM)
 *
 * Copyright (C) 2015 Ettus Research LLC
 * Author Tom Tsou <tom.tsou@ettus.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <complex.h>
#include <string.h>

#include "sigproc.h"
#include "sigvec_internal.h"
#include "log.h"

#define QAM16DIV	3.16227766017
#define QAM64DIV	6.48074069841
#define SCALE8		16.0

/* QPSK hard output */
int lte_qpsk_decode(struct cxvec *vec, signed char *bits, int len)
{
	float real, imag;

	if (len % 2) {
		LOG_DSP_ARG("Invalid QPSK length ", len);
		return -1;
	}

	for (int i = 0; i < len / 2; i++) {
		real = crealf(vec->data[i]);
		imag = cimagf(vec->data[i]);

		if ((real > 0.0) && (imag > 0.0)) {
			bits[2 * i + 0] = 0;
			bits[2 * i + 1] = 0;
		} else if ((real > 0.0) && (imag < 0.0)) {
			bits[2 * i + 0] = 0;
			bits[2 * i + 1] = 1;
		} else if ((real < 0.0) && (imag > 0.0)) {
			bits[2 * i + 0] = 1;
			bits[2 * i + 1] = 0;
		} else if ((real < 0.0) && (imag < 0.0)) {
			bits[2 * i + 0] = 1;
			bits[2 * i + 1] = 1;
		} else {
			bits[2 * i + 0] = 0;
			bits[2 * i + 1] = 0;
		}
	}

	return 0;
}

/* QPSK soft output */
int lte_qpsk_decode2(struct cxvec *vec, signed char *bits, int len)
{
	int i;

	if (len % 2) {
		LOG_DSP_ARG("Invalid QPSK length ", len);
		return -1;
	}

	for (i = 0; i < len / 2; i++) {
		bits[2 * i + 0] = (signed char) (crealf(vec->data[i]) * SCALE8);
		bits[2 * i + 1] = (signed char) (cimagf(vec->data[i]) * SCALE8);
	}

	return 0;
}

/* 16QAM soft output */
int lte_qam16_decode(struct cxvec *vec, signed char *bits, int len)
{
	int i;
	float a, b, c, d;

	if (len % 4) {
		LOG_DSP_ARG("Invalid 16QAM length ", len);
		return -1;
	}

	for (i = 0; i < len / 4; i++) {
		/* Bits 0 and 1 */
		a = crealf(vec->data[i]) * QAM16DIV;
		b = cimagf(vec->data[i]) * QAM16DIV;

		/* Bits 2 and 3 */
		if (a < 0.0f)
			c = a + 2.0f;
		else
			c = -a + 2.0f;

		if (b < 0.0f)
			d = b + 2.0f;
		else
			d = -b + 2.0f;

		/* Scale soft bits */
		bits[4 * i + 0] = (int8_t) (a * SCALE8);
		bits[4 * i + 1] = (int8_t) (b * SCALE8);
		bits[4 * i + 2] = (int8_t) (c * SCALE8);
		bits[4 * i + 3] = (int8_t) (d * SCALE8);
	}

	return 0;
}

/* 64QAM soft output */
int lte_qam64_decode(struct cxvec *vec, signed char *bits, int len)
{
	int i;
	float a, b, c, d, e, f;

	if (len % 6) {
		LOG_DSP_ARG("Invalid 64QAM length ", len);
		return -1;
	}

	for (i = 0; i < len / 6; i++) {
		/* Bits 0 and 1 */
		a = crealf(vec->data[i]) * QAM64DIV;
		b = cimagf(vec->data[i]) * QAM64DIV;

		/* Bits 2 and 3 */
		if (a > 0.0f)
			c = -a + 4.0f;
		else
			c = a + 4.0f;

		if (b > 0.0f)
			d = -b + 4.0f;
		else
			d = b + 4.0f;

		/* Bits 4 and 5 */
		if (a > 4.0f)
			e = -a + 6.0f;
		else if (a < -4.0f)
			e = a + 6.0f;
		else if (a > 0.0f)
			e = a - 2.0f;
		else
			e = -a - 2.0f;

		if (b > 4.0f)
			f = -b + 6.0f;
		else if (b < -4.0f)
			f = b + 6.0f;
		else if (b > 0.0f)
			f = b - 2.0f;
		else
			f = -b - 2.0f;

		/* Scale soft bits */
		bits[6 * i + 0] = (int8_t) (a * SCALE8);
		bits[6 * i + 1] = (int8_t) (b * SCALE8);
		bits[6 * i + 2] = (int8_t) (c * SCALE8);
		bits[6 * i + 3] = (int8_t) (d * SCALE8);
		bits[6 * i + 4] = (int8_t) (e * SCALE8);
		bits[6 * i + 5] = (int8_t) (f * SCALE8);
	}

	return 0;
}

/* 256QAM soft output */
int lte_qam256_decode(struct cxvec *vec, signed char *bits, int len)
{
	LOG_DSP_ERR("256-QAM not supported");
	return -1;
}
