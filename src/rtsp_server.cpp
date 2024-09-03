#include "rtsp_server.h"
#include "epoller.h"
#include "socket_util.h"
#include "Log.h"
#include "rtsp_connection.h"

RtspServer::RtspServer(Ipv4Address& addr):
    m_ipv4Addr(addr)
{
    m_listenFd = sockets::createTcpSock();
    sockets::setReuseAddr(m_listenFd,1);
    if(!sockets::bind(m_listenFd,addr.getId(),addr.getPort()))
        return ;
    
    epoll_event ev;
    ev.data.fd = m_listenFd;
    m_acceptEvent = new IOEvent(ev,this);
    m_acceptEvent->enableReadHandling();
    m_acceptEvent->SetReadCallback(readCallback);
}

RtspServer::~RtspServer()
{
    EPoller::instance()->removeEvent(m_acceptEvent);
    delete m_acceptEvent;
}

void RtspServer::start()
{
    if(!sockets::listen(m_listenFd,60))
        return ;
    EPoller::instance()->addEvent(m_acceptEvent);
    LOGI("rtsp://%s:%d",m_ipv4Addr.getId().c_str(),m_ipv4Addr.getPort());
}

void RtspServer::readCallback(void* arg)
{
    RtspServer* server = (RtspServer*)arg;
    server->handleRead();
}

void RtspServer::handleRead()
{
    LOGI("1\n");
    int connfd = sockets::accept(m_listenFd);
    if(connfd < 0)
        return;
    RtspConnection* connection = RtspConnection::createNew(connfd);
    m_connectionMap.insert(std::make_pair(connfd,connection));
}