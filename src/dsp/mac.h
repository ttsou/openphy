/*
 * Complex multiply-accumulate
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

/* Base multiply and accumulate complex-real */
static inline void mac_real(const float *x, const float *h, float *y)
{
	y[0] += x[0] * h[0];
	y[1] += x[1] * h[0];
}

/* Base multiply and accumulate complex-*/
static inline void mac_cmplx(const float *x, const float *h, float *y)
{
	y[0] += x[0] * h[0] - x[1] * h[1];
	y[1] += x[0] * h[1] + x[1] * h[0];
}

/* Base vector complex-real multiply and accumulate */
static inline void mac_real_vec_n(const float *x,
				  const float *h,
				  float *y, int len)
{
	int i;

	for (i = 0; i < len; i++)
		mac_real(&x[2 * i], &h[2 * i], y);
}

/* Base vector complex-multiply and accumulate */
static inline void mac_cmplx_vec_n(const float *x,
				   const float *h,
				   float *y, int len)
{
	int i;

	for (i = 0; i < len; i++)
		mac_cmplx(&x[2 * i], &h[2 * i], y);
}
