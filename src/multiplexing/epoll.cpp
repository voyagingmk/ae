#include <sys/epoll.h>
#include <vector>

typedef struct MpApiState
{
    int epfd;
    std::vector<epoll_event> events;
} MpApiState;

static int MpApiCreate(MpEventLoop *eventLoop)
{
    MpApiState *state = new MpApiState;
    if (!state)
        return -1;
    state->events.resize(eventLoop->getSetSize());
    state->epfd = epoll_create(1024); // just a hint
    if (state->epfd == -1)
    {
        delete state;
        return -1;
    }
    eventLoop->setApiData(state);
    return 0;
}

static int MpApiResize(MpEventLoop *eventLoop, int setsize)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    if ((int)(state->events.size()) > setsize)
    {
        return -1;
    }
    state->events.resize(setsize);
    return 0;
}

static void MpApiFree(MpEventLoop *eventLoop)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    close(state->epfd);
    delete state;
    eventLoop->setApiData(nullptr);
}

static int MpApiAddEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = eventLoop->getEvents()[fd].mask == MP_NO_MASK ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= eventLoop->getEvents()[fd].mask; /* Merge old events */
    if (mask & MP_READABLE)
        ee.events |= EPOLLIN;
    if (mask & MP_WRITABLE)
        ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd, op, fd, &ee) == -1)
        return -1;
    return 0;
}

static void MpApiDelEvent(MpEventLoop *eventLoop, int fd, int delmask)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    int mask = eventLoop->getEvents()[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & MP_READABLE)
        ee.events |= EPOLLIN;
    if (mask & MP_WRITABLE)
        ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (mask != MP_NO_MASK)
    {
        epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
    }
    else
    {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
}

static int MpApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    int retval, numevents = 0;

    retval = epoll_wait(state->epfd, &state->events[0], eventLoop->getSetSize(),
                        tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
    if (retval > 0)
    {
        int j;

        numevents = retval;
        for (j = 0; j < numevents; j++)
        {
            int mask = 0;
            struct epoll_event *e = &state->events[j];

            if (e->events & EPOLLIN)
                mask |= MP_READABLE;
            if (e->events & EPOLLOUT)
                mask |= MP_WRITABLE;
            if (e->events & EPOLLERR)
                mask |= MP_WRITABLE;
            if (e->events & EPOLLHUP)
                mask |= MP_WRITABLE;
            eventLoop->getFiredEvents()[j].fd = e->data.fd;
            eventLoop->getFiredEvents()[j].mask = mask;
        }
    }
    return numevents;
}

static char *MpApiName(void)
{
    return "epoll";
}
