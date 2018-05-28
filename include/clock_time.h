#ifndef WY_CLOCK_TIME_H
#define WY_CLOCK_TIME_H

#include "common.h"
#include <time.h>
#include <chrono>

namespace wynet
{

class ClockTimeC
{
  public:
    ClockTimeC(double duration)
    {
        long seconds = static_cast<long>(duration);
        double fraction = duration - seconds;
        int64_t nanoseconds = static_cast<int64_t>(fraction * kNanoSecondsPerSecond);
        m_ts.tv_nsec = nanoseconds;
        m_ts.tv_sec = seconds;
    }

    ClockTimeC()
    {
        m_ts.tv_nsec = 0;
        m_ts.tv_sec = 0;
    }

    ClockTimeC(struct timespec _ts)
    {
        m_ts = _ts;
    }

    ClockTimeC &operator+=(ClockTimeC &ct)
    {
        int64_t nanoseconds = m_ts.tv_nsec + ct.m_ts.tv_nsec;
        m_ts.tv_nsec = nanoseconds % kNanoSecondsPerSecond;
        m_ts.tv_sec = m_ts.tv_sec + ct.m_ts.tv_sec + (nanoseconds / kNanoSecondsPerSecond);
        return *this;
    }

    void debugFormat(char *buf, int bufLength)
    {
        ::snprintf(buf, bufLength, "%lld.%.9ld\n", (long long)m_ts.tv_sec, m_ts.tv_nsec);
    }

    timespec &getTimespec()
    {
        return m_ts;
    }

    static timespec getNowTime()
    {
        struct timespec _ts;
        clock_gettime(CLOCK_REALTIME, &_ts);
        return _ts;
    }

  public:
    struct timespec m_ts;

  private:
    static const int64_t kNanoSecondsPerSecond = 1000 * 1000 * 1000;
};

typedef ClockTimeC ClockTime;

}; // namespace wynet

#endif