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

enum class SyncResetFreq : bool { False, True };
enum class SyncStatePSS : bool { NotFound, Found };
enum class SyncStateSSS { Searching, NotFound, Found };

template <typename T>
class Synchronizer : protected IOInterface<T> {
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
    bool open(size_t rbs);

    static bool timePSS(struct lte_time *t);
    static bool timeSSS(struct lte_time *t);
    static bool timePBCH(struct lte_time *t);
    static bool timePDSCH(struct lte_time *t);

    SyncStatePSS syncPSS1();
    SyncStatePSS syncPSS2();
    SyncStatePSS syncPSS3();
    SyncStatePSS syncPSS4();
    SyncStateSSS syncSSS();

    void drive(struct lte_time *ltime);

    void resetState(SyncResetFreq r = SyncResetFreq::True);
    void setCellId(int cellId);
    void generateReferences();
    bool decodePBCH(struct lte_time *time, struct lte_mib *mib);

    void changeState(lte_state newState);
    static void logPSS(float mag, int offset);
    static void logSSS(float offset);

    Converter<T> _converter;
    int _cellId, _pssMisses, _sssMisses;
    double _freq, _gain;
    std::atomic<bool> _reset, _stop;
    std::map<int, std::string> _stateStrings;

    /* Internal C objects */
    std::vector<struct lte_ref_map *[4]> _pbchRefMaps;
    struct lte_rx *_rx;
    struct lte_sync _sync;
};
#endif /* _SYNCHRONIZER_H_ */
