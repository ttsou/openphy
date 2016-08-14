/*
 * Max-Log-MAP LTE turbo decoder
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

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "turbo.h"
#include "turbo_int.h"
#include "turbo_sse.h"

#define SSE_ALIGN		__attribute__((aligned(16)))
#define API_EXPORT		__attribute__((__visibility__("default")))

#define NUM_TRELLIS_STATES	8
#define MAX_TRELLIS_LEN		(TURBO_MAX_K + 3)

/* Initialization value for forward and backward sums */
#define SUM_INIT		16000

struct tmetric {
	int16_t bm[NUM_TRELLIS_STATES];
	int16_t fwsums[NUM_TRELLIS_STATES];
};

/*
 * Trellis Object
 *
 * sums       - Accumulated path metrics
 * outputs    - Trellis ouput values
 * metrics    - Path metrics
 * vals       - Input value that led to each state
 */
struct vtrellis {
	struct tmetric *tm;
	int16_t *bwsums;
	int16_t *fwnorm;
	int16_t lvals[MAX_TRELLIS_LEN];
};

/*
 * Turbo Decoder
 *
 * n         - Code order 
 * k         - Constraint length 
 * len       - Horizontal length of trellis
 * trellis   - Trellis object
 * punc      - Puncturing sequence
 * paths     - Trellis paths
 */
struct tdecoder {
	int len;
	struct vtrellis trellis[2];

	SSE_ALIGN int16_t bwsums[8];
	SSE_ALIGN struct tmetric tm[MAX_TRELLIS_LEN + 1];
	int16_t fwnorm[MAX_TRELLIS_LEN];
};

/* Allocate and initialize the trellis object */
static int generate_trellis(struct vtrellis *trellis,
			    struct tmetric *tm,
			    int16_t *bwsums, int16_t *fwnorm)
{
	trellis->tm = tm;
	trellis->bwsums = bwsums;
	trellis->fwnorm = fwnorm;

	return 0;
}

/*
 * Reset decoder
 */
static void init_tdec(struct tdecoder *dec, int len)
{
	memset(dec->trellis[0].lvals, 0, len * sizeof(int16_t));
	memset(dec->trellis[1].lvals, 0, len * sizeof(int16_t));

	dec->len = len;
}

/* Release decoder object */
API_EXPORT void free_tdec(struct tdecoder *dec)
{
	if (!dec)
		return;

	free(dec);
}

/*
 * Allocate decoder object
 *
 * Subtract the constraint length K on the normalization interval to
 * accommodate the initialization path metric of the zero state. For very
 * long codes, we could create a special case to run a short interval for
 * the first round, and a longer interval afterwards. But, just keep it
 * simple for now.
 */
API_EXPORT struct tdecoder *alloc_tdec()
{
	struct tdecoder *dec;

	dec = (struct tdecoder *) malloc(sizeof(struct tdecoder));
	dec->len = 0;

	generate_trellis(&dec->trellis[0], dec->tm,
			 dec->bwsums, dec->fwnorm);

	generate_trellis(&dec->trellis[1], dec->tm,
			 dec->bwsums, dec->fwnorm);

	memset(dec->tm[0].fwsums, 0, 8 * sizeof(int16_t));
	memset(dec->bwsums, 0, 8 * sizeof(int16_t));

	dec->tm[0].fwsums[0] = SUM_INIT;

	return dec;
}

static int turbo_iterate(struct vtrellis *trellis, int len,
			 const int8_t *x, const int8_t *z)
{
	int i;
	struct tmetric *tm = trellis->tm;

	/* We have enough headroom to not clear all sums */
	trellis->bwsums[0] = SUM_INIT;

	/* Forward */
	for (i = 0; i < len; i++) {
		trellis->fwnorm[i] = gen_fw_metrics(tm[i].bm,
						    x[i], z[i],
						    tm[i].fwsums,
						    tm[i + 1].fwsums,
						    trellis->lvals[i]);
	}

	/* Backward */
	for (i = len - 1; i >= 0; i--) {
		trellis->lvals[i] = gen_bw_metrics(tm[i].bm,
						   z[i],
						   tm[i].fwsums,
						   trellis->bwsums,
						   trellis->fwnorm[i]);
	}

	return 0;
}

