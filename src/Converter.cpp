/*
 * LTE Subframe Converter
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
#include "Converter.h"

extern "C" {
#include "lte/slot.h"
}

using namespace std;

static bool use_fft_1536(auto rbs)
{
    switch (rbs) {
    case 25:
    case 50:
    case 100:
        return true;
    }

    return false;
}

/*
 * Decimation factor for a given number of resource blocks
 *
 * Resource Blocks   Sample Rate   Decimation
 *         6           1.92 Msps       16
 *        15           3.84 Msps        8
 *        25           5.76 Msps        4
 *        50          11.52 Msps        2
 *        75          15.36 Msps        2
 *       100          23.04 Msps        1
 */
static auto decim(auto rbs)
{
    switch (rbs) {
    case 6:
        return 16;
    case 15:
        return 8;
    case 25:
        return 4;
    case 50:
        return 2;
    case 75:
        return 2;
    case 100:
        return 1;
    default:
        throw std::invalid_argument("Bad number of resource blocks");
    }
}

template <typename T>
void Converter<T>::init(size_t rbs)
{
    if (rbs == _rbs)
        return;

    size_t pdschLen = lte_subframe_len(rbs);
    size_t pbchLen = lte_subframe_len(6);
    size_t pssLen = lte_subframe_len(6) / 2;

    for (auto &b : _buffers) b.resize(pdschLen);
    for (auto &b : _prev) b = SignalVector(pdschLen, _taps);
    for (auto &b : _pdsch) b = SignalVector(pdschLen, _taps);
    for (auto &b : _pbch) b = SignalVector(pbchLen);
    for (auto &b : _pss) b = SignalVector(pssLen);

    int d = decim(rbs);
    int pss_q = use_fft_1536(rbs) ? 32 * 3 / 4 / d : 32 / d;
    int pbch_q = pss_q / 2;

    for (auto &p : _pssResamplers) p = Resampler(1, pss_q, _taps);
    for (auto &p : _pbchResamplers) p = Resampler(1, pbch_q, _taps);

    _rbs = rbs;
    reset();
}

template <typename T>
void Converter<T>::convertPDSCH()
{
    if (_convertPDSCH) return;

    auto convert = [](const auto &in, auto &out) {
        const auto s = 1.0 / 128.0;
        transform(cbegin(in), cend(in), begin(out), [s](auto &a) {
            return complex<float>(a.real()*s, a.imag()*s);
        });
    };

    auto _copy = [](const auto &in, auto &out) {
        copy(cbegin(in), cend(in), begin(out));
    };

    auto p = begin(_pdsch);
    if (sizeof(T) == sizeof(complex<short>))
        for (auto &b : _buffers) convert(b, *p++);
    else if (sizeof(T) == sizeof(complex<float>))
        for (auto &b : _buffers) _copy(b, *p++);
    else
        throw runtime_error("Unsupported sample type");

    _convertPDSCH = true;
}

template <typename T>
void Converter<T>::convertPBCH()
{
    if (_convertPBCH) return;
    if (_convertPDSCH == false) convertPDSCH();

    auto p = begin(_pbch);
    auto r = begin(_pbchResamplers);

    for (auto &b : _pdsch) r++->rotate(b, *p++);
   _convertPBCH = true;
}

template <typename T>
void Converter<T>::convertPSS()
{
    if (_convertPSS) return;
    if (_convertPDSCH == false) convertPDSCH();

    auto p = begin(_pss);
    auto r = begin(_pssResamplers);

    for (auto &b : _pdsch) r++->rotate(b, *p++);
   _convertPSS = true;
}

template <typename T>
void Converter<T>::convertPBCH(size_t channel, SignalVector &v)
{
    if (channel > channels()) throw out_of_range("");
    if (_convertPDSCH == false) convertPDSCH();

    auto &b = _pdsch[channel];
    auto &r = _pbchResamplers[channel];

    r.rotate(b, v);
}

template <typename T>
void Converter<T>::delayPDSCH(vector<vector<complex<float>>> &v, int offset)
{
    if (v.size() != channels()) throw out_of_range("");
    if (_convertPDSCH == false) convertPDSCH();

    int min = - _taps/2;
    int max = pdschLen() - _taps/2;

    if (offset < min) offset = min;
    else if (offset > max) offset = max;

    auto pi = begin(_prev);
    auto bi = begin(_pdsch);

    for (auto &vi : v) {
        vi.resize(bi->size());
        auto iter = copy(end(*pi) - _taps/2 - offset, end(*pi), begin(vi));
        copy(bi->begin(), bi->begin() + distance(iter, end(vi)), iter);
        pi++;
        bi++;
    }
}

template <typename T>
void Converter<T>::update()
{
    auto r = begin(_pssResamplers);
    for (auto &b : _pdsch) r++->update(b);

    r = begin(_pbchResamplers);
    for (auto &b : _pdsch) r++->update(b);
}

template <typename T>
void Converter<T>::reset()
{
    _convertPDSCH = false;
    _convertPBCH = false;
    _convertPSS = false;

    swap(_prev, _pdsch);
}

template <typename T>
size_t Converter<T>::channels() const
{
    return _pdsch.size();
}

template <typename T>
size_t Converter<T>::pdschLen() const
{
    return !_pdsch.size() ? 0 : _pdsch.front().size();
}

template <typename T>
Converter<T>::Converter(size_t chans, size_t taps)
  : _prev(chans), _buffers(chans), _pdsch(chans), _pbch(chans),
    _pss(chans), _pssResamplers(chans), _pbchResamplers(chans), _taps(taps)
{
}

template class Converter<complex<short>>;
template class Converter<complex<float>>;
