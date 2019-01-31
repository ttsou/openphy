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

#include <complex>
#include "Synchronizer.h"

extern "C" {
#include "lte/lte.h"
#include "lte/ref.h"
#include "lte/sync.h"
#include "lte/pbch.h"
#include "lte/slot.h"
#include "lte/si.h"
#include "lte/subframe.h"
#include "lte/ofdm.h"
#include "lte/log.h"
}

using namespace std;

/* Log PSS detection magnitude */
void Synchronizer::logPSS(float mag, int offset)
{
    char sbuf[80];
    snprintf(sbuf, 80, "PSS   : PSS detected, "
         "Magnitude %f, Timing offset %i", mag, offset);
    LOG_SYNC(sbuf);
}

/* Log SSS frequency offset */
void Synchronizer::logSSS(float offset)
{
    char sbuf[80];
    snprintf(sbuf, 80, "SSS   : "
         "Frequency offset %f Hz", offset);
    LOG_SYNC(sbuf);
}

Synchronizer::Synchronizer(size_t chans)
  : IOInterface<complex<short>>(chans),
    _rx(nullptr), _converter(chans), _pbchRefMaps(2)
{
    _stateStrings = decltype(_stateStrings) {
        { LTE_STATE_PSS_SYNC,    "PSS-Sync0" },
        { LTE_STATE_PSS_SYNC2,   "PSS-Sync1" },
        { LTE_STATE_SSS_SYNC,    "SSS-Sync" },
        { LTE_STATE_PBCH_SYNC,   "PBCH-Sync" },
        { LTE_STATE_PBCH,        "PBCH-Decode" },
        { LTE_STATE_PDSCH_SYNC,  "PDSCH-Sync" },
        { LTE_STATE_PDSCH,       "PDSCH-Decode" },
    };
}

Synchronizer::~Synchronizer()
{
    lte_free(_rx);

    for (auto &p : _pbchRefMaps) {
        lte_free_ref_map(p[0]);
        lte_free_ref_map(p[1]);
        lte_free_ref_map(p[2]);
        lte_free_ref_map(p[3]);
    }
}

bool Synchronizer::reopen(size_t rbs)
{
    IOInterface<complex<short>>::stop();
    if (!IOInterface<complex<short>>::open(rbs))
        return false;

    lte_free(_rx);

    _rx = lte_init();
    _rx->state = LTE_STATE_PSS_SYNC;
    _rx->last_state = LTE_STATE_PSS_SYNC;
    _rx->rbs;
    _cellId = -1;
    _pssMisses = 0;
    _sssMisses = 0;
    _reset = false;

    _converter.init(rbs);

    setFreq(_freq);
    setGain(_gain);

    IOInterface<complex<short>>::start();
    return true;
}

bool Synchronizer::open(size_t rbs, int ref, const std::string &args)
{
    if (!IOInterface<complex<short>>::open(rbs, ref, args))
        return false;

    lte_free(_rx);

    _rx = lte_init();
    _rx->state = LTE_STATE_PSS_SYNC;
    _rx->last_state = LTE_STATE_PSS_SYNC;
    _rx->rbs;
    _cellId = -1;
    _pssMisses = 0;
    _sssMisses = 0;
    _reset = false;

    _converter.init(rbs);
    return true;
}

bool Synchronizer::timePSS(struct lte_time *t)
{
    if (t->subframe == 0 || t->subframe == 5) return true;
    else return false;
}

bool Synchronizer::timeSSS(struct lte_time *t)
{
    return timePSS(t);
}

bool Synchronizer::timePBCH(struct lte_time *t)
{
    return !t->subframe ? true : false;
}

bool Synchronizer::timePDSCH(struct lte_time *t)
{
    return true;
}

void Synchronizer::setFreq(double freq)
{
    _freq = freq;
    IOInterface<complex<short>>::setFreq(freq);
}

void Synchronizer::setGain(double gain)
{
    _gain = IOInterface<complex<short>>::setGain(gain);
}

/*
 * Stage 1 PSS synchronizer
 */
