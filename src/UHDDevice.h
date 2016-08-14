#ifndef _UHDDEVICE_H_
#define _UHDDEVICE_H_

#include <complex>
#include <vector>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "TimestampBuffer.h"

enum dev_ref_type {
    REF_INTERNAL,
    REF_EXTERNAL,
    REF_GPSDO,
};

enum {
    DEV_TYPE_B200,
    DEV_TYPE_B210,
    DEV_TYPE_X300,
    DEV_TYPE_UNKNOWN,
};

using namespace std;

template <typename T>
class UHDDevice {
    typedef TimestampBuffer<T> TSBuffer;

public:
    UHDDevice(size_t chans = 1);
    ~UHDDevice();

    UHDDevice(const UHDDevice &) = delete;
    UHDDevice &operator=(const UHDDevice &) = delete;

    void init(int64_t &ts, size_t rbs, int ref, const std::string &args);
    void start();
    void stop();
    void reset();

    void setFreq(double freq);
    double setGain(double gain);
    void shiftFreq(double offset);
    void resetFreq();

    int64_t get_ts_high();
    int64_t get_ts_low();

    int reload();
    int pull(vector<vector<T>> &bufs, size_t len, int64_t ts);

private:
    bool initRates(int rbs);
    void initRx(int64_t &ts);

    int _type;
    size_t _chans;
    size_t _spp;
    double _rate;
    double _base_freq;
    double _offset_freq;
    uhd::usrp::multi_usrp::sptr _dev;
    uhd::rx_streamer::sptr _stream;
    vector<shared_ptr<TSBuffer>> _rx_bufs;
};

#endif /* _UHDDEVICE_H_ */
