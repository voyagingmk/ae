#ifndef WY_UTILS_H
#define WY_UTILS_H

#include "common.h"
#include "logger/log.h"

std::string hostname();

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

#endif
