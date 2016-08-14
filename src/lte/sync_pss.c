/*
 * LTE Early Acquisition and Synchronization
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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "lte.h"
#include "sync.h"
#include "sigvec.h"
#include "correlate.h"
#include "sigvec_internal.h"
#include "expand.h"

#define PSS_LEN		64
#define SIGN(V)		((V < 0.0f) ? 0 : 1)

/* Bitwise 64 element dot product */
static int16_t pss_bit_dot_prod(uint64_t xr, uint64_t xi,
				uint64_t hr, uint64_t hi)
{
	int16_t s0, s1;
	uint64_t d0, d1, d2, d3, d4, d5, d6, d7;

	/* Multiply */
	d0 = ~(xr ^ hr);
	d1 = ~(xi ^ hi);
	d2 = ~(xr ^ hi);
	d3 = ~(xi ^ hr);

	/* Positive sum */
	d4 = d0 & ~d1;
	d6 = d2 & d3;

	/* Negative sum */
	d5 = ~d0 & d1;
	d7 = ~d2 & ~d3;

	/* Accumulate */
	s0 = __builtin_popcountll(d4) - __builtin_popcountll(d5);
	s1 = __builtin_popcountll(d6) - __builtin_popcountll(d7);

	return s0 * s0 + s1 * s1;
}

/* Bitwise 64 element cross correlation */
static void pss_bit_corr(uint8_t *in, int16_t *out, int len,
			uint64_t hr, uint64_t hi)
{
	uint64_t xr = 0, xi = 0;

	for (int i = 0; i < len; i++) {
		xr = (xr >> 1) | ((uint64_t) in[2 * i + 0] << 63);
		xi = (xi >> 1) | ((uint64_t) in[2 * i + 1] << 63);

		out[i] = pss_bit_dot_prod(xr, xi, hr, hi);
	}
}

/* Floating point slice and convert to unpacked 8-bit integer */
static void pss_slice_cvt(float complex *in, uint8_t *out, int len)
{
	for (int i = 0; i < len; i++) {
		out[2 * i + 0] = SIGN(crealf(in[i]));
		out[2 * i + 1] = SIGN(cimagf(in[i]));
	}
}

/* Find the maximum value and index of a 16-bit sequence */
static int pss_findmax(int16_t *seq, int len, int *pos)
{
	int i, v, p = 0, max = 0;

	for (i = 0; i < len; i++) {
		v = seq[i];
		if (v > max) {
			max = v;
			p = i;
		}
	}

	if (pos)
		*pos = p;

	return max;
}

/* Fractional peak determination with interpolation */
static int pss_sync_frac(struct cxvec *vec, int pos, int chan)
{
        int max;

        if (pos > vec->len - 4)
                return 0;

        struct cxvec *cut = cxvec_subvec(vec, pos, 0, 0, 2);
        struct cxvec *expand = cxvec_expand(cut);

        max = cxvec_max_idx(expand);

        cxvec_free(cut);
        cxvec_free(expand);

        return max;
}

/* PSS: Quantized full span time domain synchronization */
int lte_pss_search(struct lte_rx *rx, struct cxvec **subframe,
		   int chans, struct lte_sync *sync)
{
	if ((chans < 1) || (chans > 2))
		return -EINVAL;

	int corr_pss = 0;
	int corr_mag = 0;
	int corr_pos = 0;
	int len = subframe[0]->len;

	int16_t corr[len];
	uint8_t sliced[2][2 * len];

	uint64_t pss_r[3] = { rx->pss[0][0], rx->pss[1][0], rx->pss[2][0] };
	uint64_t pss_i[3] = { rx->pss[0][1], rx->pss[1][1], rx->pss[2][1] };

	for (int i = 0; i < chans; i++)
		pss_slice_cvt(subframe[i]->data, sliced[i], len);

	for (int i = 0; i < 3; i++) {
		for (int n = 0; n < chans; n++) {
			int pos, mag = 0;

			pss_bit_corr(sliced[n], corr, len, pss_r[i], pss_i[i]);

			mag = pss_findmax(corr, len, &pos);
			if (mag > corr_mag) {
				corr_pss = i;
				corr_mag = mag;
				corr_pos = pos;
			}
		}
	}

	sync->n_id_2 = corr_pss;
	sync->coarse = corr_pos;
	sync->fine = 0;
	sync->mag = (float) corr_mag;

	return 0;
}

/* PSS: Narrower time domain synchronization */
int lte_pss_sync(struct lte_rx *rx, struct cxvec **subframe,
		 int chans, struct lte_sync *sync, int n_id_2)
{
	if ((chans < 1) || (chans > 2) || (n_id_2 < 0) || (n_id_2 > 2))
		return -1;

	int i, n, sf_len = subframe[0]->len;
	int corr_len = 60;
	int corr_start = 416 + 20;

	float par;
	struct cxvec *corr[chans];

	for (i = 0; i < chans; i++) {
		corr[i] = cxvec_alloc_simple(sf_len);
		cxvec_corr(subframe[i], rx->pss_t[n_id_2],
			   corr[i], corr_start, corr_len);
	}

	/* Channel combining */
	for (i = 1; i < chans; i++) {
		for (n = 0; n < sf_len; n++)
			corr[0]->data[n] += corr[i]->data[n];
	}

	par = cxvec_par(corr[0]);

	sync->n_id_2 = n_id_2;
	sync->coarse = cxvec_max_idx(corr[0]);
	sync->fine = 0;
	sync->mag = par;

	for (i = 0; i < chans; i++)
		cxvec_free(corr[i]);

	return 0;
}

/* PSS: Narrowest time domain synchronization with fractional interpolation */
int lte_pss_fine_sync(struct lte_rx *rx, struct cxvec **subframe,
		      int chans, struct lte_sync *sync, int n_id_2)
{
	if ((chans < 1) || (chans > 2) || (n_id_2 < 0) || (n_id_2 > 2))
		return -EINVAL;

	int i, n, pos, sf_len = subframe[0]->len;
	int corr_len = 40;
	int corr_start = 464;

	float par;
	struct cxvec *corr[chans];

	for (i = 0; i < chans; i++) {
		corr[i] = cxvec_alloc_simple(sf_len);
		cxvec_corr(subframe[i], rx->pss_t[n_id_2],
			   corr[i], corr_start, corr_len);
	}

	/* Channel selection */
	for (n = 0; n < sf_len; n++)
		corr[0]->data[n] = cabsf(corr[0]->data[n]);

	pos = cxvec_max_idx(corr[0]);
	pss_sync_frac(corr[0], pos + 7, 0);

	if (chans == 2) {
		for (n = 0; n < sf_len; n++)
			corr[1]->data[n] = cabsf(corr[1]->data[n]);

		pos = cxvec_max_idx(corr[1]);
		pss_sync_frac(corr[1], pos + 7, 1);
	}

	for (i = 1; i < chans; i++) {
		for (n = 0; n < sf_len; n++)
			corr[0]->data[n] += cabsf(corr[i]->data[n]);
	}

	par = cxvec_par(corr[0]);
	pos = cxvec_max_idx(corr[0]);

	/* These values are magic! */
	sync->n_id_2 = n_id_2;
	sync->coarse = pos;
	sync->fine = pss_sync_frac(corr[0], pos + 7, 2) - 15;
	sync->mag = par;

	for (i = 0; i < chans; i++)
		cxvec_free(corr[i]);

	return 0;
}
