#include "timer.h"
#include "event.h"
#include "epoller.h"
#include "Log.h"
#include <time.h>
#include <chrono>
#include <sys/timerfd.h>
#include <unistd.h>

static bool timerFdSetTime(int fd, Timer::Timestamp when, Timer::TimeInterval period) 
{
    struct itimerspec newVal;

    newVal.it_value.tv_sec = when / 1000; //ms->s
    newVal.it_value.tv_nsec = when % 1000 * 1000 * 1000; //ms->ns
    newVal.it_interval.tv_sec = period / 1000;
    newVal.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    int oldValue = timerfd_settime(fd, TFD_TIMER_ABSTIME, &newVal, NULL);
    if (oldValue < 0) {
        LOGE("timerfd_settime error\n");
        return false;
    }
    return true;
}

Timer::Timer(TimerEvent* event,Timestamp timestamp,TimeInterval timeInterval,TimerId timerId) :
    m_timerEvent(event),
    m_timestamp(timestamp),
    m_timeInterval(timeInterval),
    m_timerId(timerId)
{
    m_repeat = timeInterval > 0?true:false;
}

Timer::~Timer()
{
    //TODO
}

Timer::Timestamp Timer::getCurTime()
{
    struct timespec now;// tv_sec (s) tv_nsec (ns-纳秒)
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec*1000 + now.tv_nsec/1000000);
}

Timer::Timestamp Timer::getCurTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).
        count();
}

bool Timer::handleEvent()
{
    if(!m_timerEvent)
        return false;
    
    return m_timerEvent->handleEvent();
}

TimerManager::TimerManager(EPoller* poller) :
    m_poller(poller),
    m_lastTimerId(0)
{
    m_timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(m_timerfd < 0)
    {
        LOGE("create timerfd error\n");
        return;
    }
    LOGI("create timerfd %d\n",m_timerfd);

    epoll_event ev;
    ev.data.fd = m_timerfd;
    m_ioEvent = new IOEvent(ev,this);
    m_ioEvent->SetReadCallback(readCallback);
    m_ioEvent->enableReadHandling();
    modifyTimeout();
    m_poller->addEvent(m_ioEvent);
}

TimerManager::~TimerManager()
{
    //TODO
    close(m_timerfd);
}

Timer::TimerId TimerManager::addTimer(TimerEvent* event,Timer::Timestamp timestamp,Timer::TimeInterval interval)
{
    ++m_lastTimerId;
    Timer timer(event,timestamp,interval,m_lastTimerId);
    m_timers.insert(std::make_pair(m_lastTimerId,timer));
    m_events.insert(std::make_pair(timestamp,timer));
    modifyTimeout();
    return m_lastTimerId;
}

bool TimerManager::removeTimer(Timer::TimerId timerId)
{
    auto it = m_timers.find(timerId);
    if(it != m_timers.end())
    {
        m_timers.erase(it);
        //TODO 还需要删除m_events里的timer
        auto it1 = m_events.begin();
        while(it1 != m_events.end())
        {
            Timer tmpTimer = (*it1).second;
            if(tmpTimer.m_timerId == timerId)
            {
                m_events.erase(it1);
                break;
            }
            it1++;
        }
    }

    modifyTimeout();

    return true;
}

void TimerManager::modifyTimeout()
{
    auto it = m_events.begin();
    if (it != m_events.end()) {// 存在至少一个定时器
        Timer timer = it->second;
        timerFdSetTime(m_timerfd, timer.m_timestamp, timer.m_timeInterval);
        // LOGI("timer start\n");
    }
    else {
        timerFdSetTime(m_timerfd, 0, 0);
        // LOGI("timer stop\n");
    }
}

void TimerManager::readCallback(void *arg)
{
    TimerManager* timerManager = (TimerManager*)arg;
    timerManager->handleRead();
}

void TimerManager::handleRead()
{
    m_poller->removeEvent(m_ioEvent);
    // LOGI("handleRead %d %d\n",m_timers.size(),m_events.size());
    Timer::Timestamp timestamp = Timer::getCurTime();
    if(!m_timers.empty() && !m_events.empty())
    {
        auto it =m_events.begin();
        Timer timer = it->second;
        int expire = timer.m_timestamp - timestamp;
        // LOGI("%ld %ld",timer.m_timestamp,timestamp);
        if(timestamp > timer.m_timestamp || expire == 0)
        {
            timer.handleEvent();
            m_events.erase(it);
            // LOGI("repeta %d\n",timer.m_repeat);
            if(timer.m_repeat)
            {
                timer.m_timestamp = timestamp + timer.m_timeInterval;
                // LOGI("%ld %ld",timer.m_timestamp,timestamp);
                m_events.insert(std::make_pair(timer.m_timestamp,timer));
            }
            else
                m_timers.erase(timer.m_timerId);
        }
    }
    modifyTimeout();
    m_poller->addEvent(m_ioEvent);
}