#ifndef _TIMESTAMP_BUFFER_H_
#define _TIMESTAMP_BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <string>
#include <vector>

using namespace std;

template <typename T>
class TimestampBuffer {
public:
	TimestampBuffer(size_t n);
        TimestampBuffer(const TimestampBuffer &t) = default;
        TimestampBuffer(TimestampBuffer &&t) = default;
	~TimestampBuffer() = default;

        TimestampBuffer &operator=(const TimestampBuffer &t) = default;
        TimestampBuffer &operator=(TimestampBuffer &&t) = default;

	bool init();
	void reset();

	size_t avail_smpls(int64_t ts) const;

	ssize_t read(vector<T> &buf, int64_t ts);
	ssize_t write(const T *buf, size_t len, int64_t ts);
	ssize_t write(const T *buf, size_t len);
	ssize_t write(int64_t ts);

	std::string str_status() const;
	static std::string str_code(ssize_t code);

	enum err {
		ERR_MEM,
		ERR_TIMESTAMP,
		ERR_OVERFLOW,
	};

	int64_t get_last_time() const { return time_end; }
	int64_t get_first_time() const { return time_start; }
private:
	vector<T> data;
	int64_t time_start, time_end, data_start, data_end;
};

#endif /* _TIMESTAMP_BUFFER_H_ */
