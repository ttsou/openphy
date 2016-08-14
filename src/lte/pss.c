/*
 * LTE Primary Synchronization Signal (PSS)
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
#include <math.h>
#include <complex.h>

#include "pss.h"
#include "sigvec.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

struct cxvec *lte_gen_pss(unsigned n_id_2)
{
	float u;
	struct cxvec *pss;

	switch (n_id_2) {
	case 0:
		u = 25.0;
		break;
	case 1:
		u = 29.0;
		break;
	case 2:
		u = 34.0;
		break;
	default:
		fprintf(stderr, "Undefined N_id = %i\n", n_id_2);
		return NULL;
	}

	pss = cxvec_alloc(LTE_PSS_LEN, 0, 0, NULL, CXVEC_FLG_FFT_ALIGN);

	for (int n = 0; n < 31; n++) {
		pss->data[n] = cos(-M_PI * u * n * (n + 1) / 63.0) +
			       sin(-M_PI * u * n * (n + 1) / 63.0) * I;
	}

	for (int n = 31; n < LTE_PSS_LEN; n++) {
		pss->data[n] = cos(-M_PI * u * (n + 1) * (n + 2) / 63.0) +
			       sin(-M_PI * u * (n + 1) * (n + 2) / 63.0) * I;
	}

	return pss;
}