Synchronizer::StatePSS Synchronizer::syncPSS1()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;

    _converter.convertPSS();
    struct cxvec *bufs[_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_search(_rx, bufs, _chans, &_sync);
    if (_sync.mag > 900) {
        if (_sync.coarse < target)
            _sync.coarse += LTE_N0_SLOT_LEN * 10;

        _rx->sync.coarse = _sync.coarse;
        _rx->time.slot = 0;
        _rx->time.subframe = 0;
        _rx->sync.n_id_2 = _sync.n_id_2;

        return StatePSS::Found;
    }
    return StatePSS::NotFound;
}

/*
 * Stage 2 PSS synchronizer
 */
Synchronizer::StatePSS Synchronizer::syncPSS2()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;
    int confidence = 2;

    _converter.convertPSS();
    struct cxvec *bufs[_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    int rc = lte_pss_detect(_rx, bufs, _chans);
    if (rc != _rx->sync.n_id_2) {
        confidence--;
        LOG_PSS("Frequency domain detection failed");
    }

    lte_pss_sync(_rx, bufs, _chans, &_sync, _rx->sync.n_id_2);

    if ((_sync.coarse > min) && (_sync.coarse < max)) {
        _rx->sync.coarse = _sync.coarse - target;
        logPSS(_sync.mag, _sync.coarse);
    } else {
        confidence--;
        LOG_PSS("Time domain detection failed");
    }

    return confidence > 0 ? StatePSS::Found : StatePSS::NotFound;
}

/*
 * Stage 3 PSS synchronizer
 */
