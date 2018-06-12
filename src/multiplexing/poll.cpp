#include <poll.h>

struct MpApiState
{
    pollfd *fds;
    int size;
};

static int MpApiCreate(MpEventLoop *eventLoop)
{
    MpApiState *state = new MpApiState;
    if (!state)
        return -1;
    state->size = 2;
    state->fds = std::malloc(state->size * sizeof(pollfd));
    eventLoop->setApiData((void *)state);
    return 0;
}

static int MpApiResize(MpEventLoop *eventLoop, int setsize)
{
    if (setsize <= size)
    {
        return -1;
    }
    state->size = setsize;
    MpApiState *state = (MpApiState *)eventLoop->getApiData();
    state->fds = std::realloc(state->fds, state->size * sizeof(pollfd));
    return 0;
}

static void MpApiFree(MpEventLoop *eventLoop)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();
    delete state;
}

static int MpApiAddEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();
    if (fd >= state->size)
    {
        return -1;
    }
    state->fds[fd] = ;

    return 0;
}

static void MpApiDelEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();

    if (mask & MP_READABLE)
        FD_CLR(fd, &state->rfds);
    if (mask & MP_WRITABLE)
        FD_CLR(fd, &state->wfds);
}

static int MpApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();
    int retval, fd, numevents = 0;

    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    retval = poll(eventLoop->getMaxFd() + 1,
                  &state->_rfds, &state->_wfds, NULL, tvp);
    if (retval > 0)
    {
        for (fd = 0; fd <= eventLoop->getMaxFd(); fd++)
        {
            int mask = 0;
            MpFileEvent *fe = &eventLoop->getEvents()[fd];

            if (fe->mask == MP_NO_MASK)
                continue;
            if (fe->mask & MP_READABLE && FD_ISSET(fd, &state->_rfds))
                mask |= MP_READABLE;
            if (fe->mask & MP_WRITABLE && FD_ISSET(fd, &state->_wfds))
                mask |= MP_WRITABLE;
            eventLoop->getFiredEvents()[numevents].fd = fd;
            eventLoop->getFiredEvents()[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

static const char *MpApiName(void)
{
    return "select";
}
