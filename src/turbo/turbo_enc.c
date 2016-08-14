/*
 * LTE turbo encoder
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

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "turbo.h"
#include "turbo_int.h"

#define API_EXPORT	__attribute__((__visibility__("default")))
#define PARITY(X)	__builtin_parity(X)
#define POPCNT(X)	__builtin_popcount(X)

#define MAX_I		(188 + 1)

struct lte_interlv_param {
	int i;
	int k;
	int f1;
	int f2;
};

/* Plus one to accommodate indexing from 1 instead of 0 */
static int *lte_deinterlv_map[MAX_I];

/*
 * 3GPP TS 36.212 Release 8
 * Table 5.1.3-3: "Turbo code inernal interleaver parameters
 */
static struct lte_interlv_param lte_interlv_params[MAX_I] = {
	{   0,   -1,  -1,  -1, },
	{   1,   40,   3,  10, }, {   2,   48,   7,  12, },
	{   3,   56,  19,  42, }, {   4,   64,   7,  16, },
	{   5,   72,   7,  18, }, {   6,   80,  11,  20, },
	{   7,   88,   5,  22, }, {   8,   96,  11,  24, },
	{   9,  104,   7,  26, }, {  10,  112,  41,  84, },
	{  11,  120, 103,  90, }, {  12,  128,  15,  32, },
	{  13,  136,   9,  34, }, {  14,  144,  17, 108, },
	{  15,  152,   9,  38, }, {  16,  160,  21, 120, },
	{  17,  168, 101,  84, }, {  18,  176,  21,  44, },
	{  19,  184,  57,  46, }, {  20,  192,  23,  48, },
	{  21,  200,  13,  50, }, {  22,  208,  27,  52, },
	{  23,  216,  11,  36, }, {  24,  224,  27,  56, },
	{  25,  232,  85,  58, }, {  26,  240,  29,  60, },
	{  27,  248,  33,  62, }, {  28,  256,  15,  32, },
	{  29,  264,  17, 198, }, {  30,  272,  33,  68, },
	{  31,  280, 103, 210, }, {  32,  288,  19,  36, },
	{  33,  296,  19,  74, }, {  34,  304,  37,  76, },
	{  35,  312,  19,  78, }, {  36,  320,  21, 120, },
	{  37,  328,  21,  82, }, {  38,  336, 115,  84, },
	{  39,  344, 193,  86, }, {  40,  352,  21,  44, },
	{  41,  360, 133,  90, }, {  42,  368,  81,  46, },
	{  43,  376,  45,  94, }, {  44,  384,  23,  48, },
	{  45,  392, 243,  98, }, {  46,  400, 151,  40, },
	{  47,  408, 155, 102, }, {  48,  416,  25,  52, },
	{  49,  424,  51, 106, }, {  50,  432,  47,  72, },
	{  51,  440,  91, 110, }, {  52,  448,  29, 168, },
	{  53,  456,  29, 114, }, {  54,  464, 247,  58, },
	{  55,  472,  29, 118, }, {  56,  480,  89, 180, },
	{  57,  488,  91, 122, }, {  58,  496, 157,  62, },
	{  59,  504,  55,  84, }, {  60,  512,  31,  64, },
	{  61,  528,  17,  66, }, {  62,  544,  35,  68, },
	{  63,  560, 227, 420, }, {  64,  576,  65,  96, },
	{  65,  592,  19,  74, }, {  66,  608,  37,  76, },
	{  67,  624,  41, 234, }, {  68,  640,  39,  80, },
	{  69,  656, 185,  82, }, {  70,  672,  43, 252, },
	{  71,  688,  21,  86, }, {  72,  704, 155,  44, },
	{  73,  720,  79, 120, }, {  74,  736, 139,  92, },
	{  75,  752,  23,  94, }, {  76,  768, 217,  48, },
	{  77,  784,  25,  98, }, {  78,  800,  17,  80, },
	{  79,  816, 127, 102, }, {  80,  832,  25,  52, },
	{  81,  848, 239, 106, }, {  82,  864,  17,  48, },
	{  83,  880, 137, 110, }, {  84,  896, 215, 112, },
	{  85,  912,  29, 114, }, {  86,  928,  15,  58, },
	{  87,  944, 147, 118, }, {  88,  960,  29,  60, },
	{  89,  976,  59, 122, }, {  90,  992,  65, 124, },
	{  91, 1008,  55,  84, }, {  92, 1024,  31,  64, },
	{  93, 1056,  17,  66, }, {  94, 1088, 171, 204, },
	{  95, 1120,  67, 140, }, {  96, 1152,  35,  72, },
	{  97, 1184,  19,  74, }, {  98, 1216,  39,  76, },
	{  99, 1248,  19,  78, }, { 100, 1280, 199, 240, },
	{ 101, 1312,  21,  82, }, { 102, 1344, 211, 252, },
	{ 103, 1376,  21,  86, }, { 104, 1408,  43,  88, },
	{ 105, 1440, 149,  60, }, { 106, 1472,  45,  92, },
	{ 107, 1504,  49, 846, }, { 108, 1536,  71,  48, },
	{ 109, 1568,  13,  28, }, { 110, 1600,  17,  80, },
	{ 111, 1632,  25, 102, }, { 112, 1664, 183, 104, },
	{ 113, 1696,  55, 954, }, { 114, 1728, 127,  96, },
	{ 115, 1760,  27, 110, }, { 116, 1792,  29, 112, },
	{ 117, 1824,  29, 114, }, { 118, 1856,  57, 116, },
	{ 119, 1888,  45, 354, }, { 120, 1920,  31, 120, },
	{ 121, 1952,  59, 610, }, { 122, 1984, 185, 124, },
	{ 123, 2016, 113, 420, }, { 124, 2048,  31,  64, },
	{ 125, 2112,  17,  66, }, { 126, 2176, 171, 136, },
	{ 127, 2240, 209, 420, }, { 128, 2304, 253, 216, },
	{ 129, 2368, 367, 444, }, { 130, 2432, 265, 456, },
	{ 131, 2496, 181, 468, }, { 132, 2560,  39,  80, },
	{ 133, 2624,  27, 164, }, { 134, 2688, 127, 504, },
	{ 135, 2752, 143, 172, }, { 136, 2816,  43,  88, },
	{ 137, 2880,  29, 300, }, { 138, 2944,  45,  92, },
	{ 139, 3008, 157, 188, }, { 140, 3072,  47,  96, },
	{ 141, 3136,  13,  28, }, { 142, 3200, 111, 240, },
	{ 143, 3264, 443, 204, }, { 144, 3328,  51, 104, },
	{ 145, 3392,  51, 212, }, { 146, 3456, 451, 192, },
	{ 147, 3520, 257, 220, }, { 148, 3584,  57, 336, },
	{ 149, 3648, 313, 228, }, { 150, 3712, 271, 232, },
	{ 151, 3776, 179, 236, }, { 152, 3840, 331, 120, },
	{ 153, 3904, 363, 244, }, { 154, 3968, 375, 248, },
	{ 155, 4032, 127, 168, }, { 156, 4096,  31,  64, },
	{ 157, 4160,  33, 130, }, { 158, 4224,  43, 264, },
	{ 159, 4288,  33, 134, }, { 160, 4352, 477, 408, },
	{ 161, 4416,  35, 138, }, { 162, 4480, 233, 280, },
	{ 163, 4544, 357, 142, }, { 164, 4608, 337, 480, },
	{ 165, 4672,  37, 146, }, { 166, 4736,  71, 444, },
	{ 167, 4800,  71, 120, }, { 168, 4864,  37, 152, },
	{ 169, 4928,  39, 462, }, { 170, 4992, 127, 234, },
	{ 171, 5056,  39, 158, }, { 172, 5120,  39,  80, },
	{ 173, 5184,  31,  96, }, { 174, 5248, 113, 902, },
	{ 175, 5312,  41, 166, }, { 176, 5376, 251, 336, },
	{ 177, 5440,  43, 170, }, { 178, 5504,  21,  86, },
	{ 179, 5568,  43, 174, }, { 180, 5632,  45, 176, },
	{ 181, 5696,  45, 178, }, { 182, 5760, 161, 120, },
	{ 183, 5824,  89, 182, }, { 184, 5888, 323, 184, },
	{ 185, 5952,  47, 186, }, { 186, 6016,  23,  94, },
	{ 187, 6080,  47, 190, }, { 188, 6144, 263, 480, },
};

