/*
 * Convolutional encoder
 *
 * Copyright (C) 2015 Ettus Research LLC
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
 *
 * Author: Tom Tsou <tom.tsou@ettus.com>
 */

#include <stdint.h>
#include <errno.h>
#include "conv.h"

#define API_EXPORT	__attribute__((__visibility__("default")))
#define PARITY(X)	__builtin_parity(X)
#define POPCNT(X)	__builtin_popcount(X)

static int puncture(const struct lte_conv_code *code,
		    uint8_t *unpunct, uint8_t *output)
{
	int len, i, j = 0, l = 0;
	const int *punc = code->punc;

	if (code->term == CONV_TERM_FLUSH)
		len = code->len + code->k - 1;
	else
		len = code->len;

	for (i = 0; i < len * code->n; i++) {
		if (i == punc[j]) {
			j++;
			continue;
		}

		output[l++] = unpunct[i];
	}

	return l;
}

/*
 * Non-recursive rate 1/2 encoder
 */
static int encode_n2(const struct lte_conv_code *code,
		     const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, k = code->k;

	if (code->term == CONV_TERM_TAIL_BITING) {
		for (i = 0; i < k - 1; i++)
			reg |= input[len - 1 - i] << (k - 2 - i);
	}

	for (i = 0; i < len; i++) {
		reg |= input[i] << (k - 1);
		output[2 * i + 0] = PARITY(reg & gen[0]);
		output[2 * i + 1] = PARITY(reg & gen[1]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[2 * i + 0] = PARITY(reg & gen[0]);
		output[2 * i + 1] = PARITY(reg & gen[1]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Non-recursive rate 1/3 encoder
 */
static int encode_n3(const struct lte_conv_code *code,
		     const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, k = code->k;

	if (code->term == CONV_TERM_TAIL_BITING) {
		for (i = 0; i < k - 1; i++)
			reg |= input[len - 1 - i] << (k - 2 - i);
	}

	for (i = 0; i < len; i++) {
		reg |= input[i] << (k - 1);
		output[3 * i + 0] = PARITY(reg & gen[0]);
		output[3 * i + 1] = PARITY(reg & gen[1]);
		output[3 * i + 2] = PARITY(reg & gen[2]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[3 * i + 0] = PARITY(reg & gen[0]);
		output[3 * i + 1] = PARITY(reg & gen[1]);
		output[3 * i + 2] = PARITY(reg & gen[2]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Non-recursive rate 1/4 encoder
 */
static int encode_n4(const struct lte_conv_code *code,
		     const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, k = code->k;

	if (code->term == CONV_TERM_TAIL_BITING) {
		for (i = 0; i < k - 1; i++)
			reg |= input[len - 1 - i] << (k - 2 - i);
	}

	for (i = 0; i < len; i++) {
		reg |= input[i] << (k - 1);
		output[4 * i + 0] = PARITY(reg & gen[0]);
		output[4 * i + 1] = PARITY(reg & gen[1]);
		output[4 * i + 2] = PARITY(reg & gen[2]);
		output[4 * i + 3] = PARITY(reg & gen[2]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[4 * i + 0] = PARITY(reg & gen[0]);
		output[4 * i + 1] = PARITY(reg & gen[1]);
		output[4 * i + 2] = PARITY(reg & gen[2]);
		output[4 * i + 3] = PARITY(reg & gen[2]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Recursive rate 1/2 encoder
 */
static int encode_rec_n2(const struct lte_conv_code *code,
			 int pos, const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned rgen = code->rgen;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, m0, m1, k = code->k;

	m0 = pos;
	m1 = (pos + 1) % 2;

	for (i = 0; i < len; i++) {
		reg |= (PARITY(reg & rgen) ^ input[i]) << (k - 1);
		output[2 * i + m0] = input[i];
		output[2 * i + m1] = PARITY(reg & gen[m1]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[2 * i + m0] = PARITY(reg & rgen);
		output[2 * i + m1] = PARITY(reg & gen[m1]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Recursive rate 1/3 encoder
 */
static int encode_rec_n3(const struct lte_conv_code *code,
			 int pos, const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned rgen = code->rgen;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, m0, m1, m2, k = code->k;

	m0 = pos;
	m1 = (pos + 1) % 3;
	m2 = (pos + 2) % 3;

	for (i = 0; i < len; i++) {
		reg |= (PARITY(reg & rgen) ^ input[i]) << (k - 1);
		output[3 * i + m0] = input[i];
		output[3 * i + m1] = PARITY(reg & gen[m1]);
		output[3 * i + m2] = PARITY(reg & gen[m2]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[3 * i + m0] = PARITY(reg & rgen);
		output[3 * i + m1] = PARITY(reg & gen[m1]);
		output[3 * i + m2] = PARITY(reg & gen[m2]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Recursive rate 1/4 encoder
 */
static int encode_rec_n4(const struct lte_conv_code *code,
			 int pos, const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned rgen = code->rgen;
	const unsigned *gen = code->gen;
	int len = code->len;
	int i, m0, m1, m2, m3, k = code->k;

	m0 = pos;
	m1 = (pos + 1) % 4;
	m2 = (pos + 2) % 4;
	m3 = (pos + 3) % 4;

	for (i = 0; i < len; i++) {
		reg |= (PARITY(reg & rgen) ^ input[i]) << (k - 1);
		output[4 * i + m0] = input[i];
		output[4 * i + m1] = PARITY(reg & gen[m1]);
		output[4 * i + m2] = PARITY(reg & gen[m2]);
		output[4 * i + m3] = PARITY(reg & gen[m2]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		output[4 * i + m0] = PARITY(reg & rgen);
		output[4 * i + m1] = PARITY(reg & gen[m1]);
		output[4 * i + m2] = PARITY(reg & gen[m2]);
		output[4 * i + m3] = PARITY(reg & gen[m2]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Non-recursive arbitrary rate encoder
 */
static int encode_gen(const struct lte_conv_code *code,
		      const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned *gen = gen;
	int n = code->n;
	int len = code->len;
	int i, j, k = code->k;

	if (code->term == CONV_TERM_TAIL_BITING) {
		for (i = 0; i < k - 1; i++)
			reg |= input[code->len - 1 - i] << (k - 2 - i);
	}

	for (i = 0; i < len; i++) {
		reg |= input[i] << (k - 1);
		for (j = 0; j < n; j++)
			output[n * i + j] = PARITY(reg & gen[j]);
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		for (j = 0; j < n; j++)
			output[n * i + j] = PARITY(reg & gen[j]);
		reg = reg >> 1;
	}

	return i;
}

/*
 * Recursive arbitrary rate encoder
 */
static int encode_rec_gen(const struct lte_conv_code *code,
			  const uint8_t *input, uint8_t *output)
{
	unsigned reg = 0;
	const unsigned rgen = code->rgen;
	const unsigned *gen = code->gen;
	int n = code->n;
	int len = code->len;
	int i, j, k = code->k;
	int p[n];

	for (i = 0; i < n; i++)
		p[i] = POPCNT(gen[i]) == 1;

	for (i = 0; i < len; i++) {
		reg |= (PARITY(reg & rgen) ^ input[i]) << (k - 1);

		for (j = 0; j < n; j++) {
			if (p[j])
				output[n * i + j] = input[i];
			else
				output[n * i + j] = PARITY(reg & gen[j]);
		}
		reg = reg >> 1;
	}

	if (code->term != CONV_TERM_FLUSH)
		return i;

	for (i = len; i < len + k - 1; i++) {
		for (j = 0; j < n; j++) {
			if (p[j])
				output[n * i + j] = PARITY(reg & rgen);
			else
				output[n * i + j] = PARITY(reg & gen[j]);
		}
		reg = reg >> 1;
	}

	return i;
}

/*
 * Non-recursive encoder
 */
static int conv_encode(const struct lte_conv_code *code,
		       const uint8_t *input, uint8_t *output)
{
	int l;
	uint8_t *_output;
	uint8_t unpunct[(code->len + code->k - 1) * code->n];

	if (code->punc)
		_output = unpunct;
	else
		_output = output;

	switch (code->n) {
	case 2:
		l = encode_n2(code, input, _output);
		break;
	case 3:
		l = encode_n3(code, input, _output);
		break;
	case 4:
		l = encode_n4(code, input, _output);
		break;
	default:
		l = encode_gen(code, input, _output);
		break;
	}

	if (code->punc)
		return puncture(code, _output, output);

	return code->n * l;
}

/*
 * Recursive encoder
 */
static int conv_encode_rec(const struct lte_conv_code *code,
			   const uint8_t *input, uint8_t *output)
{
	int l, pos = -1, cnt = 0;
	const unsigned *gen = code->gen;
	uint8_t *_output;
	uint8_t unpunct[(code->len + code->k - 1) * code->n];

	if (code->term == CONV_TERM_TAIL_BITING)
		return -ENOTSUP;

	if (code->punc)
		_output = unpunct;
	else
		_output = output;

	/* Count systematic bits and note position */
	for (l = 0; l < code->n; l++) {
		if (POPCNT(gen[l]) == 1) {
			pos = l;
			cnt++;
		}
	}

	/* Use generic encoder if we have more than one systematic bits */
	if ((cnt > 1)) {
		l = encode_rec_gen(code, input, _output);
		goto punc;
	}
	if (cnt < 1)
		return -EPROTO;

	switch (code->n) {
	case 2:
		l = encode_rec_n2(code, pos, input, _output);
		break;
	case 3:
		l = encode_rec_n3(code, pos, input, _output);
		break;
	case 4:
		l = encode_rec_n4(code, pos, input, _output);
		break;
	default:
		l = encode_rec_gen(code, input, _output);
		break;
	}

punc:
	if (code->punc)
		return puncture(code, _output, output);

	return code->n * l;
}

API_EXPORT
int lte_conv_encode(const struct lte_conv_code *code,
		    const uint8_t *input, uint8_t *output)
{
	if ((code->n < 2) || (code->n > 4) ||
	    ((code->k != 5) && (code->k != 7))) {
		return -EINVAL;
	}

	if (code->rgen)
		return conv_encode_rec(code, input, output);

	return conv_encode(code, input, output);
}
