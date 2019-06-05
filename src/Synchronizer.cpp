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
template <typename T>
void Synchronizer<T>::logPSS(float mag, int offset)
{
    char sbuf[80];
    snprintf(sbuf, 80, "PSS   : PSS detected, "
         "Magnitude %f, Timing offset %i", mag, offset);
    LOG_SYNC(sbuf);
}

/* Log SSS frequency offset */
template <typename T>
void Synchronizer<T>::logSSS(float offset)
{
    char sbuf[80];
    snprintf(sbuf, 80, "SSS   : "
         "Frequency offset %f Hz", offset);
    LOG_SYNC(sbuf);
}

template <typename T>
Synchronizer<T>::Synchronizer(size_t chans)
  : IOInterface<T>(chans),
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

template <typename T>
Synchronizer<T>::~Synchronizer()
{
    lte_free(_rx);

    for (auto &p : _pbchRefMaps) {
        lte_free_ref_map(p[0]);
        lte_free_ref_map(p[1]);
        lte_free_ref_map(p[2]);
        lte_free_ref_map(p[3]);
    }
}

template <typename T>
bool Synchronizer<T>::reopen(size_t rbs)
{
    IOInterface<T>::stop();
    if (!IOInterface<T>::open(rbs))
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

    IOInterface<T>::start();
    return true;
}

