#ifndef WY_CLOCK_TIME_H
#define WY_CLOCK_TIME_H

#include "common.h"
#include "logger/log.h"
#include <time.h>
#include <chrono>

namespace wynet
{

class ClockTime
{
  public:
    ClockTime()
    {
        m_ts.tv_nsec = 0;
        m_ts.tv_sec = 0;
    }

    ClockTime(double duration)
    {
        long seconds = static_cast<long>(duration);
        double fraction = duration - seconds;
        int64_t nanoseconds = static_cast<int64_t>(fraction * kNanoSecondsPerSecond);
        m_ts.tv_nsec = nanoseconds;
        m_ts.tv_sec = seconds;
    }

    ClockTime(const ClockTime &t)
    {
        m_ts.tv_nsec = t.m_ts.tv_nsec;
        m_ts.tv_sec = t.m_ts.tv_sec;
    }

    ClockTime(const timespec &_ts)
    {
        m_ts = _ts;
    }

    ClockTime &operator+=(const ClockTime &ct)
    {
        int64_t nanoseconds = m_ts.tv_nsec + ct.m_ts.tv_nsec;
        m_ts.tv_nsec = nanoseconds % kNanoSecondsPerSecond;
        m_ts.tv_sec = m_ts.tv_sec + ct.m_ts.tv_sec + (nanoseconds / kNanoSecondsPerSecond);
        return *this;
    }

    bool operator==(const ClockTime &ct)
    {
        return m_ts.tv_sec == ct.m_ts.tv_sec && m_ts.tv_nsec == ct.m_ts.tv_nsec;
    }

    void debugFormat(char *buf, int bufLength)
    {
        ::snprintf(buf, bufLength, "%lld.%.9ld\n", (long long)m_ts.tv_sec, m_ts.tv_nsec);
    }

    timespec &getTimespec()
    {
        return m_ts;
    }

    static std::chrono::nanoseconds toChrono(ClockTime &ct)
    {
        std::chrono::nanoseconds s = std::chrono::seconds(ct.m_ts.tv_sec) + std::chrono::nanoseconds(ct.m_ts.tv_nsec);
        return s;
    }

    template <typename Duration>
    static ClockTime fromChrono(const Duration &d)
    {
        timespec ts;
        std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        ts.tv_sec = sec.count();
        ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d - sec).count();
        return ClockTime(ts);
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

class TimeMeasure
{
  public:
    TimeMeasure(const char *tag) : m_start(std::chrono::steady_clock::now()),
                                   m_tag(tag)
    {
    }
    ~TimeMeasure()
    {
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_start);
        duration.count();
        log_info("[measure] %s %dms", m_tag, duration.count());
    }

  private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    const char *m_tag;
};

}; // namespace wynet

#endif