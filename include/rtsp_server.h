#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H
#include "inet_addr.h"
#include "event.h"
#include <map>

class RtspConnection;
class RtspServer{
public:
    // static RtspServer* instance() {return m_instance;}
    RtspServer(Ipv4Address& addr);
    ~RtspServer();

    void start();
private:
    static void readCallback(void* arg);
    void handleRead();
    // void writeCallback(void* arg);
    // void errorCallback(void* arg);
    void disconnectCallback();

private:
    static RtspServer* m_instance;

    int m_listenFd;//监听rtsp连接请求
    Ipv4Address m_ipv4Addr;
    IOEvent* m_acceptEvent;//处理接收请求
    std::map<int ,RtspConnection*> m_connectionMap;//维护所有rtsp连接
};
// RtspServer* RtspServer::m_instance = new RtspServer();
#endif
