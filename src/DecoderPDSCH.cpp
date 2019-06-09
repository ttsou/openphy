/*
 * LTE PDSCH Decoder
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

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex>

#include "DecoderPDSCH.h"

extern "C" {
#include "lte/lte.h"
#include "lte/pdcch.h"
#include "lte/ref.h"
#include "lte/pcfich.h"
#include "lte/pdsch.h"
#include "lte/ofdm.h"
#include "lte/scramble.h"
#include "lte/subframe.h"
#include "lte/log.h"
#include "lte/pdsch_block.h"
#include "dsp/sigvec.h"
}

#define LTE_PDCCH_MAX_BITS        6269

using namespace std;

void DecoderPDSCH::generateReferences()
{
    int i = 0;
    for (auto &p : _pdcchRefMaps) {
        lte_free_ref_map(p[0]);
        lte_free_ref_map(p[1]);
        lte_free_ref_map(p[2]);
        lte_free_ref_map(p[3]);

        p[0] = lte_gen_ref_map(_cellId, 0, i, 0, _rbs);
        p[1] = lte_gen_ref_map(_cellId, 1, i, 0, _rbs);
        p[2] = lte_gen_ref_map(_cellId, 0, i, 4, _rbs);
        p[3] = lte_gen_ref_map(_cellId, 1, i, 4, _rbs);

        i++;
    }
}

bool DecoderPDSCH::addRNTI(unsigned rnti, string s)
{
    auto r = _rntis.insert(pair<unsigned, string>(rnti, s));
    return r.second;
}

bool DecoderPDSCH::delRNTI(unsigned rnti)
{
    return _rntis.erase(rnti);
}

void DecoderPDSCH::generateSequences()
{
    unsigned c_init;

    int i = 0;
    for (auto &p : _pcfichScramSeq) {
        c_init = (2 * i++ / 2 + 1) * (2 * _cellId + 1) * (1 << 9) + _cellId;
        lte_pbch_gen_scrambler(c_init, p.data(), p.size());
    }

    i = 0;
    for (auto &p : _pdcchScramSeq) {
        c_init = (2 * i++ / 2) * (1 << 9) + _cellId;
        lte_pdcch_gen_scrambler(c_init, p.data(), p.size());
    }
}

void DecoderPDSCH::initSubframes()
{
    auto &m1 = _pdcchRefMaps.front();
    auto &m2 = _pdcchRefMaps.front();

    for (auto &s : _subframes) {
        lte_subframe_free(s);
        s = lte_subframe_alloc(_rbs, _cellId, _txAntennas, m1, m2);
    }
}

void DecoderPDSCH::readBufferState(shared_ptr<LteBuffer> lbuf)
{
    auto &m1 = _pdcchRefMaps[lbuf->sfn * 2 + 0];
    auto &m2 = _pdcchRefMaps[lbuf->sfn * 2 + 1];

    /*
     * Reinitialization on cellid change
     */
    bool idChange = !_cellIdValid ||
                    lbuf->cellId     != _cellId ||
                    lbuf->txAntennas != _txAntennas ||
                    lbuf->rbs        != _rbs ||
                    lbuf->ng         != _ng;

    if (idChange)
        setCellId(lbuf->cellId, lbuf->rbs, lbuf->ng, lbuf->txAntennas);
    else
        for (auto &s : _subframes) lte_subframe_reset(s, m1, m2);

    /* Allow multi-channel decoder with single channel sample input */
    auto convert = [](vector<complex<float>> &buf, struct cxvec *vec) {
        copy(begin(buf), end(buf), (complex<float> *) cxvec_data(vec));
    };

    auto b = begin(lbuf->buffers);
    for (auto &s : _subframes) {
        if (b != end(lbuf->buffers)) convert(*b++, s->samples);
        else break;
    }

    /* Set initial return values back to the synchronizer */
    lbuf->freqOffset = 0.0;
    lbuf->crcValid = false;
}

void DecoderPDSCH::decode(shared_ptr<LteBuffer> lbuf, int cfi)
{
    auto scramSeq = _pdcchScramSeq[lbuf->sfn];

    struct lte_time t {
        .frame = lbuf->fn,
        .subframe = lbuf->sfn,
    };

    for (auto &r : _rntis) {
        auto ndci = lte_decode_pdcch(_subframes.data(),
                                     _subframes.size(),
                                     cfi,
                                     _cellId,
                                     _ng,
                                     r.first,
                                     scramSeq.data());
        while (--ndci >= 0) {
            if (lte_decode_pdsch(_subframes.data(),
                                 _subframes.size(),
                                 _block, cfi, ndci, &t) > 0) {
                lbuf->crcValid = true;
                int len;
                auto data = (const char *) lte_pdsch_blk_abuf(_block, &len);
                _decoderASN1->send(data, len/8, r.first);
            }
        }
    }
}

