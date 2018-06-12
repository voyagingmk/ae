
#include <sys/time.h>
#include <poll.h>
#include "multiplexing.h"
#include "config.h"
#include "logger/log.h"

namespace wynet
{
#ifdef HAVE_EPOLL
#include "epoll.cpp"
#else
#include "select.cpp"
#endif

bool MpTimeEvent::operator<(const MpTimeEvent &rhs) const
{
    if (when_sec == rhs.when_sec)
    {
        return when_ms < rhs.when_ms;
    }
    return when_sec < rhs.when_sec;
}

MpEventLoop::MpEventLoop(int setsize)
{
    m_events.resize(setsize);
    m_fired.resize(setsize);
    m_setsize = setsize;
    m_lastTime = time(nullptr);
    m_teNextId = 0;
    m_teNearest = nullptr;
    m_stop = 0;
    m_maxfd = -1;
    m_beforesleep = nullptr;
    m_aftersleep = nullptr;
    if (MpApiCreate(this) == -1)
    {
        log_fatal("MpApiCreate failed.");
    }
}

int MpEventLoop::resizeSetSize(int setsize)
{
    if (setsize == m_setsize)
        return MP_OK;
    if (m_maxfd >= setsize)
        return MP_ERR;
    if (MpApiResize(this, setsize) == -1)
        return MP_ERR;
    m_events.resize(setsize);
    m_fired.resize(setsize);
    m_setsize = setsize;
    return MP_OK;
}

MpEventLoop::~MpEventLoop()
{
    MpApiFree(this);
}

void MpEventLoop::stop()
{
    m_stop = 1;
}

int MpEventLoop::createFileEvent(int fd, int mask,
                                 MpFileProc proc, void *clientData)
{
    if (fd >= m_setsize)
    {
        errno = ERANGE;
        return MP_ERR;
    }
    MpFileEvent *fe = &m_events[fd];
    assert(mask > 0);
    if (MpApiAddEvent(this, fd, mask) == -1)
        return MP_ERR;
    fe->mask |= mask;
    if (mask & MP_READABLE)
        fe->rfileProc = proc;
    if (mask & MP_WRITABLE)
        fe->wfileProc = proc;
    fe->clientData = clientData;
    if (fd > m_maxfd)
        m_maxfd = fd;
    return MP_OK;
}

void MpEventLoop::deleteFileEvent(int fd, int mask)
{
    if (fd >= m_setsize)
        return;
    MpFileEvent *fe = &m_events[fd];
    if (fe->mask == MP_NO_MASK)
        return;

    MpApiDelEvent(this, fd, mask);
    fe->mask = fe->mask & (~mask);
    if (fd == m_maxfd && fe->mask == MP_NO_MASK)
    {
        int j;
        for (j = m_maxfd - 1; j >= 0; j--)
            if (m_events[j].mask != MP_NO_MASK)
                break;
        m_maxfd = j;
    }
}

int MpEventLoop::getFileEvents(int fd)
{
    if (fd >= m_setsize)
        return 0;
    MpFileEvent *fe = &m_events[fd];
    return fe->mask;
}

static void MpGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;

    gettimeofday(&tv, nullptr);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec / 1000;
}

static void addMillisecondsToNow(long now_sec, long now_ms, long long milliseconds, long *sec, long *ms)
{
    long when_sec, when_ms;

    when_sec = now_sec + milliseconds / 1000;
    when_ms = now_ms + milliseconds % 1000;
    if (when_ms >= 1000)
    {
        when_sec++;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

long long MpEventLoop::createTimeEvent(long long milliseconds,
                                       MpTimeProc proc, void *clientData,
                                       MpEventFinalizerProc finalizerProc)
{
    long long id = m_teNextId++;
    MpTimeEventPtr te = std::make_shared<MpTimeEvent>();
    te->id = id;
    long now_sec, now_ms;
    MpGetTime(&now_sec, &now_ms);
    addMillisecondsToNow(now_sec, now_ms, milliseconds, &te->when_sec, &te->when_ms);
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    m_teSet.insert(te);
    m_teNearest = searchNearestTimer();
    return id;
}

int MpEventLoop::deleteTimeEvent(long long id)
{
    auto it = std::find_if(
        m_teSet.begin(), m_teSet.end(),
        [&id](const MpTimeEventPtr &x) { return (x->id) == id; });
    if (it != m_teSet.end())
    {
        const MpTimeEventPtr &te = *it;
        te->id = DELETED_TIME_EVENT;
        m_teListDeleted.push_back(te);
        m_teNearest = searchNearestTimer();
        return MP_OK;
    }
    return MP_ERR;
}

const MpTimeEventPtr &MpEventLoop::searchNearestTimer()
{
    if (m_teSet.size() > 0)
    {
        for (auto it = m_teSet.begin(); it != m_teSet.end(); it++)
        {
            const MpTimeEventPtr &nearest = *it;
            if (nearest->id != DELETED_TIME_EVENT)
            {
                return nearest;
            }
        }
    }
    return nullptr;
}

int MpEventLoop::processTimeEvents()
{
    int processed = 0;
    long long maxId;
    time_t now = time(nullptr);

    m_lastTime = now;

    cleanDeletedTimeEvents();

    maxId = m_teNextId - 1;
    std::vector<MpTimeEventPtr> teListTimeout;
    std::vector<std::tuple<int, MpTimeEventPtr>> teListReinsert;

    long now_sec, now_ms;
    MpGetTime(&now_sec, &now_ms);

    for (auto it = m_teSet.begin(); it != m_teSet.end(); it++)
    {
        const MpTimeEventPtr &te = *it;
        if (te->id == DELETED_TIME_EVENT)
        {
            log_fatal("deleted time event?");
        }
        if (te->id > maxId)
        {
            continue;
        }
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms))
        {
            teListTimeout.push_back(te);
        }
    }
    for (auto it = teListTimeout.begin(); it != teListTimeout.end(); it++)
    {
        const MpTimeEventPtr &te = *it;
        auto _it = m_teSet.find(te);
        if (_it != m_teSet.end())
        {
            m_teSet.erase(_it);
        }
        int retval = te->timeProc(this, te->id, te->clientData);
        processed++;
        if (retval != MP_HALT)
        {
            teListReinsert.push_back(std::tuple<int, MpTimeEventPtr>{retval, te});
        }
    }

    MpGetTime(&now_sec, &now_ms); // maybe timeProc cost too much time
    for (auto it = teListReinsert.begin(); it != teListReinsert.end(); it++)
    {
        int retval = std ::get<0>(*it);
        const MpTimeEventPtr &te = std ::get<1>(*it);
        addMillisecondsToNow(now_sec, now_ms, retval, &te->when_sec, &te->when_ms);
        m_teSet.insert(te);
    }

    if (processed > 0)
    {
        m_teNearest = searchNearestTimer();
    }
    return processed;
}

