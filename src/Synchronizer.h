#ifndef _SYNCHRONIZER_H_
#define _SYNCHRONIZER_H_

#include <atomic>
#include <map>
#include "IOInterface.h"
#include "Converter.h"

extern "C" {
#include "lte/lte.h"
}

struct lte_rx;
struct lte_ref_map;

using namespace std;

typedef complex<short> SampleType;

class Synchronizer : protected IOInterface<SampleType> {
public:
    Synchronizer(size_t chans = 1);
    virtual ~Synchronizer();

    Synchronizer(const Synchronizer &) = delete;
    Synchronizer &operator=(const Synchronizer &) = delete;

    bool open(size_t rbs, int ref, const std::string &args);
    bool reopen(size_t rbs);
    void reset();
    void stop();

    void setFreq(double freq);
    void setGain(double gain);

protected:
    static bool timePSS(struct lte_time *t);
    static bool timeSSS(struct lte_time *t);
    static bool timePBCH(struct lte_time *t);
    static bool timePDSCH(struct lte_time *t);

    enum class ResetFreq : bool { False, True };
    enum class StatePSS : bool { NotFound, Found };
    enum class StateSSS { Searching, NotFound, Found };

    StatePSS syncPSS1();
    StatePSS syncPSS2();
    StatePSS syncPSS3();
    StatePSS syncPSS4();

    StateSSS syncSSS();
    void drive(struct lte_time *ltime, int adjust);

    void resetState(ResetFreq r = ResetFreq::True);
    void setCellId(int cellId);
    void generateReferences();
    bool decodePBCH(struct lte_time *time, struct lte_mib *mib);

    void changeState(auto newState);
    static void logPSS(float mag, int offset);
    static void logSSS(float offset);

    Converter<SampleType> _converter;
    int _cellId, _pssMisses, _sssMisses;
    double _freq, _gain;
    atomic<bool> _reset, _stop;

    map<int, string> _stateStrings;

    /* Internal C objects */
    vector<struct lte_ref_map *[4]> _pbchRefMaps;
    struct lte_rx *_rx;
    struct lte_sync _sync;
};
#endif /* _SYNCHRONIZER_H_ */
