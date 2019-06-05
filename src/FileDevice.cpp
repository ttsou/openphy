/*
 * File Device Access
 *
 * Copyright (C) 2019 Tom Tsou <tom@tsou.cc>
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

#include <map>
#include <complex>
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include "FileDevice.h"

extern "C" {
#include "lte/log.h"
}

#define RX_BUFLEN        (1 << 22)
#define DEV_SPP          1024

using namespace std;

template <typename T>
void FileDevice<T>::init(int64_t &ts, size_t rbs, int, const string &filename)
{
    if (_chans != 1)
        throw runtime_error("Only single channel supported in file mode"); 
    _stream.open(filename, ios::in | ios::binary);
    if (_stream.fail()) {
        ostringstream ost;
        ost << "File \"" << filename << "\" failed to open";
        throw runtime_error(ost.str());
    }
    initRates(rbs);
    initRx(ts);
}

template <typename T>
void FileDevice<T>::resetFreq()
{
    ostringstream ost;
    ost << "DEV   : Resetting offset frequency";
    LOG_DEV(ost.str().c_str());
}

static double get_rate(int rbs)
{
    switch (rbs) {
    case 6:
        return 1.92e6;
    case 15:
        return 3.84e6;
    case 25:
        return 5.76e6;
    case 50:
        return 11.52e6;
    case 75:
        return 15.36e6;
    case 100:
        return 23.04e6;
    default:
        ostringstream ost;
        ost << "DEV   :  - Invalid sample rate selection " << rbs;
        LOG_ERR(ost.str().c_str());
    }

    return 0.0;
}

template <typename T>
void FileDevice<T>::shiftFreq(double offset)
{
    ostringstream ost;
    ost << "DEV   : Adjusting frequency offset from " << _offset_freq << " Hz"
        << " to " << _offset_freq+offset  << " Hz";
    LOG_DEV(ost.str().c_str());
    _offset_freq += offset;
}

template <typename T>
int64_t FileDevice<T>::get_ts_high()
{
    return _rx_bufs.front()->get_last_time();
}

template <typename T>
int64_t FileDevice<T>::get_ts_low()
{
    return _rx_bufs.front()->get_first_time();
}

template <typename T>
void FileDevice<T>::start()
{
}

template <typename T>
int FileDevice<T>::reload()
{
    size_t total = 0;
    vector<vector<T>> pkt_bufs(_chans, vector<T>(_spp));
    vector<T *> pkt_ptrs;
    for (auto &p : pkt_bufs) pkt_ptrs.push_back(p.data());

    for (;;) {
        _stream.read((char *) pkt_ptrs.front(), _spp * sizeof(T));
        if (_stream.eof())
            throw runtime_error("End of File");

        total += _spp;
        int64_t ts = _prev_ts + _spp;

        if (_prev_ts) {
            auto b = begin(_rx_bufs);
            for (auto &p : pkt_ptrs) {
                int rc = (*b++)->write(p, _spp, ts);
                if (rc == -TimestampBuffer<T>::ERR_OVERFLOW) {
                    ostringstream ost;
                    ost << "DEV   : " << "Internal buffer overflow";
                    LOG_ERR(ost.str().c_str());
                    continue;
                }
            }
        }

        _prev_ts = ts;

        if (total >= _spp)
            break;
    }

    return 0;
}

template <typename T>
int FileDevice<T>::pull(vector<vector<T>> &bufs, size_t len, int64_t ts)
{
    int err;

    if (bufs.size() != _chans)
        throw out_of_range("");

    if (_rx_bufs.front()->avail_smpls(ts) < len)
        throw length_error("");

    auto d = begin(_rx_bufs);
    for (auto &b : bufs)
        (*d++)->read(b, ts);

    return len;
}

template <typename T>
double FileDevice<T>::setGain(double gain)
{
    return 0.0;
}

template <typename T>
bool FileDevice<T>::initRates(int rbs)
{
    double rate = get_rate(rbs);
    if (rate == 0.0)
        return false;

    ostringstream ost;
    ost << "DEV   : " << "Setting rate to " << rate/1e6 << " MHz";
    _rate = rate;
    return true;
}

template <typename T>
void FileDevice<T>::initRx(int64_t &ts)
{
    _rx_bufs.resize(_chans);

    for (size_t i = 0; i < _chans; i++) {
        _rx_bufs[i] = make_shared<TSBuffer>(RX_BUFLEN);
    }

    _spp = DEV_SPP;

    ostringstream ost;
    ost << "DEV   : " << "Setting samples per packet to " << _spp;
    LOG_DEV(ost.str().c_str());

    vector<vector<T>> pkt_bufs(_chans, vector<T>(_spp));
    vector<T *> pkt_ptrs;
    for (auto &p : pkt_bufs) pkt_ptrs.push_back(p.data());

    ts = 0; 
    for (auto &r : _rx_bufs)
        r->write(ts);
}

template <typename T>
void FileDevice<T>::setFreq(double freq)
{
    ostringstream ost;
    ost << "DEV   : No RF frequency setting in file mode";
    LOG_DEV(ost.str().c_str());
    _offset_freq = 0.0;
}

template <typename T>
void FileDevice<T>::stop()
{
}

template <typename T>
void FileDevice<T>::reset()
{
    stop();
    _prev_ts = 0;
}

template <typename T>
FileDevice<T>::FileDevice(size_t chans)
  : _chans(chans), _offset_freq(0.0), _prev_ts(0)
{
}

template <typename T>
FileDevice<T>::~FileDevice()
{
    stop();
    _stream.close();
}

template class FileDevice<complex<short>>;
template class FileDevice<complex<float>>;
