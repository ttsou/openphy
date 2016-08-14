/*
 * Polyphase Interpolator
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
#include <math.h>
#include <complex.h>

#include "sigvec.h"
#include "convolve.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define INTERP_PATHS		32
#define INTERP_WIDTH		16

static struct cxvec **partitions;

static int init_filter(int paths)
{
	int flags = CXVEC_FLG_REAL_ONLY | CXVEC_FLG_MEM_ALIGN;
	int proto_len = paths * INTERP_WIDTH;

	float sum = 0.0f;
	float scale = 0.0f;
	float midpt = (proto_len - 1) / 2.0f;
	float cutoff;

	float proto[proto_len];

	partitions = malloc(paths * sizeof *partitions);
	if (!partitions)
		return -1;

	for (int i = 0; i < paths; i++)
		partitions[i] = cxvec_alloc(INTERP_WIDTH, 0, 0, NULL, flags);

	float a0 = 0.35875;
	float a1 = 0.48829;
	float a2 = 0.14128;
	float a3 = 0.01168;

	cutoff = (float) paths;

	for (int i = 0; i < proto_len; i++) {
		proto[i] = cxvec_sinc(((float) i - midpt) / cutoff);
		proto[i] *= a0 -
			    a1 * cosf(2 * M_PI * i / (proto_len - 1)) +
			    a2 * cosf(4 * M_PI * i / (proto_len - 1)) -
			    a3 * cosf(6 * M_PI * i / (proto_len - 1));
		sum += proto[i];
	}
	scale = paths / sum;
	for (int i = 0; i < INTERP_WIDTH; i++) {
		for (int n = 0; n < paths; n++)
			partitions[n]->data[i] = proto[i * paths + n] * scale;
	}

	for (int i = 0; i < paths; i++) {
		cxvec_rvrs(partitions[i], partitions[i]);
	}

	return 0;
}

static void deinit_filter()
{
	for (int i = 0; i < INTERP_PATHS; i++)
		cxvec_free(partitions[i]);

	free(partitions);
}

static int rotate(struct cxvec *in, struct cxvec *out)
{
	for (int i = 0; i < in->len; i++) {
		for (int n = 0; n < INTERP_PATHS; n++) {
			single_convolve((const float *) &in->data[i],
					(const float *) partitions[n]->data,
					partitions[n]->len,
					(float *) &out->data[i * INTERP_PATHS + n]);
		}
	}

	return 0;
}

struct cxvec *cxvec_expand(struct cxvec *vec)
{
	struct cxvec *expand = cxvec_alloc_simple(vec->len * INTERP_PATHS);

	rotate(vec, expand);

	return expand;
}

static __attribute__((constructor)) void init()
{
	init_filter(INTERP_PATHS);
}

static __attribute__((destructor)) void release()
{
	deinit_filter();
}
