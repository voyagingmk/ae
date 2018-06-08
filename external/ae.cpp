/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "ae.h"
extern "C"
{
#include "zmalloc.h"
}
#include "config.h"

/* Include the best multiplexing layer supported by this system.
 * The following should be ordered by performances, descending. */
#ifdef HAVE_EVPORT
#include "ae_evport.cpp"
#else
#ifdef HAVE_EPOLL
#include "ae_epoll.cpp"
#else
#ifdef HAVE_KQUEUE
#include "ae_kqueue.cpp"
#else
#include "ae_select.cpp"
#endif
#endif
#endif

bool GreateThanByTime::operator()(const aeTimeEventPtr &lhs, const aeTimeEventPtr &rhs) const
{
    bool b;
    if (lhs->when_sec == rhs->when_sec)
    {
        b = lhs->when_ms < rhs->when_ms;
    }
    else
    {
        b = lhs->when_sec < rhs->when_sec;
    }
    return b;
}

static aeTimeEventPtr aeSearchNearestTimer(aeEventLoop *eventLoop);

aeEventLoop *aeCreateEventLoop(int setsize)
{
    aeEventLoop *eventLoop;
    int i;

    if ((eventLoop = new aeEventLoop()) == NULL)
        goto err;
    eventLoop->events = (aeFileEvent *)zmalloc(sizeof(aeFileEvent) * setsize);
    eventLoop->fired = (aeFiredEvent *)zmalloc(sizeof(aeFiredEvent) * setsize);
    if (eventLoop->events == NULL || eventLoop->fired == NULL)
        goto err;
    eventLoop->setsize = setsize;
    eventLoop->lastTime = time(NULL);
    eventLoop->timeEventNextId = 0;
    eventLoop->stop = 0;
    eventLoop->maxfd = -1;
    eventLoop->beforesleep = NULL;
    eventLoop->aftersleep = NULL;
    if (aeApiCreate(eventLoop) == -1)
        goto err;
    /* Events with mask == AE_NONE are not set. So let's initialize the
     * vector with it. */
    for (i = 0; i < setsize; i++)
        eventLoop->events[i].mask = AE_NONE;
    return eventLoop;

err:
    if (eventLoop)
    {
        zfree(eventLoop->events);
        zfree(eventLoop->fired);
        zfree(eventLoop);
    }
    return NULL;
}

/* Return the current set size. */
int aeGetSetSize(aeEventLoop *eventLoop)
{
    return eventLoop->setsize;
}

/* Resize the maximum set size of the event loop.
 * If the requested set size is smaller than the current set size, but
 * there is already a file descriptor in use that is >= the requested
 * set size minus one, AE_ERR is returned and the operation is not
 * performed at all.
 *
 * Otherwise AE_OK is returned and the operation is successful. */
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize)
{
    int i;

    if (setsize == eventLoop->setsize)
        return AE_OK;
    if (eventLoop->maxfd >= setsize)
        return AE_ERR;
    if (aeApiResize(eventLoop, setsize) == -1)
        return AE_ERR;

    eventLoop->events = (aeFileEvent *)zrealloc(eventLoop->events, sizeof(aeFileEvent) * setsize);
    eventLoop->fired = (aeFiredEvent *)zrealloc(eventLoop->fired, sizeof(aeFiredEvent) * setsize);
    eventLoop->setsize = setsize;

    /* Make sure that if we created new slots, they are initialized with
     * an AE_NONE mask. */
    for (i = eventLoop->maxfd + 1; i < setsize; i++)
        eventLoop->events[i].mask = AE_NONE;
    return AE_OK;
}

void aeDeleteEventLoop(aeEventLoop *eventLoop)
{
    aeApiFree(eventLoop);
    zfree(eventLoop->events);
    zfree(eventLoop->fired);
    delete eventLoop;
}

void aeStop(aeEventLoop *eventLoop)
{
    eventLoop->stop = 1;
}

int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
                      aeFileProc *proc, void *clientData)
{
    if (fd >= eventLoop->setsize)
    {
        errno = ERANGE;
        return AE_ERR;
    }
    aeFileEvent *fe = &eventLoop->events[fd];

    if (aeApiAddEvent(eventLoop, fd, mask) == -1)
        return AE_ERR;
    fe->mask |= mask;
    if (mask & AE_READABLE)
        fe->rfileProc = proc;
    if (mask & AE_WRITABLE)
        fe->wfileProc = proc;
    fe->clientData = clientData;
    if (fd > eventLoop->maxfd)
        eventLoop->maxfd = fd;
    return AE_OK;
}

