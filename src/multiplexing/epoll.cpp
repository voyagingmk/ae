#include <sys/epoll.h>
#include <sys/ioctl.h>
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
    state->events.resize(eventLoop->getSetSize(), {0});
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
    state->events.resize(setsize, {0});
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
    MpFileEvent &fe = eventLoop->getEvents()[fd];
    int op = fe.mask == MP_NO_MASK ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= fe.mask;
    if (mask & MP_READABLE)
        ee.events |= EPOLLIN;
    if (mask & MP_WRITABLE)
        ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    assert(fd > 0);
    assert(ee.events > 0);
    if (epoll_ctl(state->epfd, op, fd, &ee) == -1)
    {
        log_fatal("epoll_ctrl err %d %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int MpApiDelEvent(MpEventLoop *eventLoop, int fd, int delmask)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    struct epoll_event ee = {0};
    MpFileEvent &fe = eventLoop->getEvents()[fd];
    int mask = fe.mask & (~delmask);

    ee.events = 0;
    if (mask & MP_READABLE)
        ee.events |= EPOLLIN;
    if (mask & MP_WRITABLE)
        ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    int ret = 0;
    if (mask != MP_NO_MASK)
    {
        ret = epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
    }
    else
    {
        ret = epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
    }
    return ret;
}

static int MpApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = (struct MpApiState *)eventLoop->getApiData();
    int retval, numevents = 0;

    retval = epoll_wait(state->epfd, &state->events[0], state->events.size(),
                        tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
    if (retval > 0)
    {
        int j;
        numevents = retval;
        int r = 0;
        int w = 0;
        for (j = 0; j < numevents; j++)
        {
            int mask = MP_NO_MASK;
            struct epoll_event *ee = &state->events[j];

            if (ee->events & EPOLLIN)
            {
                r++;
                mask |= MP_READABLE;
            }
            if (ee->events & EPOLLOUT)
            {
                w++;
                mask |= MP_WRITABLE;
            }
            /*
            if (ee->events & EPOLLERR)
                mask |= MP_WRITABLE;
            if (ee->events & EPOLLHUP)
                mask |= MP_WRITABLE;
            */
            assert(ee->events > 0);
            assert(mask > 0);
            MpFiredEvent &evt = eventLoop->getFiredEvents()[j];
            evt.fd = ee->data.fd;
            evt.mask = mask;
            MpFileEvent &fe = eventLoop->getEvents()[evt.fd];
            assert((fe.mask & mask) > 0);

            int val;
            socklen_t len = sizeof(val);
            if (getsockopt(evt.fd, SOL_SOCKET, SO_ACCEPTCONN, &val, &len) == -1)
            {
                log_fatal("fd %d is not a socket", evt.fd);
            }
            else if (val)
            {
                // printf("fd %d is a listening socket\n", fd);
            }
            else
            {
                //  printf("fd %d is a non-listening socket\n", fd);
                int unsentBytes = 0;
                int r = ioctl(evt.fd, TIOCOUTQ, &unsentBytes);
                if (r == -1)
                {
                    log_info("ioctl fd %d", evt.fd);
                    log_fatal("ioctl err %d %s", errno, strerror(errno));
                }
                int sndbuf = 0;
                len = sizeof(sndbuf);
                int r2 = getsockopt(evt.fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, &len);

                assert(r2 == 0);
                assert((sndbuf - unsentBytes) > 0);
            }
        }
        // log_info("numevents %d r w %d %d", numevents, r, w);
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
