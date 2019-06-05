#ifndef _FILEDEVICE_H_
#define _FILEDEVICE_H_

#include <vector>
#include <string>
#include <memory>
#include <fstream>

#include "Device.h"
#include "TimestampBuffer.h"

template <typename T>
class FileDevice : public Device<T> {
    typedef TimestampBuffer<T> TSBuffer;

public:
    FileDevice(size_t chans = 1);
    ~FileDevice();

    FileDevice(const FileDevice &) = delete;
    FileDevice &operator=(const FileDevice &) = delete;

    void init(int64_t &ts, size_t rbs, int ref, const std::string &filename);
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

private:
    bool initRates(int rbs);
    void initRx(int64_t &ts);

    size_t _chans;
    size_t _spp;
    double _rate;
    double _offset_freq;
    unsigned long long _prev_ts;
    std::ifstream _stream;
    std::vector<std::shared_ptr<TSBuffer>> _rx_bufs;
};

#endif /* _FILEDEVICE_H_ */
