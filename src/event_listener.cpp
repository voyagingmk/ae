#include "event_listener.h"
#include "eventloop.h"

namespace wynet
{

EventListener::EventListener() : m_loop(nullptr),
                                 m_sockfd(0)
{
    log_debug("EventListener() sockfd %d", m_sockfd);
}

EventListener::~EventListener()
{
    if (m_loop && m_sockfd)
    {
        m_loop->deleteAllFileEvent(m_sockfd);
    }
    m_loop = nullptr;
    m_sockfd = 0;
}

void EventListener::createFileEvent(int mask, OnFileEvent onFileEvent)
{
    if (m_loop && m_sockfd)
    {
        m_onFileEvent = onFileEvent;
        m_loop->createFileEvent(shared_from_this(), mask);
    }
    else
    {
        log_error("EventListener::createFileEvent no m_loop or no m_sockfd");
    }
}

void EventListener::deleteFileEvent(int mask)
{
    if (m_loop && m_sockfd)
    {
        m_loop->deleteFileEvent(getSockFd(), mask);
    }
    else
    {
        log_error("EventListener::deleteFileEvent no m_loop or no m_sockfd");
    }
}

TimerRef EventListener::createTimer(int ms, OnTimerEvent onTimerEvent, void *data)
{
    if (m_loop)
    {
        return m_loop->createTimer(shared_from_this(), ms, onTimerEvent, data);
    }
    else
    {
        log_error("EventListener::createTimer no m_loop");
        return TimerRef(0);
    }
}
}; // namespace wynet