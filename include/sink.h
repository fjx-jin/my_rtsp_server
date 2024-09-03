#ifndef SINK_H
#define SINK_H
#include "media_source.h"
#include "event.h"
#include "timer.h"
#include "rtp.h"

class Sink{
public:
    enum PacketType
    {
        UNKNOWN = -1,
        RTPPACKET = 0,
    };
    typedef void (*SessionSendPacketCallback)(void* arg1, void* arg2, void* packet, PacketType packetType);
    Sink(MediaSource* mediaSource,int payloadType);
    virtual ~Sink();

    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    void setSessionCb(SessionSendPacketCallback cb,void* arg1, void* arg2);
    void start();
    void stop();
protected:
    void run(int interval); //ms
    virtual void sendFrame(MediaFrame* frame) = 0;
    void sendRtpPacket(RtpPacketData* packet);

private:
    static void timeoutCallback(void* arg);
    void handleTimeout();

protected:
    MediaSource* m_mediaSource;
    SessionSendPacketCallback m_sessionSendPacket;
    void* m_arg1;
    void* m_arg2;
    RtpPacketData* m_packetData;
    int m_fps;

    uint8_t m_csrcLen;
    uint8_t m_extension;
    uint8_t m_padding;
    uint8_t m_version;
    uint8_t m_payloadType;
    uint8_t m_marker;
    uint16_t m_seq;
    uint32_t m_timestamp;
    uint32_t m_SSRC;

    bool m_isStart;
private:
    TimerEvent* m_timerEvent;
    Timer::TimerId m_timerId;
};

#endif