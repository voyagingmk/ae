#include <sys/select.h>
#include <string.h>

struct MpApiState
{
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
};

static int MpApiCreate(MpEventLoop *eventLoop)
{
    MpApiState *state = std::malloc(sizeof(MpApiState));
    if (!state)
        return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    eventLoop->apidata = state;
    return 0;
}

static int MpApiResize(MpEventLoop *eventLoop, int setsize)
{
    /* Just ensure we have enough room in the fd_set type. */
    if (setsize >= FD_SETSIZE)
        return -1;
    return 0;
}

static void MpApiFree(MpEventLoop *eventLoop)
{
    zfree(eventLoop->apidata);
}

static int MpApiAddEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = eventLoop->apidata;

    if (mask & MP_READABLE)
        FD_SET(fd, &state->rfds);
    if (mask & MP_WRITABLE)
        FD_SET(fd, &state->wfds);
    return 0;
}

static void MpApiDelEvent(MpEventLoop *eventLoop, int fd, int mask)
{
    MpApiState *state = eventLoop->apidata;

    if (mask & MP_READABLE)
        FD_CLR(fd, &state->rfds);
    if (mask & MP_WRITABLE)
        FD_CLR(fd, &state->wfds);
}

static int MpApiPoll(MpEventLoop *eventLoop, struct timeval *tvp)
{
    MpApiState *state = eventLoop->apidata;
    int retval, j, numevents = 0;

    memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
    memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

    retval = select(eventLoop->maxfd + 1,
                    &state->_rfds, &state->_wfds, NULL, tvp);
    if (retval > 0)
    {
        for (j = 0; j <= eventLoop->maxfd; j++)
        {
            int mask = 0;
            MpFileEvent *fe = &eventLoop->events[j];

            if (fe->mask == MP_NONE)
                continue;
            if (fe->mask & MP_READABLE && FD_ISSET(j, &state->_rfds))
                mask |= MP_READABLE;
            if (fe->mask & MP_WRITABLE && FD_ISSET(j, &state->_wfds))
                mask |= MP_WRITABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

static char *MpApiName(void)
{
    return "select";
}
