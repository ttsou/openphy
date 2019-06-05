#ifndef _SYNCHRONIZER_PBCH_
#define _SYNCHRONIZER_PBCH_

#include "Synchronizer.h"

template <typename T>
class SynchronizerPBCH : public Synchronizer<T> {
public:
    SynchronizerPBCH(size_t chans = 1);

    void start();
    virtual void reset();
    int numRB() const { return _mibDecodeRB; }

private:
    void drive();

    bool _mibValid;
    int _mibDecodeRB;
};
#endif /* _SYNCHRONIZER_PBCH_ */
