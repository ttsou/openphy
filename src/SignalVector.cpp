/*
 * Floating Point Complex Signal Vector
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
#include <complex>

#include "SignalVector.h"

extern "C" {
#include "dsp/sigvec.h"
}

using namespace std;

SignalVector::SignalVector() : _local(false), _cv(nullptr)
{
}

SignalVector::SignalVector(size_t size, size_t head)
  : _local(false), _cv(nullptr)
{
    if (size > 0) {
        _cv = cxvec_alloc(size, head, 0, nullptr, 0);
        _head = head;
        _local = true;
    }
}

SignalVector::SignalVector(const SignalVector &s)
  : _local(false), _cv(nullptr)
{
    *this = s;
}

SignalVector::SignalVector(SignalVector &&s)
  : _local(false), _cv(nullptr)
{
    *this = move(s);
}

SignalVector::~SignalVector()
{
    if (_local) cxvec_free(_cv);
}

SignalVector& SignalVector::operator=(const SignalVector &s)
{
    if (this != &s) {
        if (_local) cxvec_free(_cv);

        if (s.size() > 0) {
            _cv = cxvec_alloc(s.size(), s._head, 0, nullptr, 0);
            copy(s.cbegin(), s.cend(), this->begin());
            _head = s._head;
            _local = true;
        }
    }

    return *this;
}

SignalVector& SignalVector::operator=(SignalVector &&s)
{
    if (this != &s) {
        if (_local) cxvec_free(_cv);

        _cv = s._cv;
        _head = s._head;
        _local = s._local;
        s._cv = nullptr;
    }

    return *this;
}

size_t SignalVector::size() const
{
    if (!_cv)
        return 0;

    int len = cxvec_len(_cv);
    if (len < 0) throw out_of_range("");

    return (size_t) len;
}

const complex<float> *SignalVector::cbegin() const
{
    return (const complex<float> *) cxvec_data(_cv);
}

const complex<float> *SignalVector::cend() const
{
    return (const complex<float> *) cxvec_data(_cv) + size();
}

const complex<float> *SignalVector::chead() const
{
    return (const complex<float> *) cxvec_data(_cv) - _head;
}

complex<float> *SignalVector::begin()
{
    return (complex<float> *) cxvec_data(_cv);
}

complex<float> *SignalVector::end()
{
    return (complex<float> *) cxvec_data(_cv) + size();
}

complex<float> *SignalVector::head()
{
    return (complex<float> *) cxvec_data(_cv) - _head;
}

complex<float>& SignalVector::operator[](int i)
{
    auto *buf = (complex<float> *) cxvec_data(_cv);
    return buf[i];
}

void SignalVector::reverse()
{
    cxvec_rvrs(_cv, _cv);
}

SignalVector::SignalVector(struct cxvec *v)
  : _local(false), _head(0), _cv(v)
{
}

struct cxvec *SignalVector::cv()
{
    return _cv;
}

void SignalVector::translateVectors(vector<SignalVector> &v, struct cxvec **c)
{
    for (auto &vi : v) *c++ = vi.cv();
}
