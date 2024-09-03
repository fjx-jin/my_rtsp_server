#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h>
#include <map>
#include "event.h"
#include "timer.h"
#define EPOLL_MAX_NUMS 1024
#define EPOLL_TIME_OUT 1

class EPoller
{
public:
    static EPoller* instance(){return m_instance;}
    
    int getPollFd() const {return m_epollFd;}
    bool addEvent(IOEvent* ev);
    bool updateEvent(IOEvent* ev);
    bool removeEvent(IOEvent* ev);
    void handleEvent();
    void loop();

    Timer::TimerId addTimerEventRunEvery(TimerEvent* event,Timer::TimeInterval interval);
    void removeTimeEvent(Timer::TimerId id);
private:
    EPoller();
    ~EPoller();

    static EPoller* m_instance;
    epoll_event events[EPOLL_MAX_NUMS];
    int m_epollFd;
    std::map<int,IOEvent*> m_eventMap;
    TimerManager* m_timeManager;
};
#endif