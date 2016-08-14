/*
 * Complex Signal Vectors
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

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "sigvec.h"
#include "sigvec_internal.h"
#include "fft.h"

#ifndef M_PIf
#define M_PIf                 (3.14159265358979323846264338327f)
#endif

/*
 * Memory alignment size if requested. SSE aligned loads require memory
 * aligned at 16 byte boundaries. This is only relevant for certain
 * vectors such as filter taps, which will always be accessed from a
 * known, aligned location - such as 0.
 */
#define CXVEC_ALIGN_BYTES	16
#define CXVEC_ALIGN_SAMPS	(CXVEC_ALIGN_BYTES / sizeof(float complex))

static void cxvec_init(struct cxvec *vec, int len, int buf_len,
		       int start, float complex *buf, int flags)
{
	vec->len = len;
	vec->buf_len = buf_len;
	vec->flags = flags;
	vec->start_idx = start;
	vec->buf = buf;
	vec->data = &buf[start];
}

/*! \brief Allocate and initialize a complex vector
 *  \param[in] len The buffer length in complex samples
 *  \param[in] start Starting index for useful data
 *  \param[in] tail Extra sample space appended
 *  \param[in] buf Pointer to a pre-existing buffer
 *  \param[in] flags Flags for various vector attributes
 *
 *  If buf is NULL, then a buffer will be allocated.
 */
struct cxvec *cxvec_alloc(int len, int head, int tail,
			  void *buf, int flags)
{
	int buf_size;
	struct cxvec *vec;

	/* Disallow zero length data */
	if ((len <= 0) || (head < 0) || (tail < 0)) {
		fprintf(stderr, "cxvec_alloc: invalid input\n");
		return NULL;
	}

	vec = malloc(sizeof *vec);

	buf_size = (len + head + tail) * sizeof(float complex);
	if (!buf) {
		if (flags & CXVEC_FLG_MEM_ALIGN)
			buf = memalign(CXVEC_ALIGN_BYTES, buf_size);
		else if (flags & CXVEC_FLG_FFT_ALIGN)
			buf = fft_malloc(buf_size);
		else if (flags & CXVEC_FLG_MEM_INIT)
			buf = calloc(1, buf_size);
		else
			buf = malloc(buf_size);
	} else {
		flags |= CXVEC_FLG_MEM_CHILD;
	}

	cxvec_init(vec, len, len + head + tail, head, buf, flags);

	/* Initialize with memset() only if alignment is requested. Otherwise
	 * the initialization flag will cause allocation with calloc(). */
	if ((flags & CXVEC_FLG_MEM_INIT) &&
	    ((flags & CXVEC_FLG_MEM_ALIGN) || (flags & CXVEC_FLG_FFT_ALIGN)))
		memset(vec->buf, 0, vec->buf_len * sizeof(complex float));

	return vec;
}

struct cxvec *cxvec_alloc_simple(int len)
{
	return cxvec_alloc(len, 0, 0, NULL, 0);
}

void *cxvec_data(struct cxvec *vec)
{
	return vec->data;
}

void *cxvec_get_buffer(struct cxvec *vec)
{
	return vec->buf;
}

int cxvec_len(struct cxvec *vec)
{
	return vec->len;
}

int cxvec_get_buffer_size(struct cxvec *vec)
{
	return vec->buf_len * sizeof(complex float);
}


/*! \brief Allocate and initialize a complex subvector
 *
 *  Only the buffer itself is allocated. The underlying buffer belongs to
 *  the parent vector. Flags from the parent will inherit except for alignment
 *  flags, which will depend on the reqeusted indices. If the subvector memory
 *  alignment flag is set, unaligned subvectors will be disallowed.
 */
