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
    state->epfd = epoll_create1(0);
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
    struct epoll_event ee = {0};
    int op = eventLoop->getEvents()[fd].mask == MP_NO_MASK ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= eventLoop->getEvents()[fd].mask;
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
    struct epoll_event ee = {0};
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
            struct epoll_event *ee = &state->events[j];

            if (ee->events & EPOLLIN)
                mask |= MP_READABLE;
            if (ee->events & EPOLLOUT)
                mask |= MP_WRITABLE;
            //if (ee->events & EPOLLERR)
            //    mask |= MP_WRITABLE;
            //if (ee->events & EPOLLHUP)
            //    mask |= MP_WRITABLE;
            eventLoop->getFiredEvents()[j].fd = ee->data.fd;
            eventLoop->getFiredEvents()[j].mask = mask;
        }
    }
    else if (retval < 0)
    {
        if (errno != EINTR)
        {
            log_fatal("epoll_wait errno %d %s", errno, strerror(errno));
        }
    }
    return numevents;
}

static char *MpApiName(void)
{
    return "epoll";
}
