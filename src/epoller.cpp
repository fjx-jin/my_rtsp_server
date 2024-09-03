#include "epoller.h"
#include "Log.h"
#include <unistd.h>
EPoller* EPoller::m_instance = new EPoller();
EPoller::EPoller()
{
    m_epollFd = epoll_create(EPOLL_MAX_NUMS);
    m_timeManager =  new TimerManager(this);
}

EPoller::~EPoller()
{
    close(m_epollFd);
}

bool EPoller::addEvent(IOEvent* ev)
{
    struct epoll_event epollEvent = ev->getEvent();
    int fd = epollEvent.data.fd;
    if (fd < 0)
    {
        LOGE("fd = %d\n",fd);
        return false;
    }
    auto it = m_eventMap.find(fd);
    if(it == m_eventMap.end())
    {
        if(epoll_ctl(m_epollFd,EPOLL_CTL_ADD,fd,&epollEvent) < 0)
        {
            LOGE("epoll_ctl error %d\n",fd);
            return false;
        }
        m_eventMap.insert(std::make_pair(fd,ev));
        return true;
    }
    else
    {
        LOGE("already have %d\n",fd);
        return false;
    }
}

bool EPoller::updateEvent(IOEvent* ev)
{
    return false;
}

bool EPoller::removeEvent(IOEvent* ev)
{
    struct epoll_event epollEvent = ev->getEvent();
    int fd = epollEvent.data.fd;
    if (fd < 0)
    {
        LOGE("fd = %d\n",fd);
        return false;
    }
    auto it = m_eventMap.find(fd);
    if (it != m_eventMap.end())
    {
        epoll_ctl(m_epollFd,EPOLL_CTL_DEL,fd,&epollEvent);
        m_eventMap.erase(it);
    }
    return true;
}

void EPoller::handleEvent()
{
    
    int nfds = epoll_wait(m_epollFd,events,EPOLL_MAX_NUMS,EPOLL_TIME_OUT);
    // LOGI("handleEvent\n");
    for(int i = 0;i<nfds;i++)
    {
        
        int fd = events[i].data.fd;
        // if(fd != 4)
            // LOGI("%d\n",fd);
            
        auto it =  m_eventMap.find(fd);
        if (it == m_eventMap.end())
        {
            LOGE("can not find fd = %d\n",fd);
            break;
        } 
        (*it).second->handleEvent();
    }
}

Timer::TimerId EPoller::addTimerEventRunEvery(TimerEvent* event,Timer::TimeInterval interval)
{
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += interval;

    return m_timeManager->addTimer(event, timestamp, interval);
}

void EPoller::removeTimeEvent(Timer::TimerId id)
{
    m_timeManager->removeTimer(id);
    return ;
}

void EPoller::loop()
{
    while(1)
    {
        handleEvent();
        // sleep(1);
    }
}