static struct lte_interlv_param *lte_interlv_find_param(int k)
{
	int i;

	for (i = 1; i < MAX_I; i++) {
		if (k == lte_interlv_params[i].k)
			return &lte_interlv_params[i];
	}

	return NULL;
}

int turbo_interleave(int k, const uint8_t *input, uint8_t *output)
{
	int i, n;
	struct lte_interlv_param *param;

	if ((k < TURBO_MIN_K) || (k > TURBO_MAX_K))
		return -EINVAL;

	param = lte_interlv_find_param(k);
	if (!param)
		return -EINVAL;

	i = param->i;
	if ((i < 0) || (i > 188))
		return -EINVAL;

	for (n = 0; n < k; n++) {
		int v = lte_deinterlv_map[param->i][n];
		if ((v < 0) || (v >= k))
			return -EINVAL;

		output[n] = input[lte_deinterlv_map[param->i][n]];
	}

	return 0;
}

int turbo_interleave_lval(int k, const int16_t *in, int16_t *out)
{
	int n;
	struct lte_interlv_param *param;

	if ((k < TURBO_MIN_K) || (k > TURBO_MAX_K))
		return -EINVAL;

	param = lte_interlv_find_param(k);
	if (!param)
		return -EINVAL;

	for (n = 0; n < k; n++)
		out[n] = in[lte_deinterlv_map[param->i][n]];

	return 0;
}

