/*
 * LTE Physical Downlink Control Channel (PDCCH) Multiplexing and Scrambling
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

#include <pdcch_interleave.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
 * 3GPP TS 36.211 6.8.5 "Mapping to resource elements"
 *
 * Sub-block interleaver re-uses the specification for
 * convolutionally encoded channels.
 *
 * 3GPP TS 36.212 5.1.3.2.1 "Sub-block interleaver"
 */
static void _permute_fw(int *in, int *out)
{
	out[ 0] = in[ 1];
	out[ 1] = in[17];
	out[ 2] = in[ 9];
	out[ 3] = in[25];
	out[ 4] = in[ 5];
	out[ 5] = in[21];
	out[ 6] = in[13];
	out[ 7] = in[29];
	out[ 8] = in[ 3];
	out[ 9] = in[19];
	out[10] = in[11];
	out[11] = in[27];
	out[12] = in[ 7];
	out[13] = in[23];
	out[14] = in[15];
	out[15] = in[31];
	out[16] = in[ 0];
	out[17] = in[16];
	out[18] = in[ 8];
	out[19] = in[24];
	out[20] = in[ 4];
	out[21] = in[20];
	out[22] = in[12];
	out[23] = in[28];
	out[24] = in[ 2];
	out[25] = in[18];
	out[26] = in[10];
	out[27] = in[26];
	out[28] = in[ 6];
	out[29] = in[22];
	out[30] = in[14];
	out[31] = in[30];
}

static void lte_conv_scramble(int *d, int rows, int *v)
{
	int i;

	for (i = 0; i < rows; i++)
		_permute_fw(&d[i * 32], &v[i * 32]);
}

static int readout_rows_strip(int *v, int *seq, int rows)
{
	int i, n, val, cnt = 0;

	for (i = 0; i < 32; i++) {
		for (n = 0; n < rows; n++) {
			val = v[32 * n + i];
			if (val >= 0) {
				seq[cnt++] = val;
			}
		}
	}

	return cnt;
}

static int lte_gen_interlv_seq(int *seq, int len)
{
	int i, rc;
	int rows = (int) ceil((float) len / 32.0f);
	int shift = rows * 32 - len;
	int d[rows * 32];
	int v[rows * 32];

	for (i = 0; i < rows * 32; i++) {
		if (i < shift)
			d[i] = -1;
		else
			d[i] = i - shift;
	}

	lte_conv_scramble(d, rows, v);
	rc = readout_rows_strip(v, seq, rows);
	if (rc != len) {
		fprintf(stderr, "Fault in sub-block interleaver %i\n", rc);
		return -1;
	}

	return 0;
}

/*
 * Forward sub-block column permutation
 *
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
void permute_fw(struct lte_pdcch_deinterlv *d, cquadf *in, cquadf *out)
{
	for (int i = 0; i < d->len; i++)
		out[i] = in[d->seq[i]];
}

/*
 * Reverse sub-block column permutation
 *
 * 3GPP TS 35.212 5.1.3.2.1 "Sub-block interleaver"
 */
void permute_rv(struct lte_pdcch_deinterlv *d, cquadf *in, cquadf *out)
{
	for (int i = 0; i < d->len; i++)
		out[d->seq[i]] = in[i];
}

int pdcch_deinterlv(struct lte_pdcch_deinterlv *d, cquadf *in, cquadf *out)
{
	permute_rv(d, in, out);
	return 0;
}

static void cyclic_shift(int *seq, int len, int shift)
{
	int cyc[len];

	for (int i = 0; i < len; i++)
		cyc[i] = seq[(i + shift) % len];

	memcpy(seq, cyc, len * sizeof(int));
}

struct lte_pdcch_deinterlv *lte_alloc_pdcch_deinterlv(int len, int n_cell_id)
{
	struct lte_pdcch_deinterlv *d;
	d = malloc(sizeof *d);


	d->len = len;
	d->seq = malloc(len * sizeof *d->seq);

	lte_gen_interlv_seq(d->seq, len);
	cyclic_shift(d->seq, len, n_cell_id);

	return d;
}

void lte_free_pdcch_deinterlv(struct lte_pdcch_deinterlv *d)
{
	free(d->seq);
	free(d);
}
