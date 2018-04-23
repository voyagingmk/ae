
#include "logger/log.h"
#include "logger/logger.h"
#include "wythreadbase.h"
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
                m_outputToConsole(true),
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
    bool m_outputToConsole;
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
    g_LogVars.m_logLineInfo = enabled;
}

void setEnableOutputToConsole(bool enabled)
{

    g_LogVars.m_outputToConsole = enabled;
}

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...)
{
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

    const int prefixN = 40;
    const int postfixN = 200;
    char buff[prefixN + g_LogVars.k_logLineMax + postfixN];
    int n = 0;
    int total = prefixN;
    {
        // 4 + 2 + 2 + 1 + 2 + 1 + 2 + 1 + 2 + 1 + 6 + 1 + 1 + 7 + 1 + 5 = 39
        snprintf(buff, prefixN, "%4d%02d%02d %02d:%02d:%02d.%06dZ %07d %s",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec,
                CurrentThread::tid(),
                g_LogVars.getLogLevelStr(level));
        buff[prefixN - 1] = ' ';
    }
    {
        va_list args;
        va_start(args, fmt);
        n = vsnprintf(buff + prefixN, g_LogVars.k_logLineMax, fmt, args);
        va_end(args);
        buff[prefixN + n] = ' ';
        total += n;
        n = snprintf(buff + prefixN + n, std::max(g_LogVars.k_logLineMax + postfixN - n, postfixN), 
            " - %s:%d\n", _file, _line);
        total += n;
    }

    if (g_LogVars.m_logger)
    {
        g_LogVars.m_logger->append(buff, total);
    }
    if (g_LogVars.m_outputToConsole)
    {
        fprintf(stderr, "%s", buff);
    }
}
}