int turbo_deinterleave(int k, const int8_t *in, int8_t *out)
{
	int n;
	struct lte_interlv_param *param;

	if ((k < TURBO_MIN_K) || (k > TURBO_MAX_K))
		return -EINVAL;

	param = lte_interlv_find_param(k);
	if (!param)
		return -EINVAL;

	if (!lte_deinterlv_map[param->i])
		return -EINVAL;

	for (n = 0; n < k; n++)
		out[lte_deinterlv_map[param->i][n]] = in[n];

	return 0;
}

int turbo_deinterleave_lval(int k, const int16_t *in, int16_t *out)
{
	int n;
	struct lte_interlv_param *param;

	if ((k < TURBO_MIN_K) || (k > TURBO_MAX_K))
		return -EINVAL;

	param = lte_interlv_find_param(k);
	if (!param)
		return -EINVAL;

	if (!lte_deinterlv_map[param->i])
		return -EINVAL;

	for (n = 0; n < k; n++)
		out[lte_deinterlv_map[param->i][n]] = in[n];

	return 0;
}

static int encode_n2(const struct lte_turbo_code *code,
		     const uint8_t *c, uint8_t *x, uint8_t *z)
{
	unsigned gen, rgen, reg = 0;
	int len = code->len;
	int i, k = code->k;

	gen = code->gen;
	rgen = code->rgen;

	for (i = 0; i < len; i++) {
		reg |= (PARITY((reg & rgen)) ^ c[i]) << (k - 1);
		x[i] = c[i];
		z[i] = PARITY(reg & gen);
		reg = reg >> 1;
	}

	return reg;
}

static int encode_n2p(const struct lte_turbo_code *code,
		      const uint8_t *cp, uint8_t *zp)
{
	unsigned gen, rgen, reg = 0;
	int len = code->len;
	int i, k = code->k;

	gen = code->gen;
	rgen = code->rgen;

	for (i = 0; i < len; i++) {
		reg |= (PARITY((reg & rgen)) ^ cp[i]) << (k - 1);
		zp[i] = PARITY(reg & gen);
		reg = reg >> 1;
	}

	return reg;
}

