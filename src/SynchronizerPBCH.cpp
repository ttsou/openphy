/*
 * LTE Synchronizer
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

#include "SynchronizerPBCH.h"

extern "C" {
#include "lte/pbch.h"
#include "lte/slot.h"
#include "lte/si.h"
#include "lte/subframe.h"
#include "lte/ofdm.h"
#include "lte/log.h"
}

using namespace std;

/*
 * PBCH drive sequence
 */
template <typename T>
void SynchronizerPBCH<T>::drive()
{
    struct lte_time *ltime = &Synchronizer<T>::_rx->time;
    struct lte_mib mib;
    bool mibValid = false;

    ltime->subframe = (ltime->subframe + 1) % 10;
    if (!ltime->subframe)
        ltime->frame = (ltime->frame + 1) % 1024;

    Synchronizer<T>::drive(ltime);

    auto logFreq = [](auto freq) {
        ostringstream ost;
        ost << "PBCH  : RF Frequency " << freq / 1e6 << " MHz";
        LOG_CTRL(ost.str().c_str());
    };

    switch (Synchronizer<T>::_rx->state) {
    case LTE_STATE_PBCH:
        if (Synchronizer<T>::timePBCH(ltime)) {
            if (Synchronizer<T>::decodePBCH(ltime, &mib)) {
                Synchronizer<T>::_sssMisses = 0;
                Synchronizer<T>::_pssMisses = 0;
                _mibDecodeRB = mib.rbs;
                _mibValid = true;
                logFreq(IOInterface<T>::getFreq());
            } else if (++Synchronizer<T>::_pssMisses > 20) {
                Synchronizer<T>::_reset = true;
            }
        }
        Synchronizer<T>::changeState(LTE_STATE_PBCH_SYNC);
    }

    Synchronizer<T>::_converter.update();
}

/*
 * PBCH synchronizer loop 
 */
template <typename T>
void SynchronizerPBCH<T>::start()
{
    Synchronizer<T>::_stop = false;
    IOInterface<T>::start();

    for (int counter = 0;; counter++) {
        IOInterface<T>::getBuffer(Synchronizer<T>::_converter.raw(), counter,
                                  Synchronizer<T>::_rx->sync.coarse,
                                  Synchronizer<T>::_rx->sync.fine, 0);
        Synchronizer<T>::_rx->sync.coarse = 0;
        Synchronizer<T>::_rx->sync.fine = 0;

        if (!_mibValid)
            drive();

        Synchronizer<T>::_converter.reset();

        if (Synchronizer<T>::_reset) Synchronizer<T>::resetState(SyncResetFreq::False);
        if (Synchronizer<T>::_stop) break;
   }
}

template <typename T>
void SynchronizerPBCH<T>::reset()
{
    _mibValid = false;
    Synchronizer<T>::reset();
}

template <typename T>
SynchronizerPBCH<T>::SynchronizerPBCH(size_t chans)
  : Synchronizer<T>(chans), _mibValid(false), _mibDecodeRB(0)
{
}

template class SynchronizerPBCH<complex<short>>;
template class SynchronizerPBCH<complex<float>>;
