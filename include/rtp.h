#ifndef _RTP_H_
#define _RTP_H_
#include <stdint.h>
#include <string>
#include "inet_addr.h"

#define RTP_VESION 2

#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC 97

#define RTP_HEADER_SIZE 12
#define RTP_MAX_PKT_SIZE 1400

#define RTCP_SR_EXCLUDE_ITEM 28
/*
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

struct RtpHeader
{
    uint8_t cscLen : 4;    // csrc计数器 指示csrc标识符的个数
    uint8_t extension : 1; // 如果=1 则在rtp报头后跟有一个扩展报头
    uint8_t padding : 1;   // 填充标志，如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分
    uint8_t version : 2;   // rtp协议的版本号，当前协议版本号为2

    uint8_t payloadType : 7; // 有效载荷类型，用于说明有效载荷的类型，如GSM音频、JPEM图像等
    uint8_t marker : 1;      // 标记，不同的有效载荷有不同的含义，对于视频，标识一帧的结束，对于音频，标记会话的开始

    uint16_t seq; // rtp报文的序列号，每发送一个报文，序列号加1

    uint32_t timestamp; // 时间戳，反应rtp报文第一个八位组的采样时刻，接收者使用时间戳来计算延迟和延迟抖动，并进行同步控制

    uint32_t ssrc; // 占32位，用于标识同步信源。该标识符是随机选择的，参加同一视频会议的两个同步信源不能有相同的SSRC。

    /*标准的RTP Header 还可能存在 0-15个特约信源(CSRC)标识符

   每个CSRC标识符占32位，可以有0～15个。每个CSRC标识了包含在该RTP报文有效载荷中的所有特约信源

   */
};

struct RtpPacket
{
    struct RtpHeader header;
    uint8_t payload[0];
};

class RtpPacketData
{
    public:
        RtpPacketData();
        ~RtpPacketData();
    public:
        uint8_t* m_buf; // 4+rtpHeader+rtpBody
        uint8_t* m_buf4;// rtpHeader+rtpBody
        RtpPacket* m_rtpPacket;
        int mSize;// rtpHeader+rtpBody
};
void rtpHeaderInit(struct RtpPacket *rtpPacket, uint8_t csrcLen, uint8_t extension,
                   uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc);

int rtpSendPacketOverTcp(int clientSockfd, struct RtpPacket *rtpPacket, uint32_t dataSize, char channel);

// return 0 success -1 fail
int rtpSendPacketOverUdp(int serverRtpSockfd, const char *ip, int16_t port, struct RtpPacket *rtpPacket, uint32_t dataSize);

struct RtcpHeader
{
    // byte 0
    uint8_t rc : 5;// reception report count
    uint8_t padding : 1;
    uint8_t version : 2;
    // byte 1
    uint8_t packetType;

    // bytes 2,3
    uint16_t length;
};

struct ReportItem
{
    uint32_t ssrc;
    // Fraction lost
    uint32_t fraction : 8;// 丢包率，从收到上一个SR或RR包以来的RTP数据包的丢失率

    // Cumulative number of packets lost
    uint32_t cumulative : 24; // 累计丢失的数据包数

    // Sequence number cycles count
    uint16_t seq_cycles; // 序列号循环计数

    // Highest sequence number received
    uint16_t seq_max; // 序列最大值

    // Interarrival jitter
    uint32_t jitter;//接收抖动，RTP数据包接受时间的统计方差估计

    // Last SR timestamp, NTP timestamp,(ntpmsw & 0xFFFF) << 16  | (ntplsw >> 16) & 0xFFFF)
    uint32_t last_sr_stamp;//上次SR时间戳，取最近收到的SR包中的NTP时间戳的中间32比特。如果目前还没收到SR包，则该域清零

    // Delay since last SR timestamp,expressed in units of 1/65536 seconds
    uint32_t delay_since_last_sr;//上次SR以来的延时，上次收到SR包到发送本报告的延时
};

struct RtcpSR
{
    struct RtcpHeader rtcpHeader;
    uint32_t ssrc; //和rtp的ssrc相等
    // ntp timestamp MSW(in second)  秒
    uint32_t ntpmsw;
    // ntp timestamp LSW(in picosecond)  微微秒
    uint32_t ntplsw;
    // rtp timestamp 与RTP的timestamp对应
    uint32_t rtpts;
    // sender packet count 发送RTP数据包的数量
    uint32_t packet_count;
    // sender octet count  发送RTP数据包的字节数
    uint32_t octet_count;

    struct ReportItem items;
};

class RtcpPacketData
{
    public:
        RtcpPacketData();
        ~RtcpPacketData();
    public:
        uint8_t* m_buf; // 4+RtcpSR
        uint8_t* m_buf4;// RtcpSR
        RtcpSR* m_rtcpSR;
        int mSize;// RtcpSR
};

class RtpInstance{
public:
    enum RtpType
    {
        RTP_OVER_UDP,
        RTP_OVER_TCP
    };

    static RtpInstance* createNewOverUdp(int localSockfd,uint16_t localport,
                                        std::string destIp,uint16_t destport);
    static RtpInstance* createNewOverTcp(int sockfd,uint8_t channel);

    ~RtpInstance();
    int send(RtpPacketData* rtpPacket);
    bool isAlive() const {return m_isAlive;}
    void setisAlive(bool isalive) {m_isAlive = isalive;}
    uint16_t getLocalPort() const {return m_localport;}
    void setCount(int var) {m_count = var;}
    int getCount() const {return m_count;}
private:
    RtpInstance(int localSockfd, uint16_t localPort, const std::string& destIp, uint16_t destPort);
    RtpInstance(int sockfd, uint8_t rtpChannel);
    int sendOverUdp(void* buf,int size);
    int sendOverTcp(void* buf,int size);

private:
    RtpType m_rtpType;
    int m_socketFd;
    uint8_t m_channel; // over tcp 会用到
    uint16_t m_localport; //for udp
    Ipv4Address m_destaddr; // for udp
    bool m_isAlive;
    int m_count;
};

class RtcpInstance
{
public:
    enum RtcpType
    {
        RTCP_SR = 200
    };
    static RtcpInstance* createNewOverUdp(int localSockfd,uint16_t localport,
                                        std::string destIp,uint16_t destport);
    static RtcpInstance* createNewOverTcp(int sockfd,uint8_t channel);

    ~RtcpInstance();
    int send();
    void onRtp(RtpPacketData* rtpPacket);
    bool isAlive() const {return m_isAlive;}
    void setisAlive(bool isalive) {m_isAlive = isalive;}
    uint16_t getLocalPort() const {return m_localport;}
private:
    RtcpInstance(int localSockfd, uint16_t localPort, const std::string& destIp, uint16_t destPort);
    RtcpInstance(int sockfd, uint8_t rtpChannel);
    int sendOverUdp(void* buf,int size);
    int sendOverTcp(void* buf,int size);

private:
    RtpInstance::RtpType m_rtcpType;
    int m_socketFd;
    uint8_t m_channel; // over tcp 会用到
    uint16_t m_localport; //for udp
    Ipv4Address m_destaddr; // for udp
    bool m_isAlive;
    RtcpPacketData* m_rtcpPacketData;
    size_t m_packets;
    size_t m_bytes;
    uint32_t m_last_rtp_stamp;
    uint64_t m_last_ntp_stamp_ms;
};
#endif