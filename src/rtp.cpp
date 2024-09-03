#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "rtp.h"
#include "socket_util.h"
#include "Log.h"
#include "timer.h"
static size_t alignSize(size_t bytes) {
    return (size_t)((bytes + 3) >> 2) << 2;
}
static void setupPadding(RtcpHeader* rtcp, size_t padding_size) {
    if (padding_size) {
        rtcp->padding = 1;
        ((uint8_t*)rtcp)[(1 + ntohs(rtcp->length)) << 2 - 1] = padding_size & 0xFF;
    }
    else {
        rtcp->padding = 0;
    }
}
void rtpHeaderInit(struct RtpPacket *rtpPacket, uint8_t csrcLen, uint8_t extension,
                   uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc)
{
    rtpPacket->header.cscLen = csrcLen;
    rtpPacket->header.extension = extension;
    rtpPacket->header.padding = padding;
    rtpPacket->header.version = version;
    rtpPacket->header.payloadType = payloadType;
    rtpPacket->header.marker = marker;
    rtpPacket->header.seq = seq;
    rtpPacket->header.timestamp = timestamp;
    rtpPacket->header.ssrc = ssrc;
}

int rtpSendPacketOverTcp(int clientSockfd, struct RtpPacket *rtpPacket, uint32_t dataSize, char channel)
{
    rtpPacket->header.seq = htons(rtpPacket->header.seq);
    rtpPacket->header.timestamp = htonl(rtpPacket->header.timestamp);
    rtpPacket->header.ssrc = htonl(rtpPacket->header.ssrc);

    uint32_t rtpSize = RTP_HEADER_SIZE + dataSize;
    char* tmpBuf = (char*)malloc(4+rtpSize);
    tmpBuf[0] = 0x24;//$
    tmpBuf[1] = channel;
    tmpBuf[2] = (uint8_t)(((rtpSize) & 0xFF00) >> 8);//高八位
    tmpBuf[3] = (uint8_t)((rtpSize) & 0xFF);//低八位
    memcpy(tmpBuf+4,(char*)rtpPacket,rtpSize);

    int ret = send(clientSockfd,tmpBuf,rtpSize + 4,0);
    free(tmpBuf);

    rtpPacket->header.seq = ntohs(rtpPacket->header.seq);
    rtpPacket->header.timestamp = ntohl(rtpPacket->header.timestamp);
    rtpPacket->header.ssrc = ntohl(rtpPacket->header.ssrc);
    return ret;
}

int rtpSendPacketOverUdp(int serverRtpSockfd, const char *ip, int16_t port, struct RtpPacket *rtpPacket, uint32_t dataSize)
{
    struct sockaddr_in addr;
    int ret;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    rtpPacket->header.seq = htons(rtpPacket->header.seq);
    rtpPacket->header.timestamp = htonl(rtpPacket->header.timestamp);
    rtpPacket->header.ssrc = htonl(rtpPacket->header.ssrc);

    ret = sendto(serverRtpSockfd, rtpPacket, dataSize + RTP_HEADER_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));

    rtpPacket->header.seq = ntohs(rtpPacket->header.seq);
    rtpPacket->header.timestamp = ntohl(rtpPacket->header.timestamp);
    rtpPacket->header.ssrc = ntohl(rtpPacket->header.ssrc);

    return ret;
}

RtpPacketData::RtpPacketData() :
    m_buf((uint8_t*)malloc(4+RTP_HEADER_SIZE+RTP_MAX_PKT_SIZE+100)),
    m_buf4((uint8_t*)m_buf+4),
    m_rtpPacket((RtpPacket*)m_buf4),
    mSize(0)
{

}

RtpPacketData::~RtpPacketData()
{
    free(m_buf);
    m_buf= NULL;
}

RtcpPacketData::RtcpPacketData() :
    m_buf((uint8_t*)malloc(4+RTCP_SR_EXCLUDE_ITEM)),
    m_buf4((uint8_t*)m_buf+4),
    m_rtcpSR((RtcpSR*)m_buf4),
    mSize(0)
{

}

RtcpPacketData::~RtcpPacketData()
{
    free(m_buf);
    m_buf= NULL;
}

