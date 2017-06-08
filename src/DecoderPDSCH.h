#ifndef _DECODER_PDSCH_
#define _DECODER_PDSCH_

#include <memory>
#include <vector>
#include <queue>
#include <map>
#include <string>

#include "BufferQueue.h"
#include "DecoderASN1.h"

struct lte_ref_map;
typedef std::vector<int8_t> ScramSequence;

class DecoderPDSCH {
public:
    DecoderPDSCH(unsigned chans = 1);
    DecoderPDSCH(const DecoderPDSCH &d);
    DecoderPDSCH(DecoderPDSCH &&d);
    ~DecoderPDSCH();

    DecoderPDSCH &operator=(const DecoderPDSCH &d);
    DecoderPDSCH &operator=(DecoderPDSCH &&d);

    void attachInboundQueue(std::shared_ptr<BufferQueue> q);
    void attachOutboundQueue(std::shared_ptr<BufferQueue> q);
    void attachDecoderASN1(std::shared_ptr<DecoderASN1> d);

    bool addRNTI(unsigned rnti, std::string s = "");
    bool delRNTI(unsigned rnti);

    void start();

private:
    void setCellId(unsigned cellId, unsigned rbs,
                   unsigned ng, unsigned txAntennas);
    void generateSequences();
    void generateReferences();
    void initSubframes();

    void readBufferState(std::shared_ptr<LteBuffer> lbuf);
    void setFreqOffset(std::shared_ptr<LteBuffer> lbuf);
    void decode(std::shared_ptr<LteBuffer> lbuf, int cfi);

    std::vector<ScramSequence> _pdcchScramSeq;
    std::vector<ScramSequence> _pcfichScramSeq;
    std::map<unsigned, std::string> _rntis;

    bool _cellIdValid;
    unsigned _cellId, _rbs, _ng, _txAntennas;

    std::shared_ptr<BufferQueue> _inboundQueue, _outboundQueue;
    std::shared_ptr<DecoderASN1> _decoderASN1;

    struct lte_pdsch_blk *_block;
    std::vector<struct lte_subframe *> _subframes;
    std::vector<struct lte_ref_map *[4]> _pdcchRefMaps;
};

#endif /* _DECODER_PDSCH_ */
