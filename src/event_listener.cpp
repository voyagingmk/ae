#include "event_listener.h"
#include "eventloop.h"

namespace wynet
{

EventListener::EventListener() : m_loop(nullptr),
                                 m_sockfd(0)
{
}

EventListener::~EventListener()
{
}

bool EventListener::hasFileEvent(int mask)
{
    if (m_loop && m_sockfd)
    {
        m_loop->hasFileEvent(shared_from_this(), mask);
    }
    else
    {
        log_error("EventListener::isWaitingFileEvent no m_loop or no m_sockfd");
    }
}

void EventListener::deleteAllFileEvent()
{
    if (m_loop && m_sockfd)
    {
        // log_info("deleteAllFileEvent %d", m_sockfd);
        m_loop->deleteAllFileEvent(m_sockfd);
    }
    else
    {
        log_error("EventListener::deleteAllFileEvent no m_loop or no m_sockfd");
    }
}

void EventListener::createFileEvent(int mask, OnFileEvent onFileEvent)
{
    if (m_loop && m_sockfd)
    {
        // log_info("createFileEvent %d", m_sockfd);
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