#ifndef WY_CLOCK_TIME_H
#define WY_CLOCK_TIME_H
#include <time.h>

class ClockTime
{
  public:
    ClockTime(double duration)
    {
        long seconds = static_cast<long>(duration);
        double fraction = duration - seconds;
        int64_t nanoseconds = static_cast<int64_t>(fraction * kNanoSecondsPerSecond);
        ts.tv_nsec = nanoseconds;
        ts.tv_sec = seconds;
    }

    ClockTime()
    {
        ts.tv_nsec = 0;
        ts.tv_sec = 0;
    }

    ClockTime(struct timespec _ts)
    {
        ts = _ts;
    }

    ClockTime &operator+=(ClockTime &ct)
    {
        int64_t nanoseconds = ts.tv_nsec + ct.ts.tv_nsec;
        ts.tv_nsec = nanoseconds % kNanoSecondsPerSecond;
        ts.tv_sec = ts.tv_sec + ct.ts.tv_sec + (nanoseconds / kNanoSecondsPerSecond);
    }

    static timespec getNowTime()
    {
        struct timespec _ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &_ts);
        return _ts;
    }

  public:
    struct timespec ts;

  private:
    static const int64_t kNanoSecondsPerSecond = 1000 * 1000 * 1000;
};

#endif