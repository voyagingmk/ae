
#include <sys/time.h>
#include <poll.h>
#include "multiplexing.h"
#include "config.h"
#include "logger/log.h"

namespace wynet
{

#ifdef HAVE_EPOLL
#include "multiplexing/epoll.cpp"
#else
#ifdef HAVE_KQUEUE
#include "multiplexing/kqueue.cpp"
#else
#include "multiplexing/select.cpp"
#endif
#endif

static MpTimeEventPtr searchNearestTimer(MpEventLoop *eventLoop);

MpEventLoop::MpEventLoop(int setsize)
{
    m_events.resize(setsize);
    m_fired.resize(setsize);
    m_setsize = setsize;
    m_lastTime = time(NULL);
    m_timeEventNextId = 0;
    m_stop = 0;
    m_maxfd = -1;
    m_beforesleep = NULL;
    m_aftersleep = NULL;
    /*
    if (MpApiCreate(this) == -1)
    {
        log_fatal("MpApiCreate failed.");
    }*/
    /* Events with mask == MP_NONE are not set. So let's initialize the
     * vector with it. */
    for (int i = 0; i < setsize; i++)
        m_events[i].mask = MP_NONE;
}

/* Return the current set size. */
int MpEventLoop::getSetSize()
{
    return m_setsize;
}

/* Resize the maximum set size of the event loop.
 * If the requested set size is smaller than the current set size, but
 * there is already a file descriptor in use that is >= the requested
 * set size minus one, MP_ERR is returned and the operation is not
 * performed at all.
 *
 * Otherwise MP_OK is returned and the operation is successful. */
int MpEventLoop::resizeSetSize(int setsize)
{
    int i;

    if (setsize == m_setsize)
        return MP_OK;
    if (m_maxfd >= setsize)
        return MP_ERR;
    // if (MpApiResize(setsize) == -1)
    //     return MP_ERR;

    m_events.resize(setsize);
    m_fired.resize(setsize);
    m_setsize = setsize;

    /* Make sure that if we created new slots, they are initialized with
     * an MP_NONE mask. */
    for (i = m_maxfd + 1; i < setsize; i++)
        m_events[i].mask = MP_NONE;
    return MP_OK;
}

MpEventLoop::~MpEventLoop()
{
    //  MpApiFree(this);
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

    //if (MpApiAddEvent(this, fd, mask) == -1)
    //     return MP_ERR;
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

    //   MpApiDelEvent(this, fd, mask);
    fe->mask = fe->mask & (~mask);
    if (fd == m_maxfd && fe->mask == MP_NONE)
    {
        /* Update the max fd */
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
    m_pq.insert(te);
    m_timeEventNearest = searchNearestTimer(this);
    return id;
}

int MpEventLoop::deleteTimeEvent(long long id)
{
    auto it = std::find_if(
        m_pq.begin(), m_pq.end(),
        [&id](const MpTimeEventPtr &x) { return (x->id) == id; });
    if (it != m_pq.end())
    {
        (*it)->id = AE_DELETED_EVENT_ID;
        m_timeEventNearest = searchNearestTimer(this);
        return AE_OK;
    }
    return AE_ERR; /* NO event with the specified ID found */
}

static MpTimeEventPtr searchNearestTimer(MpEventLoop *eventLoop)
{
    MpTimeEventPtr nearest = nullptr;
    if (eventLoop->m_pq.size() > 0)
    {
        for (auto it = eventLoop->m_pq.begin(); it != eventLoop->m_pq.end(); it++)
        {
            nearest = *it;
            if (nearest->id != AE_DELETED_EVENT_ID)
            {
                break;
            }
        }
    }
    return nearest;
}

int MpEventLoop::processEvents(int flags)
{
    int processed = 0;
    MpTimeEventPtr te;
    long long maxId;
    time_t now = time(NULL);

    m_lastTime = now;

    maxId = m_timeEventNextId - 1;
    std::vector<MpTimeEventPtr> teListDeleted;
    std::vector<MpTimeEventPtr> teListTimeout;
    for (auto it = m_pq.begin(); it != m_pq.end(); it++)
    {
        const MpTimeEventPtr &_te = *it;
        if (_te->id == AE_DELETED_EVENT_ID)
        {
            teListDeleted.push_back(_te);
        }
    }
    if (teListDeleted.size() > 0)
    {
        for (auto it2 = teListDeleted.begin(); it2 != teListDeleted.end(); it2++)
        {
            MpTimeEventPtr &_te = *it2;
            auto it3 = m_pq.find(_te);
            assert(it3 != m_pq.end());
            m_pq.erase(it3);
        }
    }
    for (auto it2 = teListDeleted.begin(); it2 != teListDeleted.end(); it2++)
    {
        te = *it2;
        if (te->finalizerProc)
            te->finalizerProc(this, te->clientData);
    }
    long now_sec, now_ms;
    MpGetTime(&now_sec, &now_ms);
    std::vector<std::tuple<int, MpTimeEventPtr>> teListReinsert;
    for (auto it = m_pq.begin(); it != m_pq.end(); it++)
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
        fprintf(stderr, "size: %d\n", int(m_pq.size()));
        */
        auto it2 = m_pq.find(te);
        if (it2 != m_pq.end())
        {
            m_pq.erase(it2);
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
        m_pq.insert(te);
    }

    if (processed > 0)
    {
        m_timeEventNearest = searchNearestTimer(this);
    }
    return processed;
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

char *MpEventLoop::getApiName(void)
{
    // return MpApiName();
    return "";
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