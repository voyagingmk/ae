/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#define LOG_VERSION "0.1.0"

typedef void (*log_LockFn)(void *udata, int lock);

enum
{
    LOG_TRACE__,
    LOG_DEBUG__,
    LOG_INFO__,
    LOG_WARN__,
    LOG_ERROR__,
    LOG_FATAL__
};

#ifndef ENABLE_LOG_FILELINE

#ifdef DEBUG_MODE
#define log_trace(...) log_log(LOG_TRACE__, "", 0, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG__, "", 0, __VA_ARGS__)
#else
#define log_trace(...)
#define log_debug(...)
#endif
#define log_info(...) log_log(LOG_INFO__, "", 0, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN__, "", 0, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR__, "", 0, __VA_ARGS__)
#define log_fatal(...)                            \
    {                                             \
        log_log(LOG_FATAL__, "", 0, __VA_ARGS__); \
        ::abort();                                \
    }
#else

#ifdef DEBUG_MODE
#define log_trace(...) log_log(LOG_TRACE__, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_trace(...)
#define log_debug(...)
#endif
#define log_info(...) log_log(LOG_INFO__, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOG_WARN__, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR__, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...)                                         \
    {                                                          \
        log_log(LOG_FATAL__, __FILE__, __LINE__, __VA_ARGS__); \
        ::abort();                                             \
    }

#endif

void log_set_udata(void *udata);
void log_set_lock(log_LockFn fn);
void log_set_fp(FILE *fp);
void log_set_file(const char *filename, const char *mode);
void log_close_file();
void log_set_level(int level);
void log_set_quiet(int enable);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif
