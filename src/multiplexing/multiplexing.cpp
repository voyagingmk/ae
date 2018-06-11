
#include <sys/time.h>
#include <poll.h>
#include "multiplexing.h"
#include "config.h"
#include "logger/log.h"

namespace wynet
{

#include "select.cpp"

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
    m_timeEventNextId = 0;
    m_timeEventNearest = nullptr;
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
    if (fe->mask == MP_NONE)
        return;

    MpApiDelEvent(this, fd, mask);
    fe->mask = fe->mask & (~mask);
    if (fd == m_maxfd && fe->mask == MP_NONE)
    {
        int j;
        for (j = m_maxfd - 1; j >= 0; j--)
            if (m_events[j].mask != MP_NONE)
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

    gettimeofday(&tv, NULL);
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
    long long id = m_timeEventNextId++;
    MpTimeEventPtr te = std::make_shared<MpTimeEvent>();
    te->id = id;
    long now_sec, now_ms;
    MpGetTime(&now_sec, &now_ms);
    addMillisecondsToNow(now_sec, now_ms, milliseconds, &te->when_sec, &te->when_ms);
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    m_teSet.insert(te);
    m_timeEventNearest = searchNearestTimer();
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
        te->id = AE_DELETED_EVENT_ID;
        m_teListDeleted.push_back(te);
        m_timeEventNearest = searchNearestTimer();
        return AE_OK;
    }
    return AE_ERR;
}

const MpTimeEventPtr &MpEventLoop::searchNearestTimer()
{
    if (m_teSet.size() > 0)
    {
        for (auto it = m_teSet.begin(); it != m_teSet.end(); it++)
        {
            const MpTimeEventPtr &nearest = *it;
            if (nearest->id != AE_DELETED_EVENT_ID)
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
    MpTimeEventPtr te;
    long long maxId;
    time_t now = time(nullptr);

    m_lastTime = now;

    processDeletedEvents();

    maxId = m_timeEventNextId - 1;
    std::vector<MpTimeEventPtr> teListTimeout;

    long now_sec, now_ms;
    MpGetTime(&now_sec, &now_ms);
    std::vector<std::tuple<int, MpTimeEventPtr>> teListReinsert;
    for (auto it = m_teSet.begin(); it != m_teSet.end(); it++)
    {
        te = *it;
        if (te->id > maxId)
        {
            continue;
        }
        // fprintf(stderr, " te->when_sec - now_sec %d\n", int(te->when_sec - now_sec));
        // fprintf(stderr, " te->when_ms - now_ms %d\n", int(te->when_ms - now_ms));
        if (now_sec > te->when_sec ||
            (now_sec == te->when_sec && now_ms >= te->when_ms))
        {
            // fprintf(stderr, "to remove te id %d\n", int(te->id));
            teListTimeout.push_back(te);
        }
    }
    for (auto itte = teListTimeout.begin(); itte != teListTimeout.end(); itte++)
    {
        MpTimeEventPtr te = *itte;
        /*
        fprintf(stderr, "remove te %d\n", int(te->id));
        fprintf(stderr, "size: %d\n", int(m_teSet.size()));
        */
        auto it2 = m_teSet.find(te);
        if (it2 != m_teSet.end())
        {
            m_teSet.erase(it2);
        }
        auto id = te->id;
        int retval = te->timeProc(this, id, te->clientData);
        processed++;
        if (retval != AE_NOMORE)
        {
            teListReinsert.push_back(std::tuple<int, MpTimeEventPtr>{retval, te});
        }
    }
    for (auto itte = teListReinsert.begin(); itte != teListReinsert.end(); itte++)
    {
        int retval = std ::get<0>(*itte);
        MpTimeEventPtr te = std ::get<1>(*itte);
        addMillisecondsToNow(now_sec, now_ms, retval, &te->when_sec, &te->when_ms);
        // fprintf(stderr, "insert te %d\n", int(te->id));
        m_teSet.insert(te);
    }

    if (processed > 0)
    {
        m_timeEventNearest = searchNearestTimer();
    }
    return processed;
}

void MpEventLoop::processDeletedEvents()
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
    int processed = 0, numevents;

    /* Nothing to do? return ASAP */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS))
        return 0;

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (m_maxfd != -1 ||
        ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT)))
    {
        int j;
        MpTimeEventPtr shortest = nullptr;
        struct timeval tv, *tvp;

        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            shortest = m_timeEventNearest;
        if (shortest)
        {
            long now_sec, now_ms;

            MpGetTime(&now_sec, &now_ms);
            tvp = &tv;

            /* How many milliseconds we need to wait for the next
             * time event to fire? */
            long long ms =
                (shortest->when_sec - now_sec) * 1000 +
                shortest->when_ms - now_ms;

            if (ms > 0)
            {
                tvp->tv_sec = ms / 1000;
                tvp->tv_usec = (ms % 1000) * 1000;
            }
            else
            {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }
        }
        else
        {
            /* If we have to check for events but need to return
             * ASAP because of AE_DONT_WAIT we need to set the timeout
             * to zero */
            if (flags & AE_DONT_WAIT)
            {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            }
            else
            {
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        /* Call the multiplexing API, will return only on timeout or when
         * some event fires. */
        numevents = MpApiPoll(this, tvp);

        /* After sleep callback. */
        if (m_aftersleep != NULL && flags & AE_CALL_AFTER_SLEEP)
            m_aftersleep(this);

        for (j = 0; j < numevents; j++)
        {
            int fd = m_fired[j].fd;
            int mask = m_fired[j].mask;
            MpFileEvent *fe = &m_events[fd];
            int fe_mask = fe->mask;
            const MpFileProc &rfileProc = fe->rfileProc;
            const MpFileProc &wfileProc = fe->wfileProc;
            void *clientData = fe->clientData;
            int rfired = 0;

            /* note the fe->mask & mask & ... code: maybe an already processed
             * event removed an element that fired and we still didn't
             * processed, so we check if the event is still valid. */
            if (fe_mask & mask & AE_READABLE)
            {
                rfired = 1;
                rfileProc(this, fd, clientData, mask);
            }
            if (fe_mask & mask & AE_WRITABLE)
            {
                if (!rfired /*|| wfileProc != rfileProc*/)
                    wfileProc(this, fd, clientData, mask);
            }
            processed++;
        }
    }
    /* Check time events */
    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents();

    return processed; /* return the number of processed file/time events */
}

/* Wait for milliseconds until the given file descriptor becomes
 * writable/readable/exception */
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

void MpEventLoop::main()
{
    m_stop = 0;
    while (!m_stop)
    {
        if (m_beforesleep != NULL)
            m_beforesleep(this);
        processEvents(MP_ALL_EVENTS | MP_CALL_AFTER_SLEEP);
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