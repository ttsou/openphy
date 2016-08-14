#ifndef _SYNCHRONIZER_PBCH_
#define _SYNCHRONIZER_PBCH_

#include "Synchronizer.h"

using namespace std;

class SynchronizerPBCH : public Synchronizer {
public:
    SynchronizerPBCH(size_t chans = 1);

    void start();
    virtual void reset();
    int numRB() const { return _mibDecodeRB; }

private:
    bool drive(int adjust);

    bool _mibValid;
    int _mibDecodeRB;
};
#endif /* _SYNCHRONIZER_PBCH_ */
