
#include "logger/logger.h"


namespace wynet
{
Logger::Logger(const string& logtitle,
                           off_t rollSize,
                           int flushInterval)
  : m_flushInterval(flushInterval),
    m_running(false),
    m_logtitle(logtitle),
    m_rollSize(rollSize),
   // m_thread(std::bind(&Logger::threadFunc, this), "Logging"),
   // m_latch(1),
    m_mutex(),
    m_cond(m_mutex),
    m_curBuffer(new LoggingBuffer),
    m_nextBuffer(new LoggingBuffer),
    m_buffers()
{
  m_curBuffer->clean();
  m_nextBuffer->clean();
  m_buffers.reserve(16);
}


};
