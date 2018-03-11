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
    Logger(const string &basename,
           off_t rollSize,
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
        m_thread.start();
        latch_.wait();
    }

    void stop()
    {
        m_running = false;
        m_cond.notify();
        m_thread.join();
    }

  private:
    void threadFunc();

    typedef std::vector<std::unique_ptr<LoggingBuffer>> BufferVector;
    typedef BufferVector::auto_type BufferPtr;

    const int flushInterval_;
    bool m_running;
    string basename_;
    off_t rollSize_;
    muduo::Thread m_thread;
    muduo::CountDownLatch latch_;
    MutexLock m_mutex;
    Condition m_cond;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};
}
#endif