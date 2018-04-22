#ifndef WY_LOG_FILE_H
#define WY_LOG_FILE_H

#include "common.h"
#include "mutex.h"
#include "noncopyable.h"

namespace wynet
{

class AppendFile;

// 日志文件io
class LogFile : Noncopyable
{
public:
  LogFile(const std::string &basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);

  ~LogFile();

  void append(const char *logline, int len);

  void flush();

  void rollFile();

private:
  void append_unlocked(const char *logline, int len);

  std::unique_ptr<AppendFile> &getAppendFile();

  inline time_t getPeriod(time_t &t)
  {
    return t / k_RollPerSeconds * k_RollPerSeconds;
  }

private:
  const std::string m_basename;
  const off_t m_rollSize;
  const int m_flushInterval;
  const int m_checkEveryN;

  int m_logLinesCount;
  std::unique_ptr<MutexLock> m_mutex;
  time_t m_startPeriod;
  time_t m_lastRoll;
  time_t m_lastFlush;
  std::unique_ptr<AppendFile> m_file;

  const static int k_RollPerSeconds = 3600 * 24;
};
};

#endif
