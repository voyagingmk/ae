
#include "logger/log.h"
#include "logger/logger.h"
#include <sys/time.h>

namespace wynet
{

const char *LOG_LEVEL_to_str[] = {
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

LOG_LEVEL initLogLevel()
{
    if (::getenv("LOG_TRACE"))
        return LOG_LEVEL::LOG_TRACE;
    else if (::getenv("LOG_DEBUG"))
        return LOG_LEVEL::LOG_DEBUG;
    else
        return LOG_LEVEL::LOG_INFO;
}

LOG_LEVEL g_logLevel = initLogLevel();

Logger *g_logger = nullptr;

const int k_lineBuffer = 1024;

void setLogLevel(LOG_LEVEL level)
{
    g_logLevel = level;
}

void setLogger(Logger *logger)
{
    g_logger = logger;
}

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...)
{
    if (!g_logger)
    {
        return;
    }
    if (level < g_logLevel)
    {
        return;
    }
    const int64_t k_MicroSecondsPerSecond = 1000 * 1000;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t microseconds = tv.tv_sec * k_MicroSecondsPerSecond + tv.tv_usec; // tv_usec 微秒 百万分之一秒

    struct tm tm_time;
    ::gmtime_r(&tv.tv_sec, &tm_time);

    char buff[k_lineBuffer];

    snprintf(buff, 32, "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

    memcpy(buff + 32, LOG_LEVEL_to_str[static_cast<int>(level)], 6);
    const int offset = 32 + 6;

    va_list args;
    va_start(args, fmt);
    snprintf(buff + offset, k_lineBuffer - offset, fmt, args);
    va_end(args);
    g_logger->append(buff, strlen(buff));
}
}