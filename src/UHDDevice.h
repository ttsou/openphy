#ifndef _UHDDEVICE_H_
#define _UHDDEVICE_H_

#include <complex>
#include <vector>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "TimestampBuffer.h"

template <typename T = std::complex<short>>
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
    int pull(std::vector<std::vector<T>> &bufs, size_t len, int64_t ts);

    enum ReferenceType {
        REF_INTERNAL,
        REF_EXTERNAL,
        REF_GPS,
    };

    enum DeviceType {
        DEV_B200,
        DEV_B210,
        DEV_X300,
        DEV_UNKNOWN,
    };

private:
    bool initRates(int rbs);
    void initRx(int64_t &ts);

    DeviceType _type;
    size_t _chans;
    size_t _spp;
    double _rate;
    double _base_freq;
    double _offset_freq;
    unsigned long long _prev_ts;
    uhd::usrp::multi_usrp::sptr _dev;
    uhd::rx_streamer::sptr _stream;
    std::vector<std::shared_ptr<TSBuffer>> _rx_bufs;
};

#endif /* _UHDDEVICE_H_ */
