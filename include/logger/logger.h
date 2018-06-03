#ifndef WY_LOGGER_H
#define WY_LOGGER_H

#include "common.h"
#include "noncopyable.h"
#include "mutex.h"
#include "thread.h"
#include "count_down_latch.h"
#include "logger/logging_buffer.h"

namespace wynet
{

class Logger : Noncopyable
{
public:
  Logger(const std::string &logtitle,
         off_t rollSize = 32 * 1000 * 1000,
         int flushInterval = 3);

  ~Logger();

  void append(const char *logline, size_t len);

  void start();

  void stop();

private:
  void threadEntry();

  void updateCurBuffer();

private:
  using UniquePtrBuffer = std::unique_ptr<LoggingBuffer>;
  using UniquePtrBuffers = std::vector<UniquePtrBuffer>;

  const int m_flushInterval;
  bool m_running;
  std::string m_logtitle;
  off_t m_rollSize;
  Thread m_thread;
  CountDownLatch m_latch;
  MutexLock m_mutex;
  Condition m_cond;
  UniquePtrBuffer m_curBuffer;
  UniquePtrBuffer m_nextBuffer;
  UniquePtrBuffers m_fulledBuffers;
  UniquePtrBuffers m_freeBuffers;
};
} // namespace wynet
#endif
