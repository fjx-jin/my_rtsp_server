#include "event.h"
#include <stdio.h>

IOEvent::IOEvent(epoll_event ev,void* arg) : 
    m_event(ev),
    m_arg(arg),
    m_readCb(NULL),
    m_writeCb(NULL),
    m_errorCb(NULL)
{

}

IOEvent::~IOEvent(){}

void IOEvent::handleEvent()
{
    if((m_event.events & EPOLLIN) && m_readCb)
    {
        m_readCb(m_arg);
    }
    else if((m_event.events & EPOLLOUT) && m_writeCb)
    {
        m_writeCb(m_arg);
    }
        else if((m_event.events & EPOLLERR) && m_errorCb)
    {
        m_errorCb(m_arg);
    }
}

TimerEvent::TimerEvent(void* arg) :
    m_arg(arg),
    m_isStop(false),
    m_timeoutCallback(NULL)
{

}

TimerEvent::~TimerEvent()
{

}

bool TimerEvent::handleEvent()
{
    if(m_isStop)
        return false;
    if(m_timeoutCallback)
        m_timeoutCallback(m_arg);
    return true;
}

void TimerEvent::stop()
{
    m_isStop = true;
}