template <typename T>
bool Synchronizer<T>::open(size_t rbs, int ref, const std::string &args)
{
    if (!IOInterface<T>::open(rbs, ref, args))
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

template <typename T>
bool Synchronizer<T>::timePSS(struct lte_time *t)
{
    if (t->subframe == 0 || t->subframe == 5) return true;
    else return false;
}

template <typename T>
bool Synchronizer<T>::timeSSS(struct lte_time *t)
{
    return timePSS(t);
}

template <typename T>
bool Synchronizer<T>::timePBCH(struct lte_time *t)
{
    return !t->subframe ? true : false;
}

template <typename T>
bool Synchronizer<T>::timePDSCH(struct lte_time *t)
{
    return true;
}

template <typename T>
void Synchronizer<T>::setFreq(double freq)
{
    _freq = freq;
    IOInterface<T>::setFreq(freq);
}

template <typename T>
void Synchronizer<T>::setGain(double gain)
{
    _gain = IOInterface<T>::setGain(gain);
}

/*
 * Stage 1 PSS synchronizer
 */
template <typename T>
SyncStatePSS Synchronizer<T>::syncPSS1()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;

    _converter.convertPSS();
    struct cxvec *bufs[IOInterface<T>::_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_search(_rx, bufs, IOInterface<T>::_chans, &_sync);
    if (_sync.mag > 900) {
        if (_sync.coarse < target)
            _sync.coarse += LTE_N0_SLOT_LEN * 10;

        _rx->sync.coarse = _sync.coarse;
        _rx->time.slot = 0;
        _rx->time.subframe = 0;
        _rx->sync.n_id_2 = _sync.n_id_2;

        return SyncStatePSS::Found;
    }
    return SyncStatePSS::NotFound;
}

/*
 * Stage 2 PSS synchronizer
 */
template <typename T>
SyncStatePSS Synchronizer<T>::syncPSS2()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;
    int confidence = 2;

    _converter.convertPSS();
    struct cxvec *bufs[IOInterface<T>::_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    int rc = lte_pss_detect(_rx, bufs, IOInterface<T>::_chans);
    if (rc != _rx->sync.n_id_2) {
        confidence--;
        LOG_PSS("Frequency domain detection failed");
    }

    lte_pss_sync(_rx, bufs, IOInterface<T>::_chans, &_sync, _rx->sync.n_id_2);

    if ((_sync.coarse > min) && (_sync.coarse < max)) {
        _rx->sync.coarse = _sync.coarse - target;
        logPSS(_sync.mag, _sync.coarse);
    } else {
        confidence--;
        LOG_PSS("Time domain detection failed");
    }

    return confidence > 0 ? SyncStatePSS::Found : SyncStatePSS::NotFound;
}

/*
 * Stage 3 PSS synchronizer
 */
template <typename T>
SyncStatePSS Synchronizer<T>::syncPSS3()
{
    /* Why is this different from the PDSCH case? */
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;
    int n_id_2;
    auto state = SyncStatePSS::NotFound;

    _converter.convertPSS();
    struct cxvec *bufs[IOInterface<T>::_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_sync(_rx, bufs, IOInterface<T>::_chans, &_sync, _rx->sync.n_id_2);
    logPSS(_sync.mag, _sync.coarse);

    n_id_2 = lte_pss_detect(_rx, bufs, IOInterface<T>::_chans);
    if ((_sync.coarse > min) && (_sync.coarse < max)) {
        if (n_id_2 == _rx->sync.n_id_2) state = SyncStatePSS::Found;
        else _pssMisses += 10;
    }

    if (state == SyncStatePSS::NotFound) {
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
template <typename T>
SyncStatePSS Synchronizer<T>::syncPSS4()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;

    _converter.convertPSS();
    struct cxvec *bufs[IOInterface<T>::_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_fine_sync(_rx, bufs, IOInterface<T>::_chans, &_sync, _rx->sync.n_id_2);

    if ((_sync.coarse <= min) || (_sync.coarse >= max)) {
        _pssMisses++;
        return SyncStatePSS::NotFound;
    }

    _rx->sync.coarse = _sync.coarse - target;
    _rx->sync.fine = _sync.fine - 32;

    if (lte_pss_detect3(_rx, bufs, IOInterface<T>::_chans) < 0) {
        _pssMisses++;
        return SyncStatePSS::NotFound;
    }

    return SyncStatePSS::Found;
}

/*
 * SSS synchronizer
 */
template <typename T>
SyncStateSSS Synchronizer<T>::syncSSS()
{
    int target = LTE_N0_SLOT_LEN - LTE_N0_CP0_LEN - 1;
    int min = target - 4;
    int max = target + 4;

    _converter.convertPSS();
    struct cxvec *bufs[IOInterface<T>::_chans];
    SignalVector::translateVectors(_converter.pss(), bufs);

    lte_pss_sync(_rx, bufs, IOInterface<T>::_chans, &_sync, _rx->sync.n_id_2);

    if (_sync.coarse > min && _sync.coarse < max)
        _rx->sync.coarse = _sync.coarse - target;
    else
        _pssMisses++;

    if (lte_pss_detect(_rx, bufs, IOInterface<T>::_chans) != _rx->sync.n_id_2) {
        LOG_PSS("Frequency domain detection failed");
        _pssMisses++;
    }

    int rc = lte_sss_detect(_rx, _rx->sync.n_id_2, bufs, IOInterface<T>::_chans, &_sync);
    if (rc > 0)
        return SyncStateSSS::Found;
    else if (rc == 0)
        return SyncStateSSS::Searching;

    LOG_SSS("No matching sequence found");
    _sssMisses++;
    return SyncStateSSS::NotFound;
}

/*
 * PBCH MIB Decoder
 */
template <typename T>
bool Synchronizer<T>::decodePBCH(struct lte_time *time, struct lte_mib *mib)
{
    struct lte_subframe *lsub[IOInterface<T>::_chans];

    int i = 0;
    for (auto &l : lsub) {
        l = lte_subframe_alloc(6, _cellId, 2, _pbchRefMaps[0], _pbchRefMaps[1]);
        SignalVector s(l->samples);
        _converter.convertPBCH(i++, s);
    }

    int rc = lte_decode_pbch(mib, lsub, IOInterface<T>::_chans);
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
template <typename T>
void Synchronizer<T>::drive(struct lte_time *time)
{
    switch (_rx->state) {
    case LTE_STATE_PSS_SYNC:
        if (syncPSS1() == SyncStatePSS::Found) {
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
            if (syncPSS2() == SyncStatePSS::Found)
                changeState(LTE_STATE_SSS_SYNC);
            else
                changeState(LTE_STATE_PSS_SYNC);
        }
        break;
    case LTE_STATE_SSS_SYNC:
        if (!time->subframe) {
            if (syncSSS() == SyncStateSSS::Found) {
                 IOInterface<T>::shiftFreq(_sync.f_offset);
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
            if (syncPSS3() == SyncStatePSS::Found) {
                lte_log_time(time);
                changeState(LTE_STATE_PBCH);
            } else if (_pssMisses > 20) {
                resetState();
                break;
            }
        }
    }
}

template <typename T>
void Synchronizer<T>::resetState(SyncResetFreq r)
{
    _pssMisses = 0;
    _sssMisses = 0;
    _reset = false;

    if (r == SyncResetFreq::True) IOInterface<T>::resetFreq();
    changeState(LTE_STATE_PSS_SYNC);
}

template <typename T>
void Synchronizer<T>::reset()
{
    _reset = true;
}

template <typename T>
void Synchronizer<T>::stop()
{
    _stop = true;
    sleep(1);
    IOInterface<T>::stop();
}

template <typename T>
void Synchronizer<T>::changeState(lte_state newState)
{
    auto current = _rx->state;
    char sbuf[80];
    snprintf(sbuf, 80, "STATE : State change from %s to %s",
             _stateStrings.find(current)->second.c_str(),
             _stateStrings.find(newState)->second.c_str());
    LOG_APP(sbuf);

    _rx->state = newState;
}

template <typename T>
void Synchronizer<T>::generateReferences()
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

template <typename T>
void Synchronizer<T>::setCellId(int cellId)
{
    ostringstream ostr;
    ostr << "STATE : Setting cellular ID " << cellId;

    LOG_APP(ostr.str().c_str());
    _cellId = cellId;
    generateReferences();
}

template class Synchronizer<complex<short>>;
template class Synchronizer<complex<float>>;
