#ifndef WY_TIMER_REF_H
#define WY_TIMER_REF_H

#include "common.h"

namespace wynet
{

typedef long long WyTimerId;

class TimerRef
{
    WyTimerId m_id;

  public:
    explicit TimerRef(const WyTimerId _id = 0) : m_id(_id)
    {
    }

    TimerRef(const TimerRef &tr) : m_id(tr.m_id)
    {
    }

    TimerRef &operator=(const TimerRef &tr)
    {
        m_id = tr.m_id;
        return *this;
    }

    TimerRef(TimerRef &&tr)
    {
        m_id = tr.m_id;
        tr.m_id = 0;
    }

    TimerRef &operator=(TimerRef &&tr)
    {
        m_id = tr.m_id;
        tr.m_id = 0;
        return *this;
    }

    WyTimerId Id() const
    {
        return m_id;
    }

    bool validate() const
    {
        return m_id > 0;
    }

    bool operator<(const TimerRef &tr) const
    {
        return m_id < tr.m_id;
    }

    static TimerRef newTimerRef()
    {
        WyTimerId _id = ++g_numCreated;
        return TimerRef(_id);
    }
    // TODO 可以用线程ID和thread local做唯一ID
    static std::atomic<WyTimerId> g_numCreated;
};

}; // namespace wynet

#endif