RtpInstance* RtpInstance::createNewOverUdp(int localSockfd,uint16_t localport,
                                        std::string destIp,uint16_t destport)
{
    return new RtpInstance(localSockfd,localport,destIp,destport);
}

RtpInstance* RtpInstance::createNewOverTcp(int sockfd,uint8_t channel)
{
    return new RtpInstance(sockfd,channel);
}

RtpInstance::RtpInstance(int localSockfd, uint16_t localPort, const std::string& destIp, uint16_t destPort) :
    m_rtpType(RTP_OVER_UDP),
    m_socketFd(localSockfd),
    m_localport(localPort),
    m_destaddr(destIp,destPort),
    m_isAlive(false),
    m_count(0)
{

}

RtpInstance::RtpInstance(int sockfd, uint8_t rtpChannel) :
    m_rtpType(RTP_OVER_TCP),
    m_socketFd(sockfd),
    m_channel(rtpChannel),
    m_isAlive(false),
    m_count(0)
{

}

RtpInstance::~RtpInstance()
{
    //TODO
}

int RtpInstance::send(RtpPacketData* rtpPacket)
{
    switch (m_rtpType)
    {
    case RTP_OVER_UDP:
        m_count++;
        return sendOverUdp(rtpPacket->m_buf4,rtpPacket->mSize);
        break;
    case RTP_OVER_TCP:
        rtpPacket->m_buf[0] = '$';
        rtpPacket->m_buf[1] = (uint8_t)m_channel;
        rtpPacket->m_buf[2] = (uint8_t)(((rtpPacket->mSize) & 0xFF00) >> 8);
        rtpPacket->m_buf[3] = (uint8_t)((rtpPacket->mSize) & 0xFF);
        m_count++;
        return sendOverTcp(rtpPacket->m_buf,4+rtpPacket->mSize);
        break;
    default:
        return -1;
        break;
    }
}

int RtpInstance::sendOverUdp(void* buf,int size)
{
    // LOGI("sendOverUdp\n");
    return sockets::sendto(m_socketFd,buf,size,m_destaddr.getAddr());
}

int RtpInstance::sendOverTcp(void* buf,int size)
{
    // LOGI("sendOverTcp\n");
    return sockets::send(m_socketFd,buf,size);
}

RtcpInstance* RtcpInstance::createNewOverUdp(int localSockfd,uint16_t localport,
                                        std::string destIp,uint16_t destport)
{
    return new RtcpInstance(localSockfd,localport,destIp,destport);
}

RtcpInstance* RtcpInstance::createNewOverTcp(int sockfd,uint8_t channel)
{
    return new RtcpInstance(sockfd,channel);
}

RtcpInstance::RtcpInstance(int localSockfd, uint16_t localPort, const std::string& destIp, uint16_t destPort) :
    m_rtcpType(RtpInstance::RtpType::RTP_OVER_UDP),
    m_socketFd(localSockfd),
    m_localport(localPort),
    m_destaddr(destIp,destPort),
    m_isAlive(false),
    m_packets(0),
    m_bytes(0),
    m_last_rtp_stamp(0),
    m_last_ntp_stamp_ms(0)
{
    m_rtcpPacketData = new RtcpPacketData();
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.version = 2;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.padding = 0;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.rc = 0;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.packetType = RtcpType::RTCP_SR;
    // m_rtcpPacketData->m_rtcpSR->rtcpHeader.length = htons(RTCP_SR_EXCLUDE_ITEM);
    m_rtcpPacketData->mSize = RTCP_SR_EXCLUDE_ITEM;

    size_t item_count = 0;//SR包扩展0个ReportItem
    auto real_size = sizeof(RtcpSR) - sizeof(ReportItem) + item_count * sizeof(ReportItem);
    auto bytes = alignSize(real_size);

    m_rtcpPacketData->m_rtcpSR->rtcpHeader.length = htons((uint16_t)((bytes >> 2) - 1));
    setupPadding(&m_rtcpPacketData->m_rtcpSR->rtcpHeader, bytes - real_size);
}

