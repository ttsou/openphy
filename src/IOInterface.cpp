/*
 * I/O Interface
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <complex>
#include <stdexcept>

#include "IOInterface.h"
#include "UHDDevice.h"
#include "FileDevice.h"

extern "C" {
#include "lte/log.h"
#include "lte/slot.h"
}

#define DEV_START_OFFSET    20
#define LTE_RATE            30.72e6

using namespace std;

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
static int get_decim(int rbs)
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
    }

    return -1;
}

static int use_fft_1536(int rbs)
{
    switch (rbs) {
    case 6:
        return 0;
    case 15:
        return 0;
    case 25:
        return 1;
    case 50:
        return 1;
    case 75:
        return 0;
    case 100:
        return 1;
    }

    return -1;
}

#define TIMING_OFFSET_FUNC(N,LIM0,LIM1) \
    static int timing_offset_rb##N(int coarse, int fine) \
    { \
        /* Sample 0 adjustment */ \
        if (!coarse) { \
            if (fine < LIM0) \
                return -1; \
            else \
                return 0; \
        } \
        /* Sample 1 adjustment */ \
        if (fine > LIM1) \
            return 1; \
        else \
            return 0; \
    } \

TIMING_OFFSET_FUNC(100,32,6)
TIMING_OFFSET_FUNC(75,30,9)
TIMING_OFFSET_FUNC(50,29,9)
TIMING_OFFSET_FUNC(25,26,13)
TIMING_OFFSET_FUNC(15,22,14)
TIMING_OFFSET_FUNC(6,22,16)

int (*fine_timing_offset)(int coarse, int fine) = NULL;

template <typename T>
IOInterface<T>::IOInterface(size_t chans)
  : _chans(chans), _prevFrameNum(0), _ref(UHDDevice<>::REF_UNKNOWN),
    _freq(0.0), _offset(0.0), _gain(0.0)
{
}

template <typename T>
bool IOInterface<T>::openFile(unsigned rbs, const std::string &filename)
{
    try {
        _device = make_shared<FileDevice<T>>(_chans);
        _device->init(_ts0, rbs, _ref, filename);
    } catch (exception& e) {
        ostringstream ost;
        ost << "DEV   : " << e.what();
        LOG_ERR(ost.str().c_str());
        return false;
    }

    _rbs = rbs;
    _args = filename;
    return open(rbs);
}

template <typename T>
bool IOInterface<T>::openDevice(unsigned rbs, int ref, const std::string &args)
{
    try {
        _device = make_shared<UHDDevice<T>>(_chans);
        _device->init(_ts0, rbs, ref, args);
    } catch (exception& e) {
        return false;
    }

    _rbs = rbs;
    _ref = ref;
    _args = args;
    return open(rbs);
}

template <typename T>
bool IOInterface<T>::reopen(unsigned rbs)
{
    if (isFile()) {
        LOG_DEV_ERR("File device cannot be reopened");
        LOG_DEV_ERR("Resource block configuration mismatch");
        return false;
    }
    return openDevice(rbs, _ref, _args);
}

template <typename T>
bool IOInterface<T>::open(unsigned rbs)
{
    int baseQ = get_decim(rbs);
    if (use_fft_1536(rbs))
        _pssTimingAdjust = 32 * 3 / 4 / baseQ;
    else
        _pssTimingAdjust = 32 / baseQ;

    _frameSize = lte_subframe_len(rbs);
    _ts0 += _frameSize * DEV_START_OFFSET;

    ostringstream ost;
    ost << "DEV   : Initial timestamp " << _ts0;
    LOG_DEV(ost.str().c_str());

    switch (rbs) {
    case 6:
        fine_timing_offset = timing_offset_rb6;
        break;
    case 15:
        fine_timing_offset = timing_offset_rb15;
        break;
    case 25:
        fine_timing_offset = timing_offset_rb25;
        break;
    case 50:
        fine_timing_offset = timing_offset_rb50;
        break;
    case 75:
        fine_timing_offset = timing_offset_rb75;
        break;
    case 100:
        fine_timing_offset = timing_offset_rb100;
        break;
    default:
        ost << "DEV   : Invalid resource block " << rbs;
        LOG_DEV(ost.str().c_str());
        return -1;
    }

    if (isFile()) {
        _freqCorr = SignalVector(lte_subframe_len(rbs));
        generateFreqOffset();
    }

    return true;
}

