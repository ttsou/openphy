/*
 * LTE Scrambling
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

#include <gold.h>
#include "scramble.h"

void lte_pbch_gen_scrambler(unsigned init, signed char *seq, int len)
{
	lte_gen_gold_seq(init, seq, len);
}

void lte_pdcch_gen_scrambler(unsigned init, signed char *seq, int len)
{
	lte_gen_gold_seq(init, seq, len);
}

void lte_ul_pusch_gen_scrambler(unsigned init, signed char *c, int len)
{
	lte_gen_gold_seq(init, c, len);
}

void lte_scramble(signed char *bits, signed char *c, int len)
{
	int i;

	for (i = 0; i < len; i++)
		bits[i] ^= c[i];
}

void lte_scramble2(signed char *bits, signed char *c, int len)
{
	int i;

	for (i = 0; i < len; i++)
		bits[i] *= 2 * c[i] - 1;
}

void lte_descramble(signed char *bits, signed char *c, int len)
{
	lte_scramble(bits, c, len);
}

#define X	0
#define Y	0

void lte_ul_scramble(signed char *b_tilda, const signed char *b,
		     const signed char *c, int M_bit)
{
	int i;

	for (i = 0; i < M_bit; i++) {
		if (b[i] == X) {
			b_tilda[i] = 1;
			continue;
		}

		if (b[i] == Y)
			b_tilda[i] = b_tilda[i - 1];
		else
			b_tilda[i] = (b[i] + c[i]) % 2;
	}
}
