#ifndef WY_LOGGER_H
#define WY_LOGGER_H

#include "../common.h"
#include "../noncopyable.h"
#include "../mutex.h"
#include "../wythread.h"
#include "../count_down_latch.h"
#include "logger/logging_buffer.h"

namespace wynet
{
using namespace std;

class Logger : Noncopyable
{
public:
  Logger(const string &logtitle,
         off_t rollSize = 500 * 1024 * 1024,
         int flushInterval = 3);

  ~Logger();

  void append(const char *logline, int len);

  void start();

  void stop();

private:
  void threadEntry();

  typedef std::unique_ptr<LoggingBuffer> BufferUniquePtr;
  typedef std::vector<BufferUniquePtr> BufferPtrVector;

  const int m_flushInterval;
  bool m_running;
  string m_logtitle;
  off_t m_rollSize;
  Thread m_thread;
  CountDownLatch m_latch;
  MutexLock m_mutex;
  Condition m_cond;
  BufferUniquePtr m_curBuffer;
  BufferUniquePtr m_nextBuffer;
  BufferPtrVector m_fulledBuffers;
};
}
#endif