void MpEventLoop::cleanDeletedTimeEvents()
{

    if (m_teListDeleted.size() > 0)
    {
        for (auto it = m_teListDeleted.begin(); it != m_teListDeleted.end(); it++)
        {
            MpTimeEventPtr &_te = *it;
            auto _it = m_teSet.find(_te);
            assert(_it != m_teSet.end());
            m_teSet.erase(_it);
        }
        for (auto it = m_teListDeleted.begin(); it != m_teListDeleted.end(); it++)
        {
            MpTimeEventPtr &_te = *it;
            if (_te->finalizerProc)
                _te->finalizerProc(this, _te->clientData);
        }
        m_teListDeleted.clear();
    }
}

int MpEventLoop::processEvents(int flags)
{
    if (m_beforesleep != nullptr)
        m_beforesleep(this);

    int processed = 0, numevents;

    if (!(flags & MP_TIME_EVENTS) && !(flags & MP_FILE_EVENTS))
        return 0;

    if (m_maxfd != -1 ||
        ((flags & MP_TIME_EVENTS) && !(flags & MP_DONT_WAIT)))
    {
        MpTimeEventPtr nearest = nullptr;
        struct timeval tv, *tvp;

        if (flags & MP_TIME_EVENTS && !(flags & MP_DONT_WAIT))
            nearest = m_teNearest;
        if (nearest)
        {
            long now_sec, now_ms;

            MpGetTime(&now_sec, &now_ms);
            tvp = &tv;

            long long wait_ms = (nearest->when_sec - now_sec) * 1000 +
                                nearest->when_ms - now_ms;

            if (wait_ms > 0)
            {
                tvp->tv_sec = wait_ms / 1000;
                tvp->tv_usec = (wait_ms % 1000) * 1000;
            }
            else
            {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }
        }
        else
        {
            if (flags & MP_DONT_WAIT)
            {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            }
            else
            {
                tvp = nullptr; // block, wait forever
            }
        }

        numevents = MpApiPoll(this, tvp);

        if (m_aftersleep != nullptr && flags & MP_CALL_AFTER_SLEEP)
            m_aftersleep(this);

        for (int j = 0; j < numevents; j++)
        {
            int fd = m_fired[j].fd;
            int mask = m_fired[j].mask;
            MpFileEvent *fe = &m_events[fd];
            int fe_mask = fe->mask;
            const MpFileProc &rfileProc = fe->rfileProc;
            const MpFileProc &wfileProc = fe->wfileProc;
            void *clientData = fe->clientData;
            bool fired = false;

            if (fe_mask & mask & MP_READABLE)
            {
                fired = true;
                rfileProc(this, fd, clientData, mask);
            }
            if (fe_mask & mask & MP_WRITABLE)
            {
                // if (!fired || wfileProc != rfileProc)
                if (!fired)
                    wfileProc(this, fd, clientData, mask);
            }
            processed++;
        }
    }
    if (flags & MP_TIME_EVENTS)
        processed += processTimeEvents();

    return processed;
}

int MpEventLoop::wait(int fd, int mask, long long milliseconds)
{
    struct pollfd pfd;
    int retmask = 0, retval;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    if (mask & MP_READABLE)
        pfd.events |= POLLIN;
    if (mask & MP_WRITABLE)
        pfd.events |= POLLOUT;

    if ((retval = poll(&pfd, 1, milliseconds)) == 1)
    {
        if (pfd.revents & POLLIN)
            retmask |= MP_READABLE;
        if (pfd.revents & POLLOUT)
            retmask |= MP_WRITABLE;
        if (pfd.revents & POLLERR)
            retmask |= MP_WRITABLE;
        if (pfd.revents & POLLHUP)
            retmask |= MP_WRITABLE;
        return retmask;
    }
    else
    {
        return retval;
    }
}

const char *MpEventLoop::getApiName(void)
{
    return MpApiName();
}

void MpEventLoop::setBeforeSleepProc(MpBeforeSleepProc beforesleep)
{
    m_beforesleep = beforesleep;
}

void MpEventLoop::setAfterSleepProc(MpBeforeSleepProc aftersleep)
{
    m_aftersleep = aftersleep;
}

}; // namespace wynet