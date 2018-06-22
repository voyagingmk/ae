#include <sys/select.h>
#include <string.h>

struct MpApiState
{
    fd_set rfds, wfds;
    fd_set _rfds, _wfds; // select之后fd_set会被修改，所以要有一个备份
};

static int MpApiCreate(MpEventLoop *eventLoop)
{
    MpApiState *state = new MpApiState;
    if (!state)
        return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    eventLoop->setApiData((void *)state);
    return 0;
}

static int MpApiResize(MpEventLoop *eventLoop, int setsize)
{
    if (setsize >= FD_SETSIZE)
        return -1;
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

    if (mask & MP_READABLE)
        FD_SET(fd, &state->rfds);
    if (mask & MP_WRITABLE)
        FD_SET(fd, &state->wfds);
    return 0;
}

static int MpApiDelEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();

    if (mask & MP_READABLE)
        FD_CLR(fd, &state->rfds);
    if (mask & MP_WRITABLE)
        FD_CLR(fd, &state->wfds);
    return 0;
}

static int MpApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = (MpApiState *)eventLoop->getApiData();
    int retval, fd, numevents = 0;

    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    retval = select(eventLoop->getMaxFd() + 1,
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
