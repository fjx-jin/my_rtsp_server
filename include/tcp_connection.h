#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
#include "event.h"

typedef void (*DisConnectCallback)(void*, int);
class TcpConnection{
public:
    TcpConnection(int clientFd);
    virtual ~TcpConnection();

    void setDisConnectionCallback(DisConnectCallback cb,void* arg);

protected:
    virtual void handleRead();
    virtual void handleWrite();
    virtual void handleError();

private:
    void readCallback();
    void witeCallback();
    void errorCallback();

protected:
    int m_clientFd;
    IOEvent* m_ioEvent;
    DisConnectCallback m_disConnectCb;
    void* m_arg;
};

#endif