#ifndef WY_LOGGER_H
#define WY_LOGGER_H

#include <string>
#include <sys/types.h>
#include "../noncopyable.h"
#include "../mutex.h"
#include "logging_buffer.h"


namespace wynet
{
using namespace std;

class Logger : Noncopyable
{
  public:
    Logger(const string &logtitle,
           off_t rollSize = 500 * 1024 * 1024,
           int flushInterval = 3);

    ~Logger()
    {
        if (m_running)
        {
            stop();
        }
    }

    void append(const char *logline, int len);

    void start()
    {
        m_running = true;
      //  m_thread.start();
       // m_latch.wait();
    }

    void stop()
    {
        m_running = false;
        m_cond.notify();
      //  m_thread.join();
    }

  private:
    void threadFunc();

    typedef std::unique_ptr<LoggingBuffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferVector;

    const int m_flushInterval;
    bool m_running;
    string m_logtitle;
    off_t m_rollSize;
   // Thread m_thread;
   // CountDownLatch m_latch;
    MutexLock m_mutex;
    Condition m_cond;
    BufferPtr m_curBuffer;
    BufferPtr m_nextBuffer;
    BufferVector m_buffers;
};
}
#endif
