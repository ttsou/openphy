/*
 * Polyphase Resampler
 *
 * Copyright (C) 2016 Ettus Research LLC
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

#include <algorithm>
#include "Resampler.h"

extern "C" {
#include "dsp/convolve.h"
}

#define INIT_OUTPUT_LEN		4096

using namespace std;

void Resampler::init(unsigned cutoff)
{
    vector<float> proto(partitions.size() * filterLen);

    /* 
     * Generate the prototype filter with a Blackman-harris window.
     * Scale coefficients with DC filter gain set to unity divided
     * by the number of filter partitions. 
     */
    float a[] = { 0.35875, 0.48829, 0.14128, 0.01168 };
    float scale, sum = 0.0f;
    float midpt = proto.size() / 2.0;

    int i = 0;
    for (auto &p : proto) {
        p = cxvec_sinc(((float) i - midpt) / cutoff);
        p *= a[0] -
             a[1] * cos(2 * M_PI * i / (proto.size() - 1)) +
             a[2] * cos(4 * M_PI * i / (proto.size() - 1)) -
             a[3] * cos(6 * M_PI * i / (proto.size() - 1));
        sum += p;
        i++;
    }
    scale = partitions.size() / sum;

    /* 
     * Populate partition filters and reverse the coefficients per
     * convolution requirements.
     */
    auto iter = begin(proto);
    for (auto &p : partitions) {
        for_each(begin(p), end(p), [scale, &iter](auto &a) {
            a = *iter++ * scale;
        });
        p.reverse();
    }
}

void Resampler::rotate(SignalVector &in, SignalVector &out)
{
    if (in.begin()-in.head() < history.size())
        throw out_of_range("Insufficient input head space");

    if (out.size() > paths.size())
        generatePaths(out.size());

    copy(in.begin()-history.size(), in.begin(), history.begin());

    auto out_iter = out.begin();
    auto path_iter = begin(paths);
    while (out_iter != out.end()) {
	single_convolve((float *) &in[path_iter->first],
                        (float *) partitions[path_iter->second].begin(),
                        filterLen,
                        (float *) out_iter++);
        path_iter++;
    }

    copy(in.end()-history.size(), in.end(), history.begin());
}

void Resampler::update(SignalVector &in)
{
    copy(in.end()-history.size(), in.end(), history.begin());
}

void Resampler::generatePaths(size_t n)
{
    paths.resize(n);

    int i = 0;
    for (auto &p : paths)
        p = pair<int, int>((Q * i) / P, (Q * i++) % P);
}

Resampler::Resampler(unsigned P, unsigned Q, size_t filterLen)
  : partitions(P, SignalVector(filterLen)), history(filterLen),
    paths(INIT_OUTPUT_LEN), filterLen(filterLen), P(P), Q(Q)
{
    init(P > Q ? P : Q);
    generatePaths(paths.size());
}