void DecoderPDSCH::start()
{
    struct lte_pcfich_info info;

    if (_block == nullptr) _block = lte_pdsch_blk_alloc();

    for (;;) {
        auto lbuf = _inboundQueue->read();
        if (!lbuf)
            continue;

        readBufferState(lbuf);
        auto scramSeq = _pcfichScramSeq[lbuf->sfn];
        int rc = lte_decode_pcfich(&info,
                                   _subframes.data(),
                                   _cellId,
                                   scramSeq.data(),
                                   _subframes.size());
        if (rc > 0) decode(lbuf, info.cfi);

        setFreqOffset(lbuf);

        _outboundQueue->write(lbuf);
    }
}

void DecoderPDSCH::attachInboundQueue(shared_ptr<BufferQueue> q)
{
    _inboundQueue = q;
}

void DecoderPDSCH::attachOutboundQueue(shared_ptr<BufferQueue> q)
{
    _outboundQueue = q;
}

void DecoderPDSCH::attachDecoderASN1(shared_ptr<DecoderASN1> d)
{
    _decoderASN1 = d;
}

void DecoderPDSCH::setCellId(unsigned cellId, unsigned rbs,
                             unsigned ng, unsigned txAntennas)
{
    _ng = ng;
    _rbs = rbs;
    _cellId = cellId;
    _txAntennas = txAntennas;
    _cellIdValid = true;

    generateSequences();
    generateReferences();
    initSubframes();
}

void DecoderPDSCH::setFreqOffset(shared_ptr<LteBuffer> lbuf)
{
    auto offset = 0.0f;

    for (auto &s : _subframes)
        offset += lte_ofdm_offset(s);

    lbuf->freqOffset = offset / _subframes.size();
}

DecoderPDSCH::DecoderPDSCH(unsigned chans)
  : _pdcchScramSeq(10, ScramSequence(LTE_PDCCH_MAX_BITS)),
    _pcfichScramSeq(10, ScramSequence(32)), _pdcchRefMaps(20),
    _cellIdValid(false), _block(nullptr), _subframes(chans)
{
}

DecoderPDSCH::DecoderPDSCH(const DecoderPDSCH &d)
  : _pdcchRefMaps(20), _block(nullptr)
{
    *this = d;
}

DecoderPDSCH::DecoderPDSCH(DecoderPDSCH &&d)
  : _pdcchRefMaps(20), _block(nullptr)
{
    *this = move(d);
}

DecoderPDSCH::~DecoderPDSCH()
{
    for (auto &p : _pdcchRefMaps) {
        lte_free_ref_map(p[0]);
        lte_free_ref_map(p[1]);
        lte_free_ref_map(p[2]);
        lte_free_ref_map(p[3]);
    }

    for (auto &s : _subframes)
        lte_subframe_free(s);

    lte_pdsch_blk_free(_block);
}

DecoderPDSCH &DecoderPDSCH::operator=(const DecoderPDSCH &d)
{
    if (this != &d) {
        for (auto &s : _subframes) lte_subframe_free(s);
        lte_pdsch_blk_free(_block);

        _pdcchScramSeq = d._pdcchScramSeq;
        _pcfichScramSeq = d._pcfichScramSeq;
        _rntis = d._rntis;
        _block = nullptr;
        _cellIdValid = d._cellIdValid;
        _subframes.resize(d._subframes.size());

        if (_cellIdValid) setCellId(d._cellId, d._rbs, d._ng, d._txAntennas);
    }
    return *this;
}

DecoderPDSCH &DecoderPDSCH::operator=(DecoderPDSCH &&d)
{
    if (this != &d) {
        for (auto &s : _subframes) lte_subframe_free(s);
        lte_pdsch_blk_free(_block);

        _pdcchScramSeq = move(d._pdcchScramSeq);
        _pcfichScramSeq = move(d._pcfichScramSeq);
        _rntis = move(d._rntis);
        _block = nullptr;
        _cellIdValid = d._cellIdValid;
        _subframes = move(d._subframes);

        if (_cellIdValid) setCellId(d._cellId, d._rbs, d._ng, d._txAntennas);
    }
    return *this;
}
