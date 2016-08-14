/*
 * LTE PSS and SSS sequences 
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
#include <stdlib.h>
#include <string.h>
#include <complex.h>

#include "lte.h"
#include "pss.h"
#include "sss.h"
#include "sigproc.h"
#include "slot.h"
#include "sigvec_internal.h"

struct fft_hdl *fft_3rb_r;

/*
 * Generate time domain PSS sequence
 */
static struct cxvec *lte_gen_pss_t(unsigned n_id_2, int *pss_i,
				   unsigned long long *pss_ll)
{
	int len = LTE_N0_SYM_LEN;
	struct cxvec *pss, *pss_t, *pss_f;

	pss = lte_gen_pss(n_id_2);
	pss_f = cxvec_alloc(len, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);
	pss_t = cxvec_alloc(len, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	/* Indices 0 and 32 must be initialized */
	cxvec_reset(pss_f);

	memcpy(&pss_f->data[64 - 31],
	       &pss->data[0], 31 * sizeof(complex float));

	memcpy(&pss_f->data[1],
	       &pss->data[31], 31 * sizeof(complex float));

	cxvec_fft(fft_3rb_r, pss_f, pss_t);

	cxvec_free(pss_f);
	cxvec_conj(pss_t);

	*pss_ll = 0;

	int real, imag;
	pss_ll[0] = 0;
	pss_ll[1] = 0;
	for (int i = 0; i < 64; i++) {
		real = crealf(pss_t->data[i]) < 0.0f ? 0 : 1;
		imag = cimagf(pss_t->data[i]) < 0.0f ? 0 : 1;
		pss_i[2 * i + 0] = (float) real;
		pss_i[2 * i + 1] = (float) imag;

		pss_ll[0] |= (unsigned long long) real << i;
		pss_ll[1] |= (unsigned long long) imag << i;
	}

//	free_fft(fft);
	cxvec_free(pss);
	return pss_t;
}

#include <math.h>

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/*
 * Generate frequency domain PSS sequence
 */
static struct cxvec *lte_gen_pss_f(unsigned n_id_2, int conj)
{
	struct cxvec *pss, *pss_f;
	int len = LTE_N0_SYM_LEN;

	pss = lte_gen_pss(n_id_2);
	pss_f = cxvec_alloc_simple(len);

	cxvec_reset(pss_f);

	memcpy(&pss_f->data[len - 31],
	       &pss->data[0], 31 * sizeof(complex float));

	memcpy(&pss_f->data[1],
	       &pss->data[31], 31 * sizeof(complex float));

	if (conj) {
		cxvec_conj(pss_f);

		/* Half Pi rotation to accomodate fractional cyclic prefix timing */
		int i;
		for (i = 0; i < len; i++) {
			pss_f->data[i] *= cosf((float) i / (float) len * M_PI) -
					  sinf((float) i / (float) len * M_PI) * I;
		}
	}

	cxvec_free(pss);
	return pss_f;
}

/*
 * Generate frequency domain SSS sequence
 */
unsigned long long lte_gen_sss_f(unsigned n_id_1, unsigned n_id_2, int subframe)
{
	struct lte_sss *sss;
	struct cxvec *d;
	int len = LTE_N0_SYM_LEN;
	unsigned long long reg = 0;

	sss = lte_gen_sss(n_id_1, n_id_2);
	d = cxvec_alloc_simple(len);

	if (!subframe) {
		memcpy(&d->data[len - 31],
		       &sss->d0->data[0], 31 * sizeof(complex float));
		memcpy(&d->data[1],
		       &sss->d0->data[31], 31 * sizeof(complex float));
	} else {
		memcpy(&d->data[len - 31],
		       &sss->d5->data[0], 31 * sizeof(complex float));
		memcpy(&d->data[1],
		       &sss->d5->data[31], 31 * sizeof(complex float));
	}

	for (int i = 0; i < len; i++)
		reg |= (unsigned long long) (crealf(d->data[i]) < 0.0f ? 0 : 1) << i;

	cxvec_free(d);
	cxvec_free(sss->d0);
	cxvec_free(sss->d5);
	free(sss);

	return reg;
}

struct lte_rx *lte_init()
{
	struct lte_rx *rx = malloc(sizeof *rx);
	if (!rx) {
		fprintf(stderr, "Memory allocation error\n");
		return NULL;
	}

	memset(rx, 0, sizeof(struct lte_rx));

	for (int i = 0; i < LTE_PSS_NUM; i++) {
		rx->pss_f[i] = lte_gen_pss_f(i, 0);
		if (!rx->pss_f[i]) {
			fprintf(stderr, "Failed to generate PSS sequence %i\n", i);
			return NULL;
		}

		rx->pss_fc[i] = lte_gen_pss_f(i, 1);
		if (!rx->pss_fc[i]) {
			fprintf(stderr, "Failed to generate PSS sequence %i\n", i);
			return NULL;
		}

		rx->pss_t[i] = lte_gen_pss_t(i, &rx->pss_i[i][0], &rx->pss[i][0]);
		if (!rx->pss_t[i]) {
			fprintf(stderr, "Failed to generate PSS sequence %i\n", i);
			return NULL;
		}

		for (int n = 0; n < 168; n++) {
			rx->sss[i][n][0] = lte_gen_sss_f(n, i, 0);
			rx->sss[i][n][1] = lte_gen_sss_f(n, i, 5);
		}
	}

	rx->pss_chan = cxvec_alloc_simple(rx->pss_f[0]->len);
	rx->pss_chan1 = cxvec_alloc_simple(rx->pss_f[0]->len);

	return rx;
}

void lte_free(struct lte_rx *rx)
{
	if (!rx)
		return;

	for (int i = 0; i < LTE_PSS_NUM; i++) {
		cxvec_free(rx->pss_f[i]);
		cxvec_free(rx->pss_fc[i]);
		cxvec_free(rx->pss_t[i]);
	}

	cxvec_free(rx->pss_chan);
	cxvec_free(rx->pss_chan1);
}

static __attribute__((constructor)) void init()
{
	struct cxvec *buf_3rb0 = cxvec_alloc(64, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);
	struct cxvec *buf_3rb1 = cxvec_alloc(64, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	fft_3rb_r = init_fft(1, 64, 1, 0, 0, 1, 1, buf_3rb0, buf_3rb1, 0);
}
