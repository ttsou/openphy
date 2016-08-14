/*
 * Timestamped Ring Buffer
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

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <complex>
#include <limits.h>
#include <algorithm>

#include "TimestampBuffer.h"

template <typename T>
TimestampBuffer<T>::TimestampBuffer(size_t len)
    : data(len), time_start(0), time_end(0), data_start(0), data_end(0)
{
}

/* Reset time and data marker indicies */
template <typename T>
void TimestampBuffer<T>::reset()
{
    time_start = 0;
    time_end = 0;
    data_start = 0;
    data_end = 0;
}

/* Return number of available samples for a given timestamp */
template <typename T>
size_t TimestampBuffer<T>::avail_smpls(int64_t ts) const
{
    if (ts >= time_end) return 0;
    else return time_end - ts;
}

/* Read into supplied buffer with timestamp and internal copy */
template <typename T>
ssize_t TimestampBuffer<T>::read(vector<T> &buf, int64_t ts)
{
    /* Check for valid read */
    if (buf.size() >= data.size() || ts + (int64_t) buf.size() > time_end)
        return -ERR_TIMESTAMP;

    /* Disallow reads prior to read marker with no readable data */
    if (ts + (int64_t) buf.size() < time_start)
        return ERR_TIMESTAMP;

    /* How many samples should be copied out */
    size_t num_smpls = time_end - ts;
    if (num_smpls > buf.size())
        num_smpls = buf.size();

    /* Starting index */
    size_t rd_start = (data_start + (ts - time_start)) % data.size();

    /* Read out */
    if (rd_start + num_smpls < data.size()) {
        copy_n(begin(data) + rd_start, buf.size(), begin(buf));
    } else {
        auto iter = copy(begin(data)+rd_start, end(data), begin(buf));
        copy_n(begin(data), buf.size() - (data.size() - rd_start), iter);
    }

    data_start = (rd_start + buf.size()) % data.size();
    time_start = ts + buf.size();

    if (time_start > time_end) return -ERR_TIMESTAMP;
    else return num_smpls;
}

template <typename T>
ssize_t TimestampBuffer<T>::write(const T *buf, size_t len, int64_t ts)
{
    bool wrap = false;
    bool init = false;

    if ((len >= data.size()) || (ts < 0) || (ts + (int64_t) len <= time_end))
        return -ERR_TIMESTAMP;

    /* Start index */
    size_t wr_start = data_start + (ts - time_start);
    if (wr_start >= data.size()) {
        wr_start = wr_start % data.size();
        wrap = true;
    }

    /* Write it or just update head on 0 length write */
    if (len) {
        if (wr_start + len < data.size()) {
            copy_n(buf, len, &data[wr_start]);
        } else {
            size_t copy0 = data.size() - wr_start;
            size_t copy1 = len - copy0;
            copy_n(buf, copy0, &data[wr_start]);
            copy_n(&buf[copy0], copy1, begin(data));

            wrap = true;
        }
    }

    if (!data_start) {
        data_start = wr_start;
        time_start = ts;
        init = true;
    }

    data_end = (wr_start + len) % data.size();
    time_end = ts + len;

    if (!init && wrap && (data_end > data_start))
        return -ERR_OVERFLOW;
    else if (time_end <= time_start)
        return -ERR_TIMESTAMP;

    return len;
}

template <typename T>
ssize_t TimestampBuffer<T>::write(const T *buf, size_t len)
{
    return write(buf, len, time_end);
}

template <typename T>
ssize_t TimestampBuffer<T>::write(int64_t timestamp)
{
    return write(nullptr, 0, timestamp);
}

template <typename T>
std::string TimestampBuffer<T>::str_status() const
{
    std::ostringstream ost("Sample buffer: ");

    ost << "length = " << data.size();
    ost << ", time_start = " << time_start;
    ost << ", time_end = " << time_end;
    ost << ", data_start = " << data_start;
    ost << ", data_end = " << data_end;

    return ost.str();
}

template <typename T>
std::string TimestampBuffer<T>::str_code(ssize_t code)
{
    switch (code) {
    case ERR_TIMESTAMP:
        return "Sample buffer: Requested timestamp is not valid";
    case ERR_MEM:
        return "Sample buffer: Memory error";
    case ERR_OVERFLOW:
        return "Sample buffer: Overrun";
    default:
        return "Sample buffer: Unknown error";
    }
}

template class TimestampBuffer<complex<short>>;
template class TimestampBuffer<complex<float>>;
