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

void SynchronizerPDSCH::handleFreqOffset(double offset)
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
        shiftFreq(average);
    }
}

/*
 * PDSCH drive sequence
 */
int SynchronizerPDSCH::drive(int adjust)
{
    struct lte_time *time = &_rx->time;
    static struct lte_mib mib;

    time->subframe = (time->subframe + 1) % 10;
    if (!time->subframe)
        time->frame = (time->frame + 1) % 1024;

    Synchronizer::drive(time, adjust);

    switch (_rx->state) {
    case LTE_STATE_PBCH:
        if (timePBCH(time)) {
            if (decodePBCH(time, &mib)) {
                lte_log_time(time);
                if (mib.rbs != _rbs) {
                    _rbs = mib.rbs;
                    reopen(_rbs);
                    changeState(LTE_STATE_PSS_SYNC);
                } else {
                    changeState(LTE_STATE_PDSCH_SYNC);
                    _pssMisses = 0;
                }
                _pssMisses = 0;
            } else if (++_pssMisses > 10) {
                resetState();
            }
        }
        break;
    case LTE_STATE_PDSCH_SYNC:
        /* SSS must match so we only check timing/frequency on 0 */
        if (time->subframe == 5) {
            if (syncPSS4() == StatePSS::NotFound && _pssMisses > 100) {
                resetState();
                break;
            }
        }
    case LTE_STATE_PDSCH:
        if (timePDSCH(time)) {
            auto lbuf = _inboundQueue->readNoBlock();
            if (!lbuf) {
                LOG_ERR("SYNC  : Dropped frame");
                break;
            }

            handleFreqOffset(lbuf->freqOffset);

            if (lbuf->crcValid) {
                _pssMisses = 0;
                _sssMisses = 0;
                lbuf->crcValid = false;
            }

            lbuf->rbs = mib.rbs;
            lbuf->cellId = _cellId;
            lbuf->ng = mib.phich_ng;
            lbuf->txAntennas = mib.ant;
            lbuf->sfn = time->subframe;
            lbuf->fn = time->frame;

            _converter.delayPDSCH(lbuf->buffers, adjust);
            _outboundQueue->write(lbuf);
        }
    }

    _converter.update();

    return 0;
}

/*
 * PDSCH synchronizer loop 
 */
void SynchronizerPDSCH::start()
{
    _stop = false;
    IOInterface<complex<short>>::start();

    for (int counter = 0;; counter++) {
        int shift = getBuffer(_converter.raw(), counter,
                              _rx->sync.coarse,
                              _rx->sync.fine,
                              _rx->state == LTE_STATE_PDSCH_SYNC);
        _rx->sync.coarse = 0;
        _rx->sync.fine = 0;

        if (drive(shift) < 0) {
            fprintf(stderr, "Drive: Fatal error\n");
            break;
        }

        _converter.reset();
        if (_reset) resetState();
        if (_stop) break;
    }
}

void SynchronizerPDSCH::attachInboundQueue(shared_ptr<BufferQueue> q)
{
    _inboundQueue = q;
}

void SynchronizerPDSCH::attachOutboundQueue(shared_ptr<BufferQueue> q)
{
    _outboundQueue = q;
}

SynchronizerPDSCH::SynchronizerPDSCH(size_t chans)
  : Synchronizer(chans), _freqOffsets(200)
{
}
