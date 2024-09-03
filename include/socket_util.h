#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace sockets
{
    int createTcpSock();//默认创建非阻塞的tcp描述符
    int createUdpSock();
    void close(int sockfd);
    bool bind(int sockfd, std::string ip, uint16_t port);
    bool listen(int sockfd, int backlog);
    int accept(int sockfd);
    void setNonBlock(int sockfd);
    void setCloseOnExec(int sockfd);
    void setIgnoreSignalPipeOnSocket(int sockfd);
    void setReuseAddr(int sockfd, int on);
    int send(int sockfd, const void* buf, int size);// tcp 写入
    int sendto(int sockfd, const void* buf, int len, const struct sockaddr *destAddr); // udp 写入
    std::string getFdIp(int fd);
}

#endif