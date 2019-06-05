#ifndef _SYNCHRONIZER_PDSCH_
#define _SYNCHRONIZER_PDSCH_

#include "Synchronizer.h"
#include "BufferQueue.h"
#include "FreqAverager.h"

template <typename T>
class SynchronizerPDSCH : public Synchronizer<T> {
public:
    SynchronizerPDSCH(size_t chans = 1);

    void attachInboundQueue(std::shared_ptr<BufferQueue> q);
    void attachOutboundQueue(std::shared_ptr<BufferQueue> q);

    void start();

private:
    void drive(int adjust);
    void handleFreqOffset(double offset);

    std::shared_ptr<BufferQueue> _inboundQueue;
    std::shared_ptr<BufferQueue> _outboundQueue;

    FreqAverager _freqOffsets;
};
#endif /* _SYNCHRONIZER_PDSCH_ */
