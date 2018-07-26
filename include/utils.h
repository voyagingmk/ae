#ifndef WY_UTILS_H
#define WY_UTILS_H

#include "common.h"
#include "logger/log.h"

namespace wynet
{

int little_endian();

int big_endian();

void ignoreSignalPipe();

std::string hostname();

void checkOpenFileNum(int expectedNum);

// #define ENABLE_TIME_MEASURE 1

#ifdef ENABLE_TIME_MEASURE
#define log_timemeasure(x) TimeMeasure measure(x);
#else
#define log_timemeasure(x)
#endif

// #define LOG_CTOR_DTOR 1
#ifdef LOG_CTOR_DTOR
#define __str__(x) #x
#define log_ctor(...) log_info(__VA_ARGS__);
#define log_dtor(...) log_info(__VA_ARGS__);
#define ctor_dtor_forlogging(classname)  \
    classname()                          \
    {                                    \
        log_info(__str__(classname()));  \
    }                                    \
    ~classname()                         \
    {                                    \
        log_info(__str__(~classname())); \
    }
#else
#define log_ctor(...)
#define log_dtor(...)
#define ctor_dtor_forlogging(classname)
#endif

}; // namespace wynet

#endif