struct cxvec *cxvec_subvec(struct cxvec *vec, int start,
			   int head, int tail, int len)
{
	struct cxvec *subvec;
	int flags = vec->flags;

	if (!vec || (head < 0) || (tail < 0) || (len < 0))
		return NULL;

	if ((flags & CXVEC_FLG_MEM_ALIGN) || (flags & CXVEC_FLG_FFT_ALIGN)) {
		if (start % CXVEC_ALIGN_SAMPS) {
			if (flags & CXVEC_FLG_SUB_MEM_ALIGN)
				return NULL;

			flags &= ~CXVEC_FLG_MEM_ALIGN;
			flags &= ~CXVEC_FLG_FFT_ALIGN;
		}
	}

	if ((start + len + tail) > vec->len) {
		fprintf(stderr, "cxvec_subvec: Invalid indices\n");
		fprintf(stderr, "cxvec_subvec: %i + %i + %i exceeds %i\n",
			start, len, tail, (int) vec->len);
		return NULL;
	}

	subvec = malloc(sizeof *subvec);

	flags |= CXVEC_FLG_MEM_CHILD;
	cxvec_init(subvec, len, head + len + tail,
		   head, &vec->data[start], flags);

	return subvec;
}

/*! \brief Allocate an alias of a complex vector 
 *
 *  Allocate and copy from the existing descriptor. The underlying buffer
 *  belongs to the parent vector and will not be released on object free.
 */
struct cxvec *cxvec_alias(struct cxvec *vec)
{
	struct cxvec *alias;
	int flags = vec->flags | CXVEC_FLG_MEM_CHILD;

	alias = malloc(sizeof *alias);
	cxvec_init(alias, vec->len, vec->buf_len,
		   vec->start_idx, vec->buf, flags);

	return alias;
}

/*! \brief Free a complex vector 
 *  \param[in] vec The complex vector to free
 *
 *  If the buffer exists, it will be freed. Set the buffer pointer to NULL if
 *  this is not desired behaviour. Memory allocated from memalign() can be
 *  recalimed with free() using glibc. Check memory reclamation requirments
 *  for other systems.
 */
void cxvec_free(struct cxvec *vec)
{
	if (!vec)
		return;

	if (!(vec->flags & CXVEC_FLG_MEM_CHILD)) {
		if (vec->flags & CXVEC_FLG_FFT_ALIGN)
			fft_free_buf(vec->buf);
		else
			free(vec->buf);
	}

	free(vec);
}

/*! \brief Copy the contents of a complex vector to another
 *  \param[in] src Source complex vector 
 *  \param[out] dst m Destination complex vector
 *
 *  Vectors must be of identical length. Head and tail space are not handled.
 */
int cxvec_cp(struct cxvec *a, struct cxvec *b, int idx_a, int idx_b, int len)
{
	if (idx_a + len > a->len) {
		fprintf(stderr, "cxvec_cp: invalid output length or index\n");
		return -1;
	} else if (idx_b + len > b->len) {
		fprintf(stderr, "cxvec_cp: invalid input length or index\n");
		return -1;
	}

	memcpy(&a->data[idx_a], &b->data[idx_b], sizeof(float complex) * len);
	return len;
}

complex float cxvec_max_val(struct cxvec *vec)
{
	complex float max = 0.0f;

	for (int i = 0; i < vec->len; i++) {
		if (cabsf(vec->data[i]) > cabsf(max))
			max = vec->data[i];
	}

	return max;
}

float cxvec_par(struct cxvec *vec)
{
	int num = 64;
	int low, high, idx = -1;
	float val;
	float avg = 0.0f;
	float max = 0.0f;

	for (int i = 0; i < vec->len; i++) {
		val = cabsf(vec->data[i]);

		if (val > max) {
			max = val;
			idx = i;
		}
	}

	low = (idx - num < 0) ? 0 : idx - num;
	high = (idx + num > vec->len) ? vec->len : idx + num;

	for (int i = low; i < high; i++) {
		if (abs(i - idx) < 3)
			continue;
		if (i == idx)
			continue;

		avg += cabsf(vec->data[i]);
	}

	return max / (avg / ((num - 2) * 2));
}

int cxvec_max_idx(struct cxvec *vec)
{
	int idx = 0;
	float val, max = 0.0f;

	for (int i = 0; i < vec->len; i++) {
		val = cabsf(vec->data[i]);
		if (val > max) {
			max = val;
			idx = i;
		}
	}

	return idx;
}

void cxvec_conj(struct cxvec *vec)
{
	for (int i = 0; i < vec->len; i++)
		vec->data[i] = conj(vec->data[i]);
}

struct cxvec *cxvec_fftshift(struct cxvec *vec)
{
	int len = vec->len;
	struct cxvec *shift = cxvec_alloc_simple(len);
	if (!shift)
		return NULL;