void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
{
    if (fd >= eventLoop->setsize)
        return;
    aeFileEvent *fe = &eventLoop->events[fd];
    if (fe->mask == AE_NONE)
        return;

    aeApiDelEvent(eventLoop, fd, mask);
    fe->mask = fe->mask & (~mask);
    if (fd == eventLoop->maxfd && fe->mask == AE_NONE)
    {
        /* Update the max fd */
        int j;

        for (j = eventLoop->maxfd - 1; j >= 0; j--)
            if (eventLoop->events[j].mask != AE_NONE)
                break;
        eventLoop->maxfd = j;
    }
}

int aeGetFileEvents(aeEventLoop *eventLoop, int fd)
{
    if (fd >= eventLoop->setsize)
        return 0;
    aeFileEvent *fe = &eventLoop->events[fd];

    return fe->mask;
}

static void aeGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec / 1000;
}

static void aeAddMillisecondsToNow(long now_sec, long now_ms, long long milliseconds, long *sec, long *ms)
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

long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
                            aeTimeProc *proc, void *clientData,
                            aeEventFinalizerProc *finalizerProc)
{
    long long id = eventLoop->timeEventNextId++;
    aeTimeEventPtr te;

    te = std::make_shared<aeTimeEvent>();
    if (te == NULL)
        return AE_ERR;
    te->id = id;
    long now_sec, now_ms;
    aeGetTime(&now_sec, &now_ms);
    aeAddMillisecondsToNow(now_sec, now_ms, milliseconds, &te->when_sec, &te->when_ms);
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    eventLoop->pq.insert(te);
    eventLoop->timeEventNearest = aeSearchNearestTimer(eventLoop);
    return id;
}

int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id)
{
    auto it = std::find_if(
        eventLoop->pq.begin(), eventLoop->pq.end(),
        [&id](const aeTimeEventPtr &x) { return (x->id) == id; });
    if (it != eventLoop->pq.end())
    {
        (*it)->id = AE_DELETED_EVENT_ID;
        eventLoop->timeEventNearest = aeSearchNearestTimer(eventLoop);
        return AE_OK;
    }
    return AE_ERR; /* NO event with the specified ID found */
}

/* Search the first timer to fire.
 * This operation is useful to know how many time the select can be
 * put in sleep without to delay any event.
 * If there are no timers NULL is returned.
 *
 * Note that's O(N) since time events are unsorted.
 * Possible optimizations (not needed by Redis so far, but...):
 * 1) Insert the event in order, so that the nearest is just the head.
 *    Much better but still insertion or deletion of timers is O(N).
 * 2) Use a skiplist to have this operation as O(1) and insertion as O(log(N)).
 */
static aeTimeEventPtr aeSearchNearestTimer(aeEventLoop *eventLoop)
{
    aeTimeEventPtr nearest = nullptr;
    if (eventLoop->pq.size() > 0)
    {
        for (auto it = eventLoop->pq.begin(); it != eventLoop->pq.end(); it++)
        {
            nearest = *it;
            if (nearest->id != AE_DELETED_EVENT_ID)
            {
                break;
            }
        }
        /*
        int num = 0;
        for (auto it = eventLoop->pq.begin(); it != eventLoop->pq.end(); it++)
        {
            fprintf(stderr, "[%d] sec %d ms %d\n", num++, (int)((*it)->when_sec), (int)((*it)->when_ms));
        }*/
    }
    return nearest;
}

