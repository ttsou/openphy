/*
 * LTE Precoding
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
#include <complex.h>

#include "subframe.h"
#include "precode.h"
#include "sigproc.h"
#include "ofdm.h"
#include "log.h"
#include "sigvec_internal.h"

int lte_unprecode(struct lte_sym *sym0, struct lte_sym *sym1,
		  int tx_ants, int rx_ants, int rb, int k0, int k1,
		  struct cxvec *data, int index)
{
	if ((rx_ants < 1) || (rx_ants > 2))  {
		LOG_DSP_ERR("Invalid Rx antenna combination");
		return -1;
	}

	switch (tx_ants) {
	case 1:
		if (rx_ants == 1)
			return lte_unprecode_1x1(sym0, rb,
						 k0, k1, data, index);
		else
			return lte_unprecode_1x2(sym0, sym1, rb,
						 k0, k1, data, index);
		break;
	case 2:
		if (rx_ants == 1)
			return lte_unprecode_2x1(sym0, rb,
						 k0, k1, data, index);
		else
			return lte_unprecode_2x2(sym0, sym1, rb,
						 k0, k1, data, index);
		break;
	default:
		break;
	}

	printf("Antennas %i\n", tx_ants);
	LOG_DSP_ERR("Invalid Tx antenna combination");
	return -1;
}

/*
 * 1 Tx - 1 Rx
 */
int lte_unprecode_1x1(struct lte_sym *sym0,
		      int rb, int k0, int k1,
		      struct cxvec *data, int idx)
{
	float scale[2];
	complex float a, b, c, d;

	struct lte_ref *ref = sym0->ref;

	if (idx + 1 >= data->len) {
		LOG_DSP_ERR("No room left in precoding output buffer");
		return -1;
	}

	/* Channel squared amplitude */
	a = ref->rb[2][rb]->data[k0];
	b = ref->rb[2][rb]->data[k1];

	scale[0] = 1.0f / a;
	scale[1] = 1.0f / b;

	a = sym0->rb[rb]->data[k0];
	b = sym0->rb[rb]->data[k1];
	c = ref->rb[0][rb]->data[k0];
	d = ref->rb[0][rb]->data[k1];

	data->data[idx + 0] = scale[0] * (a * conjf(c));
	data->data[idx + 1] = scale[1] * (b * conjf(d));

	return 0;
}

/*
 * 1 Tx - 2 Rx
 */
int lte_unprecode_1x2(struct lte_sym *sym0, struct lte_sym *sym1,
		      int rb, int k0, int k1,
		      struct cxvec *data, int idx)
{
	float scale[2];
	complex float a, b, c, d, e, f, g, h;

	struct lte_ref *ref[2] = { sym0->ref, sym1->ref };

	if (idx + 1 >= data->len) {
		LOG_DSP_ERR("No room left in precoding output buffer");
		return -1;
	}

	/* Channel squared amplitude */
	a = ref[0]->rb[2][rb]->data[k0];
	b = ref[0]->rb[2][rb]->data[k1];
	c = ref[1]->rb[2][rb]->data[k0];
	d = ref[1]->rb[2][rb]->data[k1];

	scale[0] = 1.0f / (a + c);
	scale[1] = 1.0f / (b + d);

	a = sym0->rb[rb]->data[k0];
	b = sym0->rb[rb]->data[k1];
	c = ref[0]->rb[0][rb]->data[k0];
	d = ref[0]->rb[0][rb]->data[k1];

	e = sym1->rb[rb]->data[k0];
	f = sym1->rb[rb]->data[k1];
	g = ref[1]->rb[0][rb]->data[k0];
	h = ref[1]->rb[0][rb]->data[k1];

	data->data[idx + 0] = scale[0] * (a * conjf(c) + e * conjf(g));
	data->data[idx + 1] = scale[1] * (b * conjf(d) + f * conjf(h));

	return 0;
}

/*
 * 2 Tx - 1 Rx
 */
int lte_unprecode_2x1(struct lte_sym *sym,
		      int rb, int k0, int k1, struct cxvec *data, int idx)
{
	float scale[2];
	complex float a, b, c, d, e, f;

	struct lte_ref *ref = sym->ref;

	if (idx + 1 >= data->len) {
		LOG_DSP_ERR("No room left in precoding output buffer");
		return -1;
	}

	/* Channel squared amplitude */
	a = ref->rb[2][rb]->data[k0];
	b = ref->rb[2][rb]->data[k1];

	scale[0] = 1.0f / a;
	scale[1] = 1.0f / b;

	/* Rx antenna 1 symbols */
	a = sym->rb[rb]->data[k0];
	b = sym->rb[rb]->data[k1];

	/* Rx antenna 1 channel */
	c = ref->rb[0][rb]->data[k0];
	d = ref->rb[1][rb]->data[k1];
	e = ref->rb[1][rb]->data[k0];
	f = ref->rb[0][rb]->data[k1];

	data->data[idx + 0] = scale[0] * a * conjf(c) + scale[1] * conjf(b) * d;
	data->data[idx + 1] = scale[1] * b * conjf(f) + scale[0] * -conjf(a) * e;

	return 0;
}

/*
 * 2 Tx - 2 Rx
 */
int lte_unprecode_2x2(struct lte_sym *sym0, struct lte_sym *sym1,
		      int rb, int k0, int k1, struct cxvec *data, int idx)
{
	float scale[2];
	complex float a, b, c, d, e, f, g, h, i, j, k, l;

	struct lte_ref *ref[2] = { sym0->ref, sym1->ref };

	if (idx + 1 >= data->len) {
		LOG_DSP_ERR("No room left in precoding output buffer");
		return -1;
	}

	/* Channel squared amplitude */
	a = ref[0]->rb[2][rb]->data[k0];
	b = ref[0]->rb[2][rb]->data[k1];
	c = ref[1]->rb[2][rb]->data[k0];
	d = ref[1]->rb[2][rb]->data[k1];

	scale[0] = 1.0f / (a + c);
	scale[1] = 1.0f / (b + d);

	/* Rx antenna 1 symbols */
	a = sym0->rb[rb]->data[k0];
	b = sym0->rb[rb]->data[k1];

	/* Rx antenna 1 channel */
	c = ref[0]->rb[0][rb]->data[k0];
	d = ref[0]->rb[1][rb]->data[k1];
	e = ref[0]->rb[1][rb]->data[k0];
	f = ref[0]->rb[0][rb]->data[k1];

	/* Rx antenna 2 symbols */
	g = sym1->rb[rb]->data[k0];
	h = sym1->rb[rb]->data[k1];

	/* Rx antenna 2 channel */
	i = ref[1]->rb[0][rb]->data[k0];
	j = ref[1]->rb[1][rb]->data[k1];
	k = ref[1]->rb[1][rb]->data[k0];
	l = ref[1]->rb[0][rb]->data[k1];

	data->data[idx + 0] = scale[0] * (a * conjf(c) + g * conjf(i)) +
			      scale[1] * (conjf(b) * d + conjf(h) * j);
	data->data[idx + 1] = scale[1] * (b * conjf(f) + h * conjf(l)) +
			      scale[0] * (-conjf(a) * e + -conjf(g) * k);
	return 0;
}
