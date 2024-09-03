#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <map>

class TimerEvent;
class EPoller;
class IOEvent;

class Timer
{
public:
    
    typedef uint32_t TimerId;
    typedef int64_t Timestamp;//ms
    typedef uint32_t TimeInterval;//ms

    static Timestamp getCurTime(); // 当前系统启动以来的毫秒数
    static Timestamp getCurTimestamp(); // 获取毫秒级时间戳 13位

    Timer(TimerEvent* event,Timestamp timestamp,TimeInterval timeInterval,TimerId timerId);
    ~Timer();
private:
    friend class TimerManager;
    bool handleEvent();
private:
    TimerEvent* m_timerEvent;
    Timestamp m_timestamp;
    TimeInterval m_timeInterval;
    TimerId m_timerId;

    bool m_repeat;
};

class TimerManager
{
public:
    TimerManager(EPoller* poller);
    ~TimerManager();

    Timer::TimerId addTimer(TimerEvent* event,Timer::Timestamp timestamp,Timer::TimeInterval interval);
    bool removeTimer(Timer::TimerId timerId);

private:
    static void readCallback(void* arg);
    void handleRead();
    void modifyTimeout();

private:
    int m_timerfd;
    IOEvent* m_ioEvent;
    EPoller* m_poller;
    std::map<Timer::TimerId, Timer> m_timers;
    std::multimap<Timer::Timestamp, Timer> m_events;
    uint32_t m_lastTimerId;
};

#endif