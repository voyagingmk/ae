#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

struct MpApiState
{
    int kqfd;
    kevent *events;
};

static int aeApiCreate(MpEventLoop *eventLoop)
{
    MpApiState *state = new MpApiState;

    if (!state)
        return -1;
    state->events = (struct kevent *)zmalloc(sizeof(struct kevent) * eventLoop->setsize);
    if (!state->events)
    {
        zfree(state);
        return -1;
    }
    state->kqfd = kqueue();
    if (state->kqfd == -1)
    {
        zfree(state->events);
        zfree(state);
        return -1;
    }
    eventLoop->apidata = state;
    return 0;
}

static int aeApiResize(MpEventLoop *eventLoop, int setsize)
{
    MpApiState *state = (MpApiState *)eventLoop->apidata;

    state->events = (struct kevent *)zrealloc(state->events, sizeof(struct kevent) * setsize);
    return 0;
}

static void aeApiFree(MpEventLoop *eventLoop)
{
    MpApiState *state = (MpApiState *)eventLoop->apidata;

    close(state->kqfd);
    zfree(state->events);
    zfree(state);
}

static int aeApiAddEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (MpApiState *)eventLoop->apidata;
    struct kevent ke;

    if (mask & AE_READABLE)
    {
        EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(state->kqfd, &ke, 1, NULL, 0, NULL) == -1)
            return -1;
    }
    if (mask & AE_WRITABLE)
    {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        if (kevent(state->kqfd, &ke, 1, NULL, 0, NULL) == -1)
            return -1;
    }
    return 0;
}

static void aeApiDelEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (MpApiState *)eventLoop->apidata;
    struct kevent ke;

    if (mask & AE_READABLE)
    {
        EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(state->kqfd, &ke, 1, NULL, 0, NULL);
    }
    if (mask & AE_WRITABLE)
    {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(state->kqfd, &ke, 1, NULL, 0, NULL);
    }
}

static int aeApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = (MpApiState *)eventLoop->apidata;
    int retval, numevents = 0;

    if (tvp != NULL)
    {
        struct timespec timeout;
        timeout.tv_sec = tvp->tv_sec;
        timeout.tv_nsec = tvp->tv_usec * 1000;
        retval = kevent(state->kqfd, NULL, 0, state->events, eventLoop->setsize,
                        &timeout);
    }
    else
    {
        retval = kevent(state->kqfd, NULL, 0, state->events, eventLoop->setsize,
                        NULL);
    }

    if (retval > 0)
    {
        int j;

        numevents = retval;
        for (j = 0; j < numevents; j++)
        {
            int mask = 0;
            struct kevent *e = state->events + j;

            if (e->filter == EVFILT_READ)
                mask |= AE_READABLE;
            if (e->filter == EVFILT_WRITE)
                mask |= AE_WRITABLE;
            eventLoop->fired[j].fd = e->ident;
            eventLoop->fired[j].mask = mask;
        }
    }
    return numevents;
}

static char *aeApiName(void)
{
    return "kqueue";
}
