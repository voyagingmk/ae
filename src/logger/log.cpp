
#include "logger/log.h"
#include "logger/logger.h"
#include <sys/time.h>

namespace wynet
{

LOG_LEVEL initLogLevel()
{
    if (::getenv("LOG_TRACE"))
        return LOG_LEVEL::LOG_TRACE;
    else if (::getenv("LOG_DEBUG"))
        return LOG_LEVEL::LOG_DEBUG;
    else
        return LOG_LEVEL::LOG_INFO;
}

class LogVars
{
  public:
    LogVars() : m_logLevel(initLogLevel()),
                m_logger(nullptr),
                m_logLineInfo(true),
                k_logLineMax(1024)
    {
    }

    const char *getLogLevelStr(LOG_LEVEL level) const
    {
        const char *k_logLevelToStr[] = {
            "TRACE ",
            "DEBUG ",
            "INFO  ",
            "WARN  ",
            "ERROR ",
            "FATAL ",
        };
        return k_logLevelToStr[static_cast<int>(level)];
    }

  public:
    LOG_LEVEL m_logLevel;
    Logger *m_logger;
    bool m_logLineInfo;
    const int k_logLineMax;
    const char *k_logLevelToStr[];
};

static LogVars g_LogVars;

void setLogLevel(LOG_LEVEL level)
{
    g_LogVars.m_logLevel = level;
}

LOG_LEVEL logLevel()
{
    return g_LogVars.m_logLevel;
}

void setLogger(Logger *logger)
{
    g_LogVars.m_logger = logger;
}

void setEnableLogLineInfo(bool enabled)
{
}

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...)
{
    if (!g_LogVars.m_logger)
    {
        return;
    }
    if (level < g_LogVars.m_logLevel)
    {
        return;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm tm_time;
    ::gmtime_r(&tv.tv_sec, &tm_time);

    char buff[g_LogVars.k_logLineMax];

    // 4 2 2 1 2 1 2 1 2 1 6= 24,  24 + '\0' = 25
    snprintf(buff, 25, "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec); // tv_usec 微秒 百万分之一秒
    buff[24] = ' ';
    memcpy(buff + 25, g_LogVars.getLogLevelStr(level), 6);

    const int prefixLength = 25 + 6;
    va_list args;
    va_start(args, fmt);
    char *begin = buff + prefixLength;
    vsnprintf(begin, g_LogVars.k_logLineMax - prefixLength, fmt, args);
    va_end(args);

    g_LogVars.m_logger->append(buff, prefixLength + strlen(begin));
}
}