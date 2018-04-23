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

#define log_trace(...) log_log(wynet::LOG_LEVEL::LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(wynet::LOG_LEVEL::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(wynet::LOG_LEVEL::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(wynet::LOG_LEVEL::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(wynet::LOG_LEVEL::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...)                                                         \
    {                                                                          \
        log_log(wynet::LOG_LEVEL::LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); \
        ::abort();                                                             \
    }

#define log_setting(filename, level)    \
    wynet::Logger __logger__(filename); \
    setLogger(&__logger__);             \
    setLogLevel(level);

#define log_start() __logger__.start();

#define log_lineinfo(enabled) setEnableLogLineInfo(enabled);

#define log_console(enabled) setEnableOutputToConsole(enabled);

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...);

// ----- shouldn't call directly  ----------

class Logger;

void setLogLevel(LOG_LEVEL level);

LOG_LEVEL logLevel();

void setLogger(Logger *logger);

// Note: change setting before log_start

void setEnableLogLineInfo(bool enabled);

void setEnableOutputToConsole(bool enabled);
};

#endif