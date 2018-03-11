#ifndef WY_LOGFILE_H
#define WY_LOGFILE_H

#include "../mutex.h"
#include "../noncopyable.h"
#include <memory>
#include <string>

namespace wynet
{
using namespace std;

class AppendFile : Noncopyable
{
public:
  explicit AppendFile(std::string &filename);

  ~AppendFile();

  void append(const char *logline, const size_t len);

  void flush();

  off_t writtenBytes() const { return m_writtenBytes; }

private:
  size_t write(const char *logline, size_t len);

  FILE *m_fp;
  char m_buffer[64 * 1024];
  off_t m_writtenBytes;
};

class LogFile : Noncopyable
{
public:
  LogFile(const string &basename,
          off_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char *logline, int len);
  void flush();
  bool rollFile();

private:
  void append_unlocked(const char *logline, int len);

  static string getLogFileName(const string &basename, time_t *now);

  const string m_basename;
  const off_t m_rollSize;
  const int m_flushInterval;
  const int m_checkEveryN;

  int m_count;

  unique_ptr<MutexLock> m_mutex;
  time_t m_startOfPeriod;
  time_t m_lastRoll;
  time_t m_lastFlush;
  unique_ptr<AppendFile> m_file;

  const static int kRollPerSeconds_ = 60 * 60 * 24;
};
};

#endif
