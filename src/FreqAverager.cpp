/*
 * Frquency Offset Averaging
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

#include <thread>

#include "FreqAverager.h"

using namespace std;

FreqAverager::FreqAverager(size_t n) : _size(n)
{
}

FreqAverager::FreqAverager(const FreqAverager &a)
  : FreqAverager(a._size)
{
}

FreqAverager::FreqAverager(FreqAverager &&a)
{
    *this = move(a);
}

FreqAverager &FreqAverager::operator=(const FreqAverager &a)
{
    if (this != &a) {
      _freqs = decltype(_freqs)();
      _size = a._size;
    }
    return *this;
}

FreqAverager &FreqAverager::operator=(FreqAverager &&a)
{
    if (this != &a) {
      _freqs = move(a._freqs);
      _size = a._size;
    }
    return *this;
}

bool FreqAverager::empty()
{
    lock_guard<std::mutex> guard(_mutex);
    return _freqs.empty();
}

bool FreqAverager::full()
{
    lock_guard<mutex> guard(_mutex);
    return _freqs.size() >= _size;
}

void FreqAverager::push(double f)
{
    lock_guard<mutex> guard(_mutex);
    _freqs.push(f);
}

double FreqAverager::average()
{
    double a = 0.0;

    lock_guard<mutex> guard(_mutex);
    auto s = _freqs.size();
    if (!s) return 0.0;

    while (!_freqs.empty()) {
        a += _freqs.front();
        _freqs.pop();
    }

    return a / s;
}
