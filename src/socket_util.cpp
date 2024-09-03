#include "socket_util.h"
#include "Log.h"
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>

int sockets::createTcpSock()
{
    int sockfd = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    return sockfd;
}
int sockets::createUdpSock()
{
    int sockfd = socket(AF_INET,SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    return sockfd;
}
void sockets::close(int sockfd)
{
    ::close(sockfd);
}
bool sockets::bind(int sockfd, std::string ip, uint16_t port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if(::bind(sockfd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    {
        LOGE("::bind socket error fd=%d ip=%s port=%d\n",sockfd,ip.c_str(),port);
        return false;
    }
    return true;
}

bool sockets::listen(int sockfd, int backlog)
{
    if(::listen(sockfd,backlog) < 0)
    {
        LOGE("::listen error fd=%d backlog=%d\n",sockfd,backlog);
        return false;
    }
    return true;
}

int sockets::accept(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int conn_fd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
    if(conn_fd < 0)
    {
        LOGE("accept error fd=%d\n",conn_fd);
        goto out;
    }
    printf("accept a new client : %s\n",inet_ntoa(addr.sin_addr));
    setNonBlock(conn_fd);
    setCloseOnExec(conn_fd);
    setIgnoreSignalPipeOnSocket(conn_fd);
out:
    return conn_fd;
}

void sockets::setNonBlock(int sockfd)
{
    int flags = fcntl(sockfd,F_GETFL,0);
    flags |= O_NONBLOCK;
    fcntl(sockfd,F_SETFL,flags);
}

void sockets::setCloseOnExec(int sockfd)
{
    int flags = fcntl(sockfd,F_GETFL,0);
    flags |= FD_CLOEXEC;
    fcntl(sockfd,F_SETFL,flags);
}
void sockets::setIgnoreSignalPipeOnSocket(int sockfd)
{
    int var = 1;
    setsockopt(sockfd,SOL_SOCKET,MSG_NOSIGNAL,(const char*)&var,sizeof(var));
}

void sockets::setReuseAddr(int sockfd, int on)
{
    int var = on ? 1:0;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&var,sizeof(var));
}

int sockets::send(int sockfd, const void* buf, int size)
{
    // return ::send(sockfd,buf,size,0);
    return ::write(sockfd,buf,size);
}

int sockets::sendto(int sockfd, const void* buf, int len, const struct sockaddr *destAddr)
{
    socklen_t addrLen = sizeof(struct sockaddr);
    return ::sendto(sockfd, (char*)buf, len, 0, destAddr, addrLen);
}

std::string sockets::getFdIp(int fd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getpeername(fd,(struct sockaddr*)&addr,&addrlen);
    return inet_ntoa(addr.sin_addr);
}
