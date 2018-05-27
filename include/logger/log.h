#ifndef WY_LOG_H
#define WY_LOG_H

#include "common.h"

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

#ifdef DEBUG_MODE
#define log_trace(...) log_log(wynet::LOG_LEVEL::LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(wynet::LOG_LEVEL::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_trace(...)
#define log_debug(...)
#endif
#define log_info(...) log_log(wynet::LOG_LEVEL::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(wynet::LOG_LEVEL::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(wynet::LOG_LEVEL::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...)                                                         \
    {                                                                          \
        log_log(wynet::LOG_LEVEL::LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); \
        ::abort();                                                             \
    }

#define log_file(filename)                                                                        \
    static std::shared_ptr<wynet::Logger> __logger__ = std::make_shared<wynet::Logger>(filename); \
    setLogger(__logger__);

#define log_file_start() __logger__->start();

#define log_level(level) setLogLevel(level);

#define log_lineinfo(enabled) setEnableLogLineInfo(enabled);

#define log_console(enabled) setEnableOutputToConsole(enabled);

void log_log(LOG_LEVEL level, const char *file, int line, const char *fmt, ...);

// ----- shouldn't call directly  ----------

class Logger;

void setLogLevel(LOG_LEVEL level);

LOG_LEVEL logLevel();

void setLogger(std::shared_ptr<Logger> logger);

// Note: change setting before log_file_start

void setEnableLogLineInfo(bool enabled);

void setEnableOutputToConsole(bool enabled);
}; // namespace wynet

#endif