template <typename T>
bool IOInterface<T>::isFile() const
{
    return dynamic_pointer_cast<FileDevice<T>>(_device) != nullptr;
}

template <typename T>
int IOInterface<T>::comp_timing_offset(int coarse, int fine, int state)
{
    int adjust = 0;
    int pssOffset = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;

    if (fine == 9999)
        return -1;

    if (fine && ((coarse == 0) || (coarse == 1))) {
        fine += 32;
        adjust = fine_timing_offset(coarse, fine);
    } else if ((coarse >= -5) && (coarse <= 5)) {
        if (!state)
            adjust = coarse / 2;
        else
            adjust = coarse * _pssTimingAdjust;
    } else if (coarse) {
        adjust = (coarse - pssOffset) * _pssTimingAdjust;
    }

    return adjust;
}

template <typename T>
void IOInterface<T>::start()
{
    _device->start();
}

template <typename T>
void IOInterface<T>::stop()
{
    if (_device != nullptr) _device->stop();
}

template <typename T>
int IOInterface<T>::getBuffer(vector<vector<T>> &bufs,
                              unsigned frameNum, int coarse,
                              int fine, int state)
{
    int shift = comp_timing_offset(coarse, fine, state);
    _ts0 += shift;

    frameNum = frameNum % _frameMod;
    if (frameNum <= _prevFrameNum)
        _ts0 += _frameMod * _frameSize;

    int64_t ts = _ts0 + frameNum * _frameSize;

    while (ts + _frameSize > _device->get_ts_high())
        _device->reload();

    if (_device->pull(bufs, _frameSize, ts) < 0) {
        LOG_DEV_ERR("DEV   : Subframe I/O error");
        throw runtime_error("");
    }

    _prevFrameNum = frameNum;
    if (isFile()) applyFreqOffset(bufs);
    return shift;
}

template <typename T>
void IOInterface<T>::setFreq(double freq)
{
    _freq = freq;
    _device->setFreq(freq);
    if (isFile()) {
        _offset = 0.0;
        generateFreqOffset();
    }
}

template <typename T>
double IOInterface<T>::setGain(double gain)
{
    _gain = _device->setGain(gain);
    return _gain;
}

template <typename T>
double IOInterface<T>::getFreq()
{
    return _freq;
}

template <typename T>
double IOInterface<T>::getGain()
{
    return _gain;
}

template <typename T>
void IOInterface<T>::shiftFreq(double freq)
{
    _device->shiftFreq(freq);
    if (isFile()) {
        _offset += freq;
        generateFreqOffset();
    }
}

template <typename T>
void IOInterface<T>::resetFreq()
{
    _device->resetFreq();
    if (isFile()) {
        _offset = 0.0;
        generateFreqOffset();
    }
}

template <typename T>
void IOInterface<T>::reset()
{
    _device->reset();

    _ts0 = 0;
    _prevFrameNum = 0;
    _frameSize = 0;
}

template <typename T>
void IOInterface<T>::generateFreqOffset()
{
    int n = 0;
    for (auto &samp:_freqCorr) {
        samp = complex<float>(sinf((float) n * _offset*2*M_PI/(LTE_RATE)),
                              cosf((float) n * _offset*2*M_PI/(LTE_RATE)));
        n++;
    }
}

template <typename T>
void IOInterface<T>::applyFreqOffset(vector<vector<T>> &bufs)
{
    for (auto &buf:bufs) {
        auto it = buf.begin();
        for (auto n:_freqCorr)
            *it++ *= n;
    }
}


template class IOInterface<complex<short>>;
template class IOInterface<complex<float>>;
