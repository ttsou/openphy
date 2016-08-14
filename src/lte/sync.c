/*
 * LTE Synchronization
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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "pss.h"
#include "sss.h"
#include "lte.h"
#include "fft.h"
#include "correlate.h"
#include "slot.h"
#include "expand.h"
#include "log.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/*
 * PSS and SSS sample positions
 *
 * The PSS position is fractional when downsampled by 32 from the native LTE
 * rate. The integer position rounds down leaving a half sample offset between
 * sampled and actual positioning. Note that neither PSS or SSS positions are
 * 16-byte aligned, which is a case the FFT backend needs to handle.
 */
#define LTE_SSS_POS	((int) LTE_N0_SYM5)
#define LTE_PSS_POS	((int) LTE_N0_SYM6)

struct cxvec *buf_sss;
struct fft_hdl *fft_3rb;
struct fft_hdl *fft_6rb;

static int cxvec_div(struct cxvec *a, struct cxvec *b, struct cxvec *out)
{
	if ((a->len != b->len) || (b->len != out->len)) {
		fprintf(stderr, "cxvec_div: invalid length\n");
		fprintf(stderr, "a->len %i, b->len %i, out->len %i\n",
			(int) a->len, (int) b->len, (int) out->len);
		return -1;
	}

	for (int i = 0; i < a->len; i++) {
		out->data[i] = a->data[i] / b->data[i];
	}

	return 0;
}

static struct cxvec *lte_demod(struct cxvec *sym_t)
{
	struct cxvec *sym_f;
	struct fft_hdl *fft;

	switch (sym_t->len) {
	case 64:
		fft = fft_3rb;
		break;
	case 128:
		fft = fft_6rb;
		break;
	default:
		printf("Not supported length %i\n", (int) sym_t->len);
		return NULL;
	}

	sym_f = cxvec_alloc(sym_t->len, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	cxvec_fft(fft, sym_t, sym_f);

	return sym_f;
}

#define AVG_NUM		50

int ready = 0;
int avg_cnt = 0;
int super_cnt = 0;
float complex moving_avg0[AVG_NUM];
float complex moving_avg1[AVG_NUM];

static void log_sss_info(int n_id_cell, int dn, float offset)
{
	char sbuf[80];
	snprintf(sbuf, 80, "SSS   : "
		 "Cell ID %i, Slot %i, Offset %f Hz",
		 n_id_cell, dn, offset);
	LOG_SYNC(sbuf);
}

static inline float sss_norm2(float complex a)
{
	return crealf(a) * crealf(a) + cimagf(a) * cimagf(a);
}

/*
 * SSS detection and frequency offset calculation
 *
 * The symbol position of the SSS is between two samples because of the
 * fractional cyclic prefix length, which is the result of downsampling by a
 * factor of 32 from the natural 30.72 Msps rate. This means the integer
 * sample position will late relative to PSS by a half sample resulting in
 * linear phase shift across the frequency domain symbols.
 *
 * Compensate for the rotation after averaging across multiple frames. The
 * residual phase rotation of the BPSK sequence determines the early
 * acquisition frequency offset. Frequency correction switches to reference
 * symbol tracking later synchronization stages.
 */
int lte_sss_detect(struct lte_rx *rx, int n_id_2,
		   struct cxvec **slot, int chans,
		   struct lte_sync *sync)
{
	int i, mag, min;
	int dn = 0, n_id_1 = 0;
	uint64_t reg = 0;

	if ((chans < 1) || (chans > 2))
		return -1;

	struct cxvec *sym_t;
	struct cxvec *sym_f[chans];

	/* SSS_POS 343 */
	for (int i = 0; i < chans; i++) {
		sym_t = cxvec_subvec(slot[i], LTE_SSS_POS,
				     0, 0, LTE_N0_SYM_LEN);
		sym_f[i] = lte_demod(sym_t);

		cxvec_free(sym_t);
	}

	int len = sym_f[0]->len;

	for (i = 0; i < len; i++) {
		complex float a = sym_f[0]->data[i];
		complex float c = rx->pss_chan->data[i];
		float scale = sss_norm2(c);

		if (chans == 2) {
			complex float b = sym_f[1]->data[i];
			complex float d = rx->pss_chan1->data[i];
			scale += sss_norm2(d);

			sym_f[0]->data[i] = (conjf(c) * a + conjf(d) * b) / scale;
		} else {
			sym_f[0]->data[i] = conjf(c) * a / scale;
		}
	}

	for (i = 0; i < 64; i++)
		buf_sss->data[i] += sym_f[0]->data[i];

	if (++super_cnt < AVG_NUM) {
		for (i = 0; i < chans; i++)
			cxvec_free(sym_f[i]);

		return 0;
	}