RtcpInstance::RtcpInstance(int sockfd, uint8_t rtpChannel) :
    m_rtcpType(RtpInstance::RtpType::RTP_OVER_TCP),
    m_socketFd(sockfd),
    m_channel(rtpChannel),
    m_isAlive(false),
    m_packets(0),
    m_bytes(0),
    m_last_rtp_stamp(0),
    m_last_ntp_stamp_ms(0)
{
    m_rtcpPacketData = new RtcpPacketData();
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.version = 2;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.padding = 0;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.rc = 0;
    m_rtcpPacketData->m_rtcpSR->rtcpHeader.packetType = RtcpType::RTCP_SR;
    // m_rtcpPacketData->m_rtcpSR->rtcpHeader.length = htons(RTCP_SR_EXCLUDE_ITEM);
    m_rtcpPacketData->mSize = RTCP_SR_EXCLUDE_ITEM;

    size_t item_count = 0;//SR包扩展0个ReportItem
    auto real_size = sizeof(RtcpSR) - sizeof(ReportItem) + item_count * sizeof(ReportItem);
    auto bytes = alignSize(real_size);

    m_rtcpPacketData->m_rtcpSR->rtcpHeader.length = htons((uint16_t)((bytes >> 2) - 1));
    setupPadding(&m_rtcpPacketData->m_rtcpSR->rtcpHeader, bytes - real_size);
}

RtcpInstance::~RtcpInstance()
{
    //TODO
}

int RtcpInstance::send()
{

    uint32_t rtcpSize;
    switch (m_rtcpType)
    {
    case RtpInstance::RtpType::RTP_OVER_UDP:
        rtcpSize = (1 + ntohs(m_rtcpPacketData->m_rtcpSR->rtcpHeader.length)) << 2;
        return sendOverUdp(m_rtcpPacketData->m_buf4,rtcpSize);
        break;
    case RtpInstance::RtpType::RTP_OVER_TCP:
        m_rtcpPacketData->m_buf[0] = '$';
        m_rtcpPacketData->m_buf[1] = (uint8_t)m_channel;
        m_rtcpPacketData->m_buf[2] = (uint8_t)(((m_rtcpPacketData->mSize) & 0xFF00) >> 8);
        m_rtcpPacketData->m_buf[3] = (uint8_t)((m_rtcpPacketData->mSize) & 0xFF);
        rtcpSize = (1 + ntohs(m_rtcpPacketData->m_rtcpSR->rtcpHeader.length)) << 2;
        return sendOverTcp(m_rtcpPacketData->m_buf,4+rtcpSize);
        break;
    default:
        return -1;
        break;
    }
}

void RtcpInstance::onRtp(RtpPacketData* rtpPacket)
{
    m_packets++;
    m_bytes += rtpPacket->mSize;
    m_rtcpPacketData->m_rtcpSR->packet_count = htonl((uint32_t)m_packets);
    m_rtcpPacketData->m_rtcpSR->octet_count = htonl((uint32_t)m_bytes);
    m_rtcpPacketData->m_rtcpSR->rtpts = rtpPacket->m_rtpPacket->header.timestamp;
    m_last_ntp_stamp_ms = Timer::getCurTimestamp();
    struct timeval tv;
    tv.tv_sec = m_last_ntp_stamp_ms / 1000;
    tv.tv_usec = (m_last_ntp_stamp_ms % 1000) * 1000;
    m_rtcpPacketData->m_rtcpSR->ntpmsw = htonl(tv.tv_sec + 0x83AA7E80); /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
    m_rtcpPacketData->m_rtcpSR->ntplsw = htonl((uint32_t)((double)tv.tv_usec * (double)(((uint64_t)1) << 32) * 1.0e-6));
    m_rtcpPacketData->m_rtcpSR->ssrc = rtpPacket->m_rtpPacket->header.ssrc;
}

int RtcpInstance::sendOverUdp(void* buf,int size)
{
    // LOGI("sendOverUdp\n");
    return sockets::sendto(m_socketFd,buf,size,m_destaddr.getAddr());
}

int RtcpInstance::sendOverTcp(void* buf,int size)
{
    // LOGI("sendOverTcp\n");
    return sockets::send(m_socketFd,buf,size);
}
