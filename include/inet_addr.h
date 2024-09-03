#ifndef INET_ADDR_H
#define INET_ADDR_H
#include <string>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Ipv4Address{
public:
    Ipv4Address() {}
    Ipv4Address(std::string ip,uint16_t port)
    {
        setAddr(ip,port);
    }
    void setAddr(std::string ip,uint16_t port)
    {
        m_ip = ip;
        m_port = port;
        m_addr.sin_family = AF_INET;		  
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str()); 
        m_addr.sin_port = htons(port);
    }
    std::string getId() const {return m_ip;}
    uint16_t getPort() const {return m_port;}
    struct sockaddr* getAddr() const {return (struct sockaddr*)&m_addr;};
    
private:
    std::string m_ip;
    uint16_t m_port;
    struct sockaddr_in m_addr;
};

#endif