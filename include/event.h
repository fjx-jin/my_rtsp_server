#ifndef EVENT_H
#define EVENT_H
#include <sys/epoll.h>
typedef void (*EventCallback)(void*);
class IOEvent{
public:
    IOEvent(epoll_event ev,void* arg);
    ~IOEvent();

    epoll_event& getEvent() {return m_event;}
    // int getFd() const {return m_event.data.fd;}
    void setArg(void* arg){m_arg = arg;}
    void SetReadCallback(EventCallback cb) {m_readCb = cb;}
    void SetWriteCallback(EventCallback cb) {m_readCb = cb;}
    void SetErrorCallback(EventCallback cb) {m_readCb = cb;}

    void enableReadHandling() {m_event.events |= EPOLLIN;}
    void enableWriteHandling() {m_event.events |= EPOLLOUT;}
    void enableErrorHandling() {m_event.events |= EPOLLERR;}

    void handleEvent();
private:
    epoll_event m_event;
    void* m_arg;
    EventCallback m_readCb;
    EventCallback m_writeCb;
    EventCallback m_errorCb;
};

class TimerEvent{
public:
    TimerEvent(void* arg);
    ~TimerEvent();

    void setArg(void* arg) { m_arg = arg; }
    void setTimeoutCallback(EventCallback cb) { m_timeoutCallback = cb; }
    bool handleEvent();

    void stop();

private:
    void* m_arg;
    EventCallback m_timeoutCallback;
    bool m_isStop;
};
#endif