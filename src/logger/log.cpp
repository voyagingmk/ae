
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
            "TRACE",
            "DEBUG",
            "INFO ",
            "WARN ",
            "ERROR",
            "FATAL",
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

    const char *_file = g_LogVars.m_logLineInfo ? file : "?";
    int _line = g_LogVars.m_logLineInfo ? line : 0;

    struct tm tm_time;
    ::gmtime_r(&tv.tv_sec, &tm_time);

    char buff[g_LogVars.k_logLineMax];

    int n = snprintf(buff, g_LogVars.k_logLineMax, "%4d%02d%02d %02d:%02d:%02d.%06d %s:%d",
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                     tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec, _file, _line);
    buff[n] = ' ';
    const int prefixLength = n + 1;
    va_list args;
    va_start(args, fmt);
    char *content = buff + prefixLength;
    const int availableLength = g_LogVars.k_logLineMax - prefixLength;
    n = vsnprintf(content, availableLength, fmt, args);
    va_end(args);
    *(content + n) = '\n';
    g_LogVars.m_logger->append(buff, prefixLength + n + 1);
}
}