	memcpy(sym_f[0]->data, buf_sss->data, 64 * sizeof(float complex));
	memset(buf_sss->data, 0, 64 * sizeof(float complex));
	ready = 1;
	super_cnt = 0;

	/* Half Pi rotation to accomodate fractional cyclic prefix timing */
	for (i = 0; i < len; i++)
		sym_f[0]->data[i] *= cosf((float) i / (float) len * M_PI) -
				     sinf((float) i / (float) len * M_PI) * I;
	for (i = len / 2; i < len; i++)
		sym_f[0]->data[i] *= -1.0f;
	for (i = 0; i < 64; i++)
		sym_f[0]->data[i] /= (float) AVG_NUM;

	/* Zero the ends */
	sym_f[0]->data[0] = 0;
	sym_f[0]->data[32] = 0;

	/* Quantize down to one bit each I and Q */
	for (int i = 0; i < len; i++)
		reg |= (uint64_t) (crealf(sym_f[0]->data[i]) < 0.0f ? 0 : 1) << i;

	min = 9999;
	for (int i = 0; i < LTE_SSS_NUM; i++) {
		mag = __builtin_popcountll(reg ^ rx->sss[n_id_2][i][0]);
		if (mag < min) {
			min = mag;
			n_id_1 = i;
			dn = 0;
		}

		mag = __builtin_popcountll(reg ^ rx->sss[n_id_2][i][1]);
		if (mag < min) {
			min = mag;
			n_id_1 = i;
			dn = 5;
		}
	}

	if (min < 20) {
		sync->n_id_1 = n_id_1;
		sync->n_id_cell = 3 * n_id_1 + n_id_2;
	} else {
		sync->n_id_1 = 0;
		sync->n_id_cell = 0;
		dn = -1;
		ready = 0;
	}

	float complex x[2] = { 0.0f, 0.0f };
	float ang;
	int cnt0 = 0, cnt1 = 0;
	uint64_t win;

	/* Enable averaging and frequency detection */
	if (dn >= 0) {
		if (!dn)
			win = rx->sss[n_id_2][n_id_1][0];
		else
			win = rx->sss[n_id_2][n_id_1][1];


		for (int i = 1; i < 64; i++) {
			if (i == 32)
				continue;

			if ((win >> i) & 0x01) {
				x[0] += sym_f[0]->data[i];
				cnt0++;
			} else {
				x[1] += sym_f[0]->data[i];
				cnt1++;
			}
		}

		x[0] /= (float) cnt0;
		x[1] /= (float) cnt1;

		/* 128-tap downsampler */
		float factor = -2280.429f;

		float mag = cabsf(x[0] - x[1]);
		ang = cargf(x[0] - x[1]);

		if (cabsf(ang) < 1.4 && (mag > 0.55f)) {
			mag = cabsf(x[0] - x[1]);
			ang = cargf(x[0] - x[1]);

			if (ready) {
				sync->f_dist = mag;
				sync->f_offset = ang * factor;
				sync->dn = dn;

				log_sss_info(sync->n_id_cell, dn, sync->f_offset);

				memset(moving_avg0, 0, AVG_NUM * sizeof(float complex));
				memset(moving_avg1, 0, AVG_NUM * sizeof(float complex));
				avg_cnt = 0;
			} else {
				sync->f_dist = mag;
				sync->f_offset = 0.0f;
				sync->dn = dn;
			}
		} else {
			sync->f_offset = 0.0f;
			sync->dn = dn;
			ready = 0;
		}
	}

	for (i = 0; i < chans; i++)
		cxvec_free(sym_f[i]);

	if (ready) {
		ready = 0;
		return 1;
	}
	else if (dn < 0)
		return dn;