Synchronizer::StatePSS Synchronizer::syncPSS3()
{
    /* Why is this different from the PDSCH case? */
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;
    int n_id_2;
    auto state = StatePSS::NotFound;

    _converter.convertPSS();
    struct cxvec *bufs[_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_sync(_rx, bufs, _chans, &_sync, _rx->sync.n_id_2);
    logPSS(_sync.mag, _sync.coarse);

    n_id_2 = lte_pss_detect(_rx, bufs, _chans);
    if ((_sync.coarse > min) && (_sync.coarse < max)) {
        if (n_id_2 == _rx->sync.n_id_2) state = StatePSS::Found;
        else _pssMisses += 10;
    }

    if (state == StatePSS::NotFound) {
        LOG_PSS("PSS detection failed");
        _pssMisses++;
    } else {
        _rx->sync.coarse = _sync.coarse - target;
    }

    return state;
}

/*
 * Stage 4 PSS synchronizer
 */
Synchronizer::StatePSS Synchronizer::syncPSS4()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;

    _converter.convertPSS();
    struct cxvec *bufs[_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_fine_sync(_rx, bufs, _chans, &_sync, _rx->sync.n_id_2);

    if ((_sync.coarse <= min) || (_sync.coarse >= max)) {
        _pssMisses++;
        return StatePSS::NotFound;
    }

    _rx->sync.coarse = _sync.coarse - target;
    _rx->sync.fine = _sync.fine - 32;

    if (lte_pss_detect3(_rx, bufs, _chans) < 0) {
        _pssMisses++;
        return StatePSS::NotFound;
    }

    return StatePSS::Found;
}

/*
 * SSS synchronizer
 */
Synchronizer::StateSSS Synchronizer::syncSSS()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;

    _converter.convertPSS();
    struct cxvec *bufs[_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_sync(_rx, bufs, _chans, &_sync, _rx->sync.n_id_2);

    if (_sync.coarse > min && _sync.coarse < max)
        _rx->sync.coarse = _sync.coarse - target;
    else
        _pssMisses++;

    if (lte_pss_detect(_rx, bufs, _chans) != _rx->sync.n_id_2) {
        LOG_PSS("Frequency domain detection failed");
        _pssMisses++;
    }

    int rc = lte_sss_detect(_rx, _rx->sync.n_id_2, bufs, _chans, &_sync);
    if (rc > 0)
        return StateSSS::Found;
    else if (rc == 0)
        return StateSSS::Searching;

    LOG_SSS("No matching sequence found");
    _sssMisses++;
    return StateSSS::NotFound;
}

/*
 * PBCH MIB Decoder
 */
bool Synchronizer::decodePBCH(struct lte_time *time, struct lte_mib *mib)
{
    struct lte_subframe *lsub[_chans];

    int i = 0;
    for (auto &l : lsub) {
        l = lte_subframe_alloc(6, _cellId, 2, _pbchRefMaps[0], _pbchRefMaps[1]);
        SignalVector s(l->samples);
        _converter.convertPBCH(i++, s);
    }

    int rc = lte_decode_pbch(mib, lsub, _chans);
    if (rc < 0) {
        LOG_PBCH_ERR("Internal error");
    } else if (rc == 0) {
        LOG_PBCH("MIB decoding failed");
    } else {
        time->frame = mib->fn;
    }

    for (auto &l : lsub) lte_subframe_free(l);

    return rc > 0;
}

/*
 * Base drive sequence includes PSS synchronization stages 1-3
 */
void Synchronizer::drive(struct lte_time *time)
{
    switch (_rx->state) {
    case LTE_STATE_PSS_SYNC:
        if (syncPSS1() == StatePSS::Found) {
            lte_log_time(time);
            logPSS(_sync.mag, _sync.coarse);
            changeState(LTE_STATE_PSS_SYNC2);
        } else {
            _rx->sync.fine = 9999;
        }
        break;
    case LTE_STATE_PSS_SYNC2:
        if (!time->subframe) {
            lte_log_time(time);
            if (syncPSS2() == StatePSS::Found)
                changeState(LTE_STATE_SSS_SYNC);
            else
                changeState(LTE_STATE_PSS_SYNC);
        }
        break;
    case LTE_STATE_SSS_SYNC:
        if (!time->subframe) {
            if (syncSSS() == StateSSS::Found) {
                 shiftFreq(_sync.f_offset);
                 time->subframe = _sync.dn;

                 _rx->sync.n_id_1 = _sync.n_id_1;
                 _rx->sync.n_id_cell = _sync.n_id_cell;

                 if (_cellId != _sync.n_id_cell)
                     setCellId(_sync.n_id_cell);

                 lte_log_time(time);
                 changeState(LTE_STATE_PBCH_SYNC);
            } else if (_pssMisses >= 4) {
                resetState();
            }
        }
        break;
    case LTE_STATE_PBCH_SYNC:
        if (!time->subframe) {
            if (syncPSS3() == StatePSS::Found) {
                lte_log_time(time);
                changeState(LTE_STATE_PBCH);
            } else if (_pssMisses > 20) {
                resetState();
                break;
            }
        }
    }
}

void Synchronizer::resetState(ResetFreq r)
{
    _pssMisses = 0;
    _sssMisses = 0;
    _reset = false;

    if (r == ResetFreq::True) resetFreq();
    changeState(LTE_STATE_PSS_SYNC);
}

void Synchronizer::reset()
{
    _reset = true;
}

void Synchronizer::stop()
{
    _stop = true;
    sleep(1);
    IOInterface<complex<short>>::stop();
}

void Synchronizer::changeState(lte_state newState)
{
    auto current = _rx->state;
    char sbuf[80];
    snprintf(sbuf, 80, "STATE : State change from %s to %s",
             _stateStrings.find(current)->second.c_str(),
             _stateStrings.find(newState)->second.c_str());
    LOG_APP(sbuf);

    _rx->state = newState;
}

void Synchronizer::generateReferences()
{
    for (auto &p : _pbchRefMaps) {
        lte_free_ref_map(p[0]);
        lte_free_ref_map(p[1]);
        lte_free_ref_map(p[2]);
        lte_free_ref_map(p[3]);
    }

    _pbchRefMaps[0][0] = lte_gen_ref_map(_cellId, 0, 0, 0, 6);
    _pbchRefMaps[0][1] = lte_gen_ref_map(_cellId, 1, 0, 0, 6);
    _pbchRefMaps[0][2] = lte_gen_ref_map(_cellId, 0, 0, 4, 6);
    _pbchRefMaps[0][3] = lte_gen_ref_map(_cellId, 1, 0, 4, 6);

    _pbchRefMaps[1][0] = lte_gen_ref_map(_cellId, 0, 1, 0, 6);
    _pbchRefMaps[1][1] = lte_gen_ref_map(_cellId, 1, 1, 0, 6);
    _pbchRefMaps[1][2] = lte_gen_ref_map(_cellId, 0, 1, 4, 6);
    _pbchRefMaps[1][3] = lte_gen_ref_map(_cellId, 1, 1, 4, 6);
}

void Synchronizer::setCellId(int cellId)
{
    ostringstream ostr;
    ostr << "STATE : Setting cellular ID " << cellId;

    LOG_APP(ostr.str().c_str());
    _cellId = cellId;
    generateReferences();
}