/* Process time events */
static int processTimeEvents(aeEventLoop *eventLoop)
{
    int processed = 0;
    aeTimeEventPtr te;
    long long maxId;
    time_t now = time(NULL);

    eventLoop->lastTime = now;

    maxId = eventLoop->timeEventNextId - 1;

    aeEventLoop::PriorityQueue::iterator it;
    std::vector<aeTimeEventPtr> teListDeleted;
    std::vector<aeTimeEventPtr> teListTimeout;
    it = eventLoop->pq.begin();
    while (it != eventLoop->pq.end())
    {
        const aeTimeEventPtr &_te = *it;
        if (_te->id == AE_DELETED_EVENT_ID)
        {
            teListDeleted.push_back(_te);
        }
        it++;
    }
    if (teListDeleted.size() > 0)
    {
        for (auto it2 = teListDeleted.begin(); it2 != teListDeleted.end(); it2++)
        {
            aeTimeEventPtr &_te = *it2;
            auto it3 = eventLoop->pq.find(_te);
            assert(it3 != eventLoop->pq.end());
            eventLoop->pq.erase(it3);
        }
    }
    for (auto it2 = teListDeleted.begin(); it2 != teListDeleted.end(); it2++)
    {
        te = *it2;
        if (te->finalizerProc)
            te->finalizerProc(eventLoop, te->clientData);
    }
    long now_sec, now_ms;
    aeGetTime(&now_sec, &now_ms);
    std::vector<aeTimeEventPtr> teListReinsert;
    for (it = eventLoop->pq.begin(); it != eventLoop->pq.end(); it++)
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
        aeTimeEventPtr te = *itte;
        /*
        fprintf(stderr, "remove te %d\n", int(te->id));
        fprintf(stderr, "size: %d\n", int(eventLoop->pq.size()));
        auto it2 = eventLoop->pq.find(te);
        assert(it2 != eventLoop->pq.end());
        eventLoop->pq.erase(it2);
        */
        int retval;
        auto id = te->id;
        retval = te->timeProc(eventLoop, id, te->clientData);
        processed++;
        if (retval != AE_NOMORE)
        {
            aeAddMillisecondsToNow(now_sec, now_ms, retval, &te->when_sec, &te->when_ms);
            teListReinsert.push_back(te);
        }
    }
    for (auto itte = teListReinsert.begin(); itte != teListReinsert.end(); itte++)
    {
        aeTimeEventPtr te = *itte;
        // fprintf(stderr, "insert te %d\n", int(te->id));
        eventLoop->pq.insert(te);
    }

    if (processed > 0)
    {
        eventLoop->timeEventNearest = aeSearchNearestTimer(eventLoop);
    }
    return processed;
}

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * if flags has AE_DONT_WAIT set the function returns ASAP until all
 * if flags has AE_CALL_AFTER_SLEEP set, the aftersleep callback is called.
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    int processed = 0, numevents;

    /* Nothing to do? return ASAP */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS))
        return 0;

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (eventLoop->maxfd != -1 ||
        ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT)))
    {
        int j;
        aeTimeEventPtr shortest = nullptr;
        struct timeval tv, *tvp;

        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            shortest = eventLoop->timeEventNearest;
        if (shortest)
        {
            long now_sec, now_ms;

            aeGetTime(&now_sec, &now_ms);
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
            //  fprintf(stderr, "wait %d %d\n", tvp->tv_sec, tvp->tv_usec);
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
        numevents = aeApiPoll(eventLoop, tvp);

        /* After sleep callback. */
        if (eventLoop->aftersleep != NULL && flags & AE_CALL_AFTER_SLEEP)
            eventLoop->aftersleep(eventLoop);

        for (j = 0; j < numevents; j++)
        {
            int fd = eventLoop->fired[j].fd;
            int mask = eventLoop->fired[j].mask;
            aeFileEvent *fe = &eventLoop->events[fd];
            int fe_mask = fe->mask;
            aeFileProc *rfileProc = fe->rfileProc;
            aeFileProc *wfileProc = fe->wfileProc;
            void *clientData = fe->clientData;
            int rfired = 0;

            /* note the fe->mask & mask & ... code: maybe an already processed
             * event removed an element that fired and we still didn't
             * processed, so we check if the event is still valid. */
            if (fe_mask & mask & AE_READABLE)
            {
                rfired = 1;
                rfileProc(eventLoop, fd, clientData, mask);
            }
            if (fe_mask & mask & AE_WRITABLE)
            {
                if (!rfired || wfileProc != rfileProc)
                    wfileProc(eventLoop, fd, clientData, mask);
            }
            processed++;
        }
    }
    /* Check time events */
    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents(eventLoop);

    return processed; /* return the number of processed file/time events */
}

/* Wait for milliseconds until the given file descriptor becomes
 * writable/readable/exception */
int aeWait(int fd, int mask, long long milliseconds)
{
    struct pollfd pfd;
    int retmask = 0, retval;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    if (mask & AE_READABLE)
        pfd.events |= POLLIN;
    if (mask & AE_WRITABLE)
        pfd.events |= POLLOUT;

    if ((retval = poll(&pfd, 1, milliseconds)) == 1)
    {
        if (pfd.revents & POLLIN)
            retmask |= AE_READABLE;
        if (pfd.revents & POLLOUT)
            retmask |= AE_WRITABLE;
        if (pfd.revents & POLLERR)
            retmask |= AE_WRITABLE;
        if (pfd.revents & POLLHUP)
            retmask |= AE_WRITABLE;
        return retmask;
    }
    else
    {
        return retval;
    }
}

void aeMain(aeEventLoop *eventLoop)
{
    eventLoop->stop = 0;
    while (!eventLoop->stop)
    {
        if (eventLoop->beforesleep != NULL)
            eventLoop->beforesleep(eventLoop);
        aeProcessEvents(eventLoop, AE_ALL_EVENTS | AE_CALL_AFTER_SLEEP);
    }
}

char *aeGetApiName(void)
{
    return aeApiName();
}

void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep)
{
    eventLoop->beforesleep = beforesleep;
}

void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep)
{
    eventLoop->aftersleep = aftersleep;
}
