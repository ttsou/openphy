#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <complex>
#include <vector>
#include <string>

#include "TimestampBuffer.h"

template <typename T = std::complex<short>>
class Device {
    typedef TimestampBuffer<T> TSBuffer;

public:
    Device(size_t chans = 1) { };
    virtual ~Device() { };

    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    virtual void init(int64_t &ts, size_t rbs, int ref, const std::string &args) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;

    virtual void setFreq(double freq) = 0;
    virtual double setGain(double gain) = 0;
    virtual void shiftFreq(double offset) = 0;
    virtual void resetFreq() = 0;

    virtual int64_t get_ts_high() = 0;
    virtual int64_t get_ts_low() = 0;

    virtual int reload() = 0;
    virtual int pull(std::vector<std::vector<T>> &bufs, size_t len, int64_t ts) = 0;
};

#endif /* _DEVICE_H_ */