static void turbo_term(const struct lte_turbo_code *code,
		      unsigned reg0, unsigned reg1,
		      uint8_t *d0, uint8_t *d1, uint8_t *d2)
{
	int len = code->len;
	unsigned gen = code->gen;
	unsigned rgen = code->rgen;

	/* First constituent encoder */
	d0[len + 0] = PARITY(reg0 & rgen);
	d1[len + 0] = PARITY(reg0 & gen);
	reg0 = reg0 >> 1;
	d2[len + 0] = PARITY(reg0 & rgen);
	d0[len + 1] = PARITY(reg0 & gen);
	reg0 = reg0 >> 1;
	d1[len + 1] = PARITY(reg0 & rgen);
	d2[len + 1] = PARITY(reg0 & gen);

	/* Second constituent encoder */
	d0[len + 2] = PARITY(reg1 & rgen);
	d1[len + 2] = PARITY(reg1 & gen);
	reg1 = reg1 >> 1;
	d2[len + 2] = PARITY(reg1 & rgen);
	d0[len + 3] = PARITY(reg1 & gen);
	reg1 = reg1 >> 1;
	d1[len + 3] = PARITY(reg1 & rgen);
	d2[len + 3] = PARITY(reg1 & gen);
}

void turbo_unterm(int len, uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t *d0p)
{
	uint8_t _d0[4];
	uint8_t _d1[4];
	uint8_t _d2[4];

	memcpy(_d0, &d0[len], 4 * sizeof(uint8_t));
	memcpy(_d1, &d1[len], 4 * sizeof(uint8_t));
	memcpy(_d2, &d2[len], 4 * sizeof(uint8_t));

	d0[len + 0] = _d0[0];
	d0[len + 1] = _d2[0];
	d0[len + 2] = _d1[1];

	d1[len + 0] = _d1[0];
	d1[len + 1] = _d0[1];
	d1[len + 2] = _d2[1];

	d0p[len + 0] = _d0[2];
	d0p[len + 1] = _d2[2];
	d0p[len + 2] = _d1[3];

	d2[len + 0] = _d1[2];
	d2[len + 1] = _d0[3];
	d2[len + 2] = _d2[3];
}

API_EXPORT
int lte_turbo_encode(const struct lte_turbo_code *code,
		    const uint8_t *input, uint8_t *d0, uint8_t *d1, uint8_t *d2)
{
	unsigned reg0, reg1;
	uint8_t *cp;

	if ((code->n != 2) || (code->k < 3) || (code->k > 4))
		return -EINVAL;

	cp = (uint8_t *) malloc((code->len + 3) * sizeof(uint8_t));

	reg0 = encode_n2(code, input, d0, d1);
	turbo_interleave(code->len, input, cp);
	reg1 = encode_n2p(code, cp, d2);
	turbo_term(code, reg0, reg1, d0, d1, d2);

	free(cp);

	return code->len * 3 + 4 * 3;
}

static void gen_deinterlv_map(int i)
{
	int n, k, p, f1, f2;
	struct lte_interlv_param *param;

	param = &lte_interlv_params[i];

	k = param->k;

	lte_deinterlv_map[i] = (int *) malloc(k * sizeof(int));

	f1 = param->f1;
	f2 = param->f2;

	for (n = 0; n < k; n++) {
		p = n * (f1 % k + f2 * n % k) % k;
		lte_deinterlv_map[i][n] = p;
	}
}

__attribute__((constructor)) static void init()
{
	int i;

	lte_deinterlv_map[0]= NULL;

	for (i = 1; i < MAX_I; i++)
		gen_deinterlv_map(i);
}

__attribute__((destructor)) static void release()
{
	int i;

	for (i = 1; i < MAX_I; i++)
		free(lte_deinterlv_map[i]);
}
