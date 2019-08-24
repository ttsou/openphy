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

#include "SynchronizerPDSCH.h"

extern "C" {
#include "lte/pbch.h"
#include "lte/slot.h"
#include "lte/si.h"
#include "lte/subframe.h"
#include "lte/ofdm.h"
#include "lte/log.h"
}

using namespace std;

template <typename T>
void SynchronizerPDSCH<T>::handleFreqOffset(double offset)
{
    _freqOffsets.push(offset);

    auto log = [](float offset) {
        ostringstream ostr;
        ostr << "REF   : Frequency offset " << offset << " Hz";
        LOG_SYNC(ostr.str().c_str());
    };

    if (_freqOffsets.full()) {
        auto average = _freqOffsets.average();
        log(average);
        IOInterface<T>::shiftFreq(average);
    }
}

/*
 * PDSCH drive sequence
 */
template <typename T>
void SynchronizerPDSCH<T>::drive(int adjust)
{
    struct lte_time *time = &Synchronizer<T>::_rx->time;
    static struct lte_mib mib;

    time->subframe = (time->subframe + 1) % 10;
    if (!time->subframe)
        time->frame = (time->frame + 1) % 1024;

    Synchronizer<T>::drive(time);

    switch (Synchronizer<T>::_rx->state) {
    case LTE_STATE_PBCH:
        if (Synchronizer<T>::timePBCH(time)) {
            if (Synchronizer<T>::decodePBCH(time, &mib)) {
                lte_log_time(time);
                if (mib.rbs != IOInterface<T>::_rbs) {
                    IOInterface<T>::_rbs = mib.rbs;
                    Synchronizer<T>::reopen(IOInterface<T>::_rbs);
                    Synchronizer<T>::changeState(LTE_STATE_PSS_SYNC);
                } else {
                    Synchronizer<T>::changeState(LTE_STATE_PDSCH_SYNC);
                    Synchronizer<T>::_pssMisses = 0;
                }
                Synchronizer<T>::_pssMisses = 0;
            } else if (++Synchronizer<T>::_pssMisses > 10) {
                Synchronizer<T>::resetState();
            } else {
                Synchronizer<T>::changeState(LTE_STATE_PBCH_SYNC);
            }
        }
        break;
    case LTE_STATE_PDSCH_SYNC:
        /* SSS must match so we only check timing/frequency on 0 */
        if (time->subframe == 5) {
            if (Synchronizer<T>::syncPSS4() == SyncStatePSS::NotFound &&
                Synchronizer<T>::_pssMisses > 100) {
                Synchronizer<T>::resetState();
                break;
            }
        }
    case LTE_STATE_PDSCH:
        if (Synchronizer<T>::timePDSCH(time)) {
            auto lbuf = IOInterface<T>::isFile() ? _inboundQueue->read() :
                                                   _inboundQueue->readNoBlock();
            if (!lbuf) {
                LOG_ERR("SYNC  : Dropped frame");
                break;
            }

            handleFreqOffset(lbuf->freqOffset);

            if (lbuf->crcValid) {
                Synchronizer<T>::_pssMisses = 0;
                Synchronizer<T>::_sssMisses = 0;
                lbuf->crcValid = false;
            }

            lbuf->rbs = mib.rbs;
            lbuf->cellId = Synchronizer<T>::_cellId;
            lbuf->ng = mib.phich_ng;
            lbuf->txAntennas = mib.ant;
            lbuf->sfn = time->subframe;
            lbuf->fn = time->frame;

            Synchronizer<T>::_converter.delayPDSCH(lbuf->buffers, adjust);
            _outboundQueue->write(lbuf);
        }
    }

    Synchronizer<T>::_converter.update();
}

/*
 * PDSCH synchronizer loop 
 */
template <typename T>
void SynchronizerPDSCH<T>::start()
{
    Synchronizer<T>::_stop = false;
    IOInterface<T>::start();

    for (int counter = 0;; counter++) {
        int shift = IOInterface<T>::getBuffer(Synchronizer<T>::_converter.raw(), counter,
                                              Synchronizer<T>::_rx->sync.coarse,
                                              Synchronizer<T>::_rx->sync.fine,
                                              Synchronizer<T>::_rx->state == LTE_STATE_PDSCH_SYNC);
        Synchronizer<T>::_rx->sync.coarse = 0;
        Synchronizer<T>::_rx->sync.fine = 0;

        drive(shift);
        Synchronizer<T>::_converter.reset();

        if (Synchronizer<T>::_reset) Synchronizer<T>::resetState();
        if (Synchronizer<T>::_stop) break;
    }
}

template <typename T>
void SynchronizerPDSCH<T>::attachInboundQueue(shared_ptr<BufferQueue> q)
{
    _inboundQueue = q;
}

template <typename T>
void SynchronizerPDSCH<T>::attachOutboundQueue(shared_ptr<BufferQueue> q)
{
    _outboundQueue = q;
}

template <typename T>
SynchronizerPDSCH<T>::SynchronizerPDSCH(size_t chans)
  : Synchronizer<T>::Synchronizer(chans), _freqOffsets(200)
{
}

template class SynchronizerPDSCH<complex<short>>;
template class SynchronizerPDSCH<complex<float>>;
