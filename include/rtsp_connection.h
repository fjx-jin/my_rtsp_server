#ifndef RTSP_CONNECTION_H
#define RTSP_CONNECTION_H
#include "event.h"
#include "inet_addr.h"
#include "media_session.h"
#include <string>

typedef void (*DisConnectCallback)(void*, int);
class RtspConnection{
public:
    enum Method
    {
        OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN,
        NONE,
    };

    static RtspConnection* createNew(int clientFd);

    RtspConnection(int clientFd);
    virtual ~RtspConnection();

private:
    bool parseCSeq(char* buf);
    bool parseOptions(char* buf);
    bool parseDescribe(char* buf);
    bool parseSetup(char* buf);
    bool parsePlay(char* buf);

    bool handleCmdOption();
    bool handleCmdDescribe();
    bool handleCmdSetup();
    bool handleCmdPlay();
    bool handleCmdTeardown();

    static void readCallback(void* arg);
    void handleRead();
    static void writeCallback(void* arg);
    static void errorCallback(void* arg);

    void initBuf();
    void createRtpOverTcp(MediaSession::TrackId trackId,int sockfd,int channel);
    void createRtcpOverTcp(MediaSession::TrackId trackId,int sockfd,int channel);
    bool createRtpOverUdp(MediaSession::TrackId trackId,std::string destip,uint16_t destRtpPort,uint16_t destRtcpPort);
private:
    int m_clientFd;
    int m_sessionId;
    bool m_isOverTcp;
    uint8_t m_rtpchannel;
    uint8_t m_rtcpchannel;
    IOEvent* m_ioEvent;
    DisConnectCallback m_disConnectCallback; //需在rtspserver中处理断开请求
    void* m_arg; //DisConnectCallback中的入参
    Method m_method;//当前方法
    std::string m_suffix;
    std::string m_sessionName;
    Ipv4Address m_ipv4Addr;
    std::string m_destIp;
    uint16_t m_rtpPort; // 如果是over udp rtp会用到该端口
    uint16_t m_rtcpPort; //如果是over udp rtcp会用到该端口
    uint32_t m_cseq; //报文序列号
    char mBuffer[2048];
    RtpInstance* m_rtpInstances[MEDIA_MAX_TRACK_NUM];
    MediaSession::TrackId m_trackId;
    RtcpInstance* m_rtcpInstances[MEDIA_MAX_TRACK_NUM];
};

#endif