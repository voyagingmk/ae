
#include "logger/logger.h"
#include "logger/log_file.h"

namespace wynet
{
Logger::Logger(const string &logtitle,
               off_t rollSize,
               int flushInterval) : m_flushInterval(flushInterval),
                                    m_running(false),
                                    m_logtitle(logtitle),
                                    m_rollSize(rollSize),
                                    m_thread(std::bind(&Logger::threadEntry, this), "Logging"),
                                    m_latch(1),
                                    m_mutex(),
                                    m_cond(m_mutex),
                                    m_curBuffer(new LoggingBuffer),
                                    m_nextBuffer(new LoggingBuffer),
                                    m_fulledBuffers()
{
    m_fulledBuffers.reserve(16);
    assert(m_curBuffer);
    assert(m_nextBuffer);
    m_curBuffer->clean();
    m_nextBuffer->clean();
}

Logger::~Logger()
{
    if (m_running)
    {
        stop();
    }
}

void Logger::append(const char *logline, int len)
{
    MutexLockGuard<MutexLock> lock(m_mutex);
    assert(m_curBuffer);
    if (m_curBuffer->leftOpacity() > len)
    {
        m_curBuffer->append(logline, len);
    }
    else
    {
        m_fulledBuffers.push_back(std::move(m_curBuffer));
        assert(!m_curBuffer);
        updateCurBuffer();
        assert(m_curBuffer);
        m_curBuffer->append(logline, len);
        m_cond.notify(); // 满了才notify
    }
}

void Logger::updateCurBuffer()
{
    if (m_curBuffer)
    {
        return;
    }
    assert(m_nextBuffer);
    m_curBuffer = std::move(m_nextBuffer);
    m_curBuffer->clean();
    if (m_freeBuffers.size() > 0)
    {
        m_nextBuffer = std::move(m_freeBuffers.back());
        m_freeBuffers.pop_back();
        m_nextBuffer->clean();
    }
    else
    {
        m_nextBuffer.reset(new LoggingBuffer());
    }
    assert(m_nextBuffer);
    assert(m_curBuffer->usedLength() == 0);
    assert(m_nextBuffer->usedLength() == 0);
}

void Logger::start()
{
    m_running = true;
    m_thread.start();
    m_latch.wait();
}

void Logger::stop()
{
    m_running = false;
    m_cond.notify();
    m_thread.join();
}

void Logger::threadEntry()
{

    assert(m_running == true);
    m_latch.countDown();
    LogFile output(m_logtitle, m_rollSize, false);
    BufferPtrVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (m_running)
    {
        {
            MutexLockGuard<MutexLock> lock(m_mutex);
            if (m_fulledBuffers.empty()) // unusual usage!
            {
                m_cond.waitForSeconds(m_flushInterval);
            }
            m_fulledBuffers.push_back(std::move(m_curBuffer));
            assert(!m_curBuffer);
            updateCurBuffer();
            assert(m_curBuffer);
            buffersToWrite.swap(m_fulledBuffers);
        }

        assert(!buffersToWrite.empty());
        for (size_t i = 0; i < buffersToWrite.size(); i++)
        {
            output.append((const char *)buffersToWrite[i]->data(), buffersToWrite[i]->usedLength());
        }
        while (buffersToWrite.size() > 0)
        {
            m_freeBuffers.push_back(std::move(buffersToWrite.back()));
            buffersToWrite.pop_back();
        }
        buffersToWrite.clear();
        output.flush();
    }
}

void Logger::threadEntry2()
{
    assert(m_running == true);
    m_latch.countDown();
    LogFile output(m_logtitle, m_rollSize, false);
    BufferUniquePtr newBuffer1(new LoggingBuffer);
    BufferUniquePtr newBuffer2(new LoggingBuffer);
    newBuffer1->clean();
    newBuffer2->clean();
    BufferPtrVector buffersToWrite;
    buffersToWrite.reserve(16);
    while (m_running)
    {
        assert(newBuffer1);
        assert(newBuffer1->usedLength() == 0);
        assert(newBuffer2);
        assert(newBuffer2->usedLength() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard<MutexLock> lock(m_mutex);
            if (m_fulledBuffers.empty()) // unusual usage!
            {
                m_cond.waitForSeconds(m_flushInterval);
            }
            // 可能是满了被唤醒，或者超时了唤醒，超时唤醒时m_curBuffer不满
            // 放到full里
            m_fulledBuffers.push_back(std::move(m_curBuffer));
            assert(!m_curBuffer);
            // 给cur分配可用的buffer
            m_curBuffer = std::move(newBuffer1);
            assert(m_curBuffer);
            assert(!newBuffer1);
            buffersToWrite.swap(m_fulledBuffers);
            if (!m_nextBuffer)
            {
                m_nextBuffer = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages, %zd larger buffers\n",
                     buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append((const char *)buffersToWrite[i]->data(), buffersToWrite[i]->usedLength());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->clean();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->clean();
        }
        assert(newBuffer1);
        assert(newBuffer2);
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}
};