static inline void _turbo_decode(struct tdecoder *dec,
				 int len, int iter, uint8_t *output,
				 const int8_t *d0, const int8_t *d1,
				 const int8_t *d2)
{
	int i;
	int8_t d0p[len + 3];
	struct vtrellis *trellis = dec->trellis;

	turbo_interleave(len, (uint8_t *) d0, (uint8_t *) d0p);
	turbo_unterm(len, (uint8_t *) d0, (uint8_t *) d1,
		     (uint8_t *) d2, (uint8_t *) d0p);

	init_tdec(dec, len + 3);

	for (i = 0; i < iter; i++) {
		turbo_iterate(&trellis[0], dec->len, d0, d1);
		turbo_interleave_lval(len,
				      trellis[0].lvals,
				      trellis[1].lvals);

		turbo_iterate(&trellis[1], dec->len, d0p, d2);
		turbo_deinterleave_lval(len,
					trellis[1].lvals,
					trellis[0].lvals);
	}
}

#define SLICE_PACK_LE(X,I) \
	((X[I + 0] > 0 ? 1 : 0) << 0) | \
	((X[I + 1] > 0 ? 1 : 0) << 1) | \
	((X[I + 2] > 0 ? 1 : 0) << 2) | \
	((X[I + 3] > 0 ? 1 : 0) << 3) | \
	((X[I + 4] > 0 ? 1 : 0) << 4) | \
	((X[I + 5] > 0 ? 1 : 0) << 5) | \
	((X[I + 6] > 0 ? 1 : 0) << 6) | \
	((X[I + 7] > 0 ? 1 : 0) << 7)

#define SLICE_PACK_BE(X,I) \
	((X[I + 0] > 0 ? 1 : 0) << 7) | \
	((X[I + 1] > 0 ? 1 : 0) << 6) | \
	((X[I + 2] > 0 ? 1 : 0) << 5) | \
	((X[I + 3] > 0 ? 1 : 0) << 4) | \
	((X[I + 4] > 0 ? 1 : 0) << 3) | \
	((X[I + 5] > 0 ? 1 : 0) << 2) | \
	((X[I + 6] > 0 ? 1 : 0) << 1) | \
	((X[I + 7] > 0 ? 1 : 0) << 0)
/*
 * Default to big endian byte packing
 */
#ifdef PACK_LE
#define SLICE_PACK	SLICE_PACK_LE
#else
#define SLICE_PACK	SLICE_PACK_BE
#endif

API_EXPORT
int lte_turbo_decode(struct tdecoder *dec,
		     int len, int iter, uint8_t *output,
		     const int8_t *d0, const int8_t *d1, const int8_t *d2)
{
	int i;

	if ((len < TURBO_MIN_K) || len > TURBO_MAX_K)
		return -EINVAL;

	_turbo_decode(dec, len, iter, output, d0, d1, d2);

	for (i = 0; i < len / 8; i++)
		output[i] = SLICE_PACK(dec->trellis[0].lvals, 8 * i);

	return 0;
}

API_EXPORT
int lte_turbo_decode_unpack(struct tdecoder *dec,
			   int len, int iter, uint8_t *output,
			   const int8_t *d0, const int8_t *d1, const int8_t *d2)
{
	int i;

	if ((len < TURBO_MIN_K) || len > TURBO_MAX_K)
		return -EINVAL;

	_turbo_decode(dec, len, iter, output, d0, d1, d2);

	for (i = 0; i < len; i++)
		output[i] = dec->trellis[0].lvals[i] > 0 ? 1 : 0;

	return 0;
}
