/*
 * LTE Gold sequences 
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
#include <math.h>

#include "ref.h"
#include "sigvec.h"
#include "sigvec_internal.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

static void gold_shift(unsigned *reg0, unsigned *reg1, int num)
{
	for (int i = 0; i < num; i++) {
		*reg0 = (((*reg0 >> 0 & 0x1) ^
			  (*reg0 >> 3 & 0x1)) << 30) | (*reg0 >> 1);

		*reg1 = (((*reg1 >> 0 & 0x1) ^
			  (*reg1 >> 1 & 0x1) ^
			  (*reg1 >> 2 & 0x1) ^
			  (*reg1 >> 3 & 0x1)) << 30) | (*reg1 >> 1);
	}
}

/* Initialize registers and fast-forward 1600 places */
static void init_regs(unsigned init, unsigned *reg0, unsigned *reg1)
{
	int Nc = 1600;

	*reg0 = 0x01;
	*reg1 = init;

	gold_shift(reg0, reg1, Nc);
}

/*
 * 3GPP 26.211 Release 8: 7.2 Pseudo-random sequence generation
 */
struct cxvec *lte_gen_gold_cxvec(unsigned init, int len)
{
	unsigned reg0;
	unsigned reg1;

	struct cxvec *seq;
	seq = cxvec_alloc_simple(len);

	init_regs(init, &reg0, &reg1);

	for (int i = 0; i < len; i++) {
		seq->data[i] = ((reg0 >> 0) & 0x1) ^
			       ((reg1 >> 0) & 0x1);

		gold_shift(&reg0, &reg1, 1);
	}

	return seq;
}

/*
 * 3GPP 26.211 Release 8: 7.2 Pseudo-random sequence generation
 */
void lte_gen_gold_seq(unsigned init, signed char *seq, int len)
{
	unsigned reg0;
	unsigned reg1;

	init_regs(init, &reg0, &reg1);

	for (int i = 0; i < len; i++) {
		seq[i] = ((reg0 >> 0) & 0x1) ^
			 ((reg1 >> 0) & 0x1);

		gold_shift(&reg0, &reg1, 1);
	}
}
