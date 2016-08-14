#ifndef _DECODER_PDSCH_
#define _DECODER_PDSCH_

#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <string>

#include "BufferQueue.h"
#include "DecoderASN1.h"

using namespace std;

struct lte_ref_map;
typedef vector<int8_t> ScramSequence;

class DecoderPDSCH {
public:
    DecoderPDSCH(unsigned chans = 1);
    DecoderPDSCH(const DecoderPDSCH &d);
    DecoderPDSCH(DecoderPDSCH &&d);
    ~DecoderPDSCH();

    DecoderPDSCH &operator=(const DecoderPDSCH &d);
    DecoderPDSCH &operator=(DecoderPDSCH &&d);

    void attachInboundQueue(shared_ptr<BufferQueue> q);
    void attachOutboundQueue(shared_ptr<BufferQueue> q);
    void attachDecoderASN1(shared_ptr<DecoderASN1> d);

    bool addRNTI(unsigned rnti, string s = "");
    bool delRNTI(unsigned rnti);

    void start();

private:
    void setCellId(unsigned cellId, unsigned rbs,
                   unsigned ng, unsigned txAntennas);
    void generateSequences();
    void generateReferences();
    void initSubframes();

    void readBufferState(shared_ptr<LteBuffer> lbuf);
    void setFreqOffset(shared_ptr<LteBuffer> lbuf);
    void decode(shared_ptr<LteBuffer> lbuf, int cfi);

    vector<ScramSequence> _pdcchScramSeq;
    vector<ScramSequence> _pcfichScramSeq;
    map<unsigned, string> _rntis;

    bool _cellIdValid;
    unsigned _cellId, _rbs, _ng, _txAntennas;

    shared_ptr<BufferQueue> _inboundQueue, _outboundQueue;
    shared_ptr<DecoderASN1> _decoderASN1;

    struct lte_pdsch_blk *_block;
    vector<struct lte_subframe *> _subframes;
    vector<struct lte_ref_map *[4]> _pdcchRefMaps;
};

#endif /* _DECODER_PDSCH_ */
