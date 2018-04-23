#ifndef WY_LOG_H
#define WY_LOG_H

namespace wynet
{

enum class LOG_LEVEL
{
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#ifndef ENABLE_LOG_FILELINE

#define log_trace(...) log_log(wynet::LOG_LEVEL::LOG_TRACE, "", 0, __VA_ARGS__)
#define log_debug(...) log_log(wynet::LOG_LEVEL::LOG_DEBUG, "", 0, __VA_ARGS__)
#define log_info(...) log_log(wynet::LOG_LEVEL::LOG_INFO, "", 0, __VA_ARGS__)
#define log_warn(...) log_log(wynet::LOG_LEVEL::LOG_WARN, "", 0, __VA_ARGS__)
#define log_error(...) log_log(wynet::LOG_LEVEL::LOG_ERROR, "", 0, __VA_ARGS__)
#define log_fatal(...)                                            \
    {                                                             \
        log_log(wynet::LOG_LEVEL::LOG_FATAL, "", 0, __VA_ARGS__); \
        ::abort();                                                \
    }

#else // !ENABLE_LOG_FILELINE

#define log_trace(...) log_log(wynet::LOG_LEVEL::LOG_TRACE, __FILE, __LINE, __VA_ARGS__)
#define log_debug(...) log_log(wynet::LOG_LEVEL::LOG_DEBUG, __FILE, __LINE, __VA_ARGS__)
#define log_info(...) log_log(wynet::LOG_LEVEL::LOG_INFO, __FILE, __LINE, __VA_ARGS__)
#define log_warn(...) log_log(wynet::LOG_LEVEL::LOG_WARN, __FILE, __LINE, __VA_ARGS__)
#define log_error(...) log_log(wynet::LOG_LEVEL::LOG_ERROR, __FILE, __LINE, __VA_ARGS__)
#define log_fatal(...)                                                     \
    {                                                                      \
        log_log(wynet::LOG_LEVEL::LOG_FATAL, __FILE, __LINE, __VA_ARGS__); \
        ::abort();                                                         \
    }

#endif // ENABLE_LOG_FILELINE

class Logger;

void setLogLevel(LOG_LEVEL level);

LOG_LEVEL logLevel();

void setLogger(Logger *logger);

void setEnableLogLineInfo(bool enabled);

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...);
};

#endif