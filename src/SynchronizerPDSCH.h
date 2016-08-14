#ifndef _SYNCHRONIZER_PDSCH_
#define _SYNCHRONIZER_PDSCH_

#include "Synchronizer.h"
#include "BufferQueue.h"
#include "FreqAverager.h"

using namespace std;

class SynchronizerPDSCH : public Synchronizer{
public:
    SynchronizerPDSCH(size_t chans = 1);

    void attachInboundQueue(shared_ptr<BufferQueue> q);
    void attachOutboundQueue(shared_ptr<BufferQueue> q);

    void start();

private:
    int drive(int adjust);
    void handleFreqOffset(double offset);

    shared_ptr<BufferQueue> _inboundQueue;
    shared_ptr<BufferQueue> _outboundQueue;

    FreqAverager _freqOffsets;
};
#endif /* _SYNCHRONIZER_PDSCH_ */
