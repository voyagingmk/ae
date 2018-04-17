#ifndef WY_THREADBASE_H
#define WY_THREADBASE_H

#include "common.h"

namespace wynet
{

namespace CurrentThread
{
extern __thread pid_t t_tidCached;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

pid_t gettid();

void cacheTid();

inline pid_t tid()
{
    if (G_UNLIKELY(t_tidCached == 0))
    {
        cacheTid();
    }
    return t_tidCached;
}

pid_t mainThreadId();

inline const char *tidString()
{
    return t_tidString;
}

inline int tidStringLength()
{
    return t_tidStringLength;
}

inline const char *name()
{
    return t_threadName;
}

bool isMainThread();
};
};

#endif