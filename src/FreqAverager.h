#ifndef _FREQ_AVERAGER_
#define _FREQ_AVERAGER_

#include <queue>
#include <mutex>
#include <thread>

class FreqAverager {
public:
    FreqAverager(size_t n = 0);
    FreqAverager(const FreqAverager &f);
    FreqAverager(FreqAverager &&f);
    ~FreqAverager() = default;

    FreqAverager &operator=(const FreqAverager &f);
    FreqAverager &operator=(FreqAverager &&f);

    bool empty();
    bool full();
    void push(double f);
    double average();

private:
    size_t _size;
    std::mutex _mutex;
    std::queue<double> _freqs;
};
#endif /* _FREQ_AVERAGER_ */