	return 0;
}

/*
 * Frequency domain PSS detection
 */
int lte_pss_detect(struct lte_rx *rx, struct cxvec **slot, int chans)
{
	int n_id_2;
	float max;
	float mag[3] = { 0.0, 0.0, 0.0 };

	if ((chans < 1) || (chans > 2))
		return -1;

	struct cxvec *sym_t[chans];
	struct cxvec *sym_f[chans];

	for (int n = 0; n < chans; n++) {
		sym_t[n] = cxvec_subvec(slot[n], LTE_PSS_POS, 0, 0, LTE_N0_SYM_LEN);
		sym_f[n] = lte_demod(sym_t[n]);

		mag[0] += cabsf(cxvec_mac(sym_f[n], rx->pss_fc[0]));
		mag[1] += cabsf(cxvec_mac(sym_f[n], rx->pss_fc[1]));
		mag[2] += cabsf(cxvec_mac(sym_f[n], rx->pss_fc[2]));
	}

	max = 0.0f;
	if (mag[0] > max) {
		n_id_2 = 0;
		max = mag[0];
	}
	if (mag[1] > max) {
		n_id_2 = 1;
		max = mag[1];
	}
	if (mag[2] > max) {
		n_id_2 = 2;
	}

	/* Set the channel estimate */
	cxvec_div(sym_f[0], rx->pss_f[n_id_2], rx->pss_chan);

	if (chans == 2)
		cxvec_div(sym_f[1], rx->pss_f[n_id_2], rx->pss_chan1);

	for (int i = 0; i < chans; i++) {
		cxvec_free(sym_t[i]);
		cxvec_free(sym_f[i]);
	}

	return n_id_2;
}

int lte_pss_detect2(struct lte_rx *rx, struct cxvec **slot, int chans)
{
	int n_id_2;
	float mag = 0.0;

	if ((chans < 1) || (chans > 2))
		return -1;

	struct cxvec *sym_t[chans];
	struct cxvec *sym_f[chans];

	n_id_2 = rx->sync.n_id_2;

	/* PSS_POS 411 */
	for (int i = 0; i < chans; i++) {
		sym_t[i] = cxvec_subvec(slot[i], LTE_PSS_POS, 0, 0, LTE_N0_SYM_LEN);
		sym_f[i] = lte_demod(sym_t[i]);

		mag += cabsf(cxvec_mac(sym_f[i], rx->pss_fc[n_id_2]));
	}

	/* Set the channel estimate */
	cxvec_div(sym_f[0], rx->pss_f[n_id_2], rx->pss_chan);

	if (chans == 2)
		cxvec_div(sym_f[1], rx->pss_f[n_id_2], rx->pss_chan1);

	for (int i = 0; i < chans; i++) {
		cxvec_free(sym_t[i]);
		cxvec_free(sym_f[i]);
	}

	return 0;
}

int lte_pss_detect3(struct lte_rx *rx, struct cxvec **slot, int chans)
{
	int n_id_2, match = 0;
	float max;
	float mag[3] = { 0.0, 0.0, 0.0 };

	if ((chans < 1) || (chans > 2))
		return -1;

	struct cxvec *sym_t[chans];
	struct cxvec *sym_f[chans];

	for (int i = 0; i < chans; i++) {
		sym_t[i] = cxvec_subvec(slot[i], LTE_PSS_POS, 0, 0, LTE_N0_SYM_LEN);
		sym_f[i] = lte_demod(sym_t[i]);

		mag[0] += cabsf(cxvec_mac(sym_f[i], rx->pss_fc[0]));
		mag[1] += cabsf(cxvec_mac(sym_f[i], rx->pss_fc[1]));
		mag[2] += cabsf(cxvec_mac(sym_f[i], rx->pss_fc[2]));
	}

	max = 0.0f;
	if (mag[0] > max) {
		n_id_2 = 0;
		max = mag[0];
	}
	if (mag[1] > max) {
		n_id_2 = 1;
		max = mag[1];
	}
	if (mag[2] > max) {
		n_id_2 = 2;
	}

	if (n_id_2 == rx->sync.n_id_2)
		match = 1;
	else
		goto release;

#ifdef PDSCH_SSS_CHK 
	/* Only set set the channel estimate if we check SSS in PDSCH state*/
	cxvec_div(sym_f[0], rx->pss_f[n_id_2], rx->pss_chan);

	if (chans == 2)
		cxvec_div(sym_f[1], rx->pss_f[n_id_2], rx->pss_chan1);
#endif

release:
	for (int i = 0; i < chans; i++) {
		cxvec_free(sym_t[i]);
		cxvec_free(sym_f[i]);
	}

	return match ? 0 : -1;
}


static __attribute__((constructor)) void init()
{
	struct cxvec *in_6rb, *out_6rb;
	struct cxvec *buf_3rb0, *buf_3rb1;

	in_6rb = cxvec_alloc((lte_cp_len(6) + 128) * 7,
			     0, 0, NULL, CXVEC_FLG_FFT_ALIGN);
	out_6rb = cxvec_alloc(128 * 7, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	buf_3rb0 = cxvec_alloc(64, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);
	buf_3rb1 = cxvec_alloc(64, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	fft_3rb = init_fft(0, 64, 1, 0, 0, 1, 1, buf_3rb0, buf_3rb1, 1);
	fft_6rb = init_fft(0, 128, 7, lte_cp_len(6) + 128,
			   128, 1, 1, in_6rb, out_6rb, 0);

	buf_sss = cxvec_alloc_simple(64);

	memset(moving_avg0, 0, AVG_NUM * sizeof(float complex));
	memset(moving_avg1, 0, AVG_NUM * sizeof(float complex));

	cxvec_free(buf_3rb0);
	cxvec_free(buf_3rb1);
	cxvec_free(in_6rb);
	cxvec_free(out_6rb);
}

static __attribute__((destructor)) void release()
{
}