	memcpy(shift->data, &vec->data[len / 2],
	       len / 2 * sizeof(float complex));
	memcpy(&shift->data[len / 2], vec->data,
	       len / 2 * sizeof(float complex));

	return shift;
}

float cxvec_avgpow(struct cxvec *vec)
{
	float sum = 0.0f;

	for (int i = 0; i < vec->len; i++)
		sum += cabsf(vec->data[i]);

	return sum / vec->len;
}

float cxvec_min(struct cxvec *vec)
{
	float min = FLT_MAX;

	for (int i = 0; i < vec->len; i++) {
		float val = cabsf(vec->data[i]);
		if (val < min)
			min = val;
	}

	return min;
}

/*! \brief Set all values of a complex vector to zero
 *  \param[in] vec The complex vector to zero
 *
 *  The entire buffer is set to zero including headroom.
 */
void cxvec_reset(struct cxvec *vec)
{
	memset(vec->data, 0, vec->len * sizeof(float complex));
}

/*! \brief Deinterleave M complex vectors with forward loading
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 *
 *  Deinterleave a complex vector from channel 0 to 'M-1'.
 */
int cxvec_deinterlv_fw(struct cxvec *in, struct cxvec **out, int m)
{
	int i, n;

	assert(!(in->len % m));

	for (i = 0; i < (in->len / m); i++) {
		for (n = 0; n < m; n++) {
			out[n]->data[i] = in->data[i * m + n];
		}
	}

	return 0;
}

/*! \brief Deinterleave M complex vectors with reverse loading
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 *
 *  Deinterleave a complex vector from channel 'M-1' to 0.
 */
int cxvec_deinterlv_rv(struct cxvec *in, struct cxvec **out, int m)
{
	int i, n;

	assert(!(in->len % m));

	for (i = 0; i < (in->len / m); i++) {
		for (n = 0; n < m; n++) {
			out[m-1-n]->data[i] = in->data[i * m + n];
		}
	}

	return i;
}

/*! \brief Interleave M complex vectors
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 *  \param[in] m Number of channels
 */
int cxvec_interlv(struct cxvec **in, struct cxvec *out, int m)
{
	int i, n;

	for (i = 0; i < in[0]->len; i++) {
		for (n = 0; n < m; n++) {
			out->data[i * m + n] = in[n]->data[i];
		}
	}

	return i;
}

int cxvec_cyc_shft(struct cxvec *in, struct cxvec *out, int shift)
{
	if (in->len != out->len) {
		fprintf(stderr, "cxvec_cyc_shft: lengths do not match\n");
		return -1;
	}

        for (int i = 0; i < in->len; i++) {
                out->data[i] = in->data[(i + shift) % in->len];
        }

        return 0;
}

/*! \brief Reverse a complex vector
 *  \param[in] in Complex input vector
 *  \param[out] out Complex output vector pointers
 */
int cxvec_rvrs(struct cxvec *in, struct cxvec *out)
{
	int i;
	struct cxvec *rev = cxvec_alloc(in->len, 0, 0, NULL, 0);

	for (i = 0; i < in->len; i++)
		rev->data[i] = in->data[in->len - 1 - i];

	memcpy(out->data, rev->data, in->len * sizeof(float complex));

	cxvec_free(rev);

	return 0;
}

/*! \brief Subtract the contents of a complex vector from another
 *  \param[in] a Input complex vector 'A'
 *  \param[in] b Input complex vector 'B'
 *  \param[out] out Output complex vector
 *
 * Vector 'B' is subtracted from vector 'A' and the results are written to
 * the output vector. All vectors must be of identical length. 
 */
int cxvec_sub(struct cxvec *a, struct cxvec *b, struct cxvec *out)
{
	int i, len = a->len;

	if ((b->len != len) || (out->len != len)) {
		fprintf(stderr, "cxvec_sub: invalid input\n");
		return -1;
	}

	for (i = 0; i < a->len; i++)
		out->data[i] = a->data[i] * b->data[i];

	return a->len;
}

float cxvec_sinc(float x)
{
	if (x == 0.0f)
		return .9999999999;

	return sin(M_PIf * x) / (M_PIf * x);
}
