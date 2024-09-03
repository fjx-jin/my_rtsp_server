#include "sink.h"
#include "Log.h"
#include "epoller.h"

Sink::Sink(MediaSource* mediaSource,int payloadType) :
    m_mediaSource(mediaSource),
    m_csrcLen(0),
    m_extension(0),
    m_padding(0),
    m_version(RTP_VESION),
    m_payloadType(payloadType),
    m_marker(0),
    m_seq(0),
    m_timestamp(0),
    m_SSRC(rand()),
    m_timerId(0),
    m_sessionSendPacket(NULL),
    m_arg1(NULL),
    m_arg2(NULL),
    m_isStart(false),
    m_fps(mediaSource->getFps())
{
    LOGI("Sink()\n");
    m_packetData =  new RtpPacketData();
    m_timerEvent = new TimerEvent(this);
    m_timerEvent->setTimeoutCallback(timeoutCallback);
}

Sink::~Sink()
{
    //TODO
    stop();
    delete m_timerEvent;
    delete m_packetData;
}

void Sink::setSessionCb(SessionSendPacketCallback cb,void* arg1, void* arg2)
{
    m_sessionSendPacket = cb;
    m_arg1 = arg1;
    m_arg2 = arg2;
}

void Sink::run(int interval)
{
    m_timerId = EPoller::instance()->addTimerEventRunEvery(m_timerEvent,interval);
}

void Sink::sendRtpPacket(RtpPacketData* packet)
{
    RtpHeader* rtpHeader = &packet->m_rtpPacket->header;
    rtpHeader->cscLen = m_csrcLen;
    rtpHeader->extension = m_extension;
    rtpHeader->padding = m_padding;
    rtpHeader->version = m_version;
    rtpHeader->payloadType = m_payloadType;
    rtpHeader->marker = m_marker;
    rtpHeader->seq = htons(m_seq);
    rtpHeader->timestamp = htonl(m_timestamp);
    rtpHeader->ssrc = htonl(m_SSRC);

    if(m_sessionSendPacket){
        //arg1 mediaSession 对象指针
        //arg2 mediaSession 被回调track对象指针
        // 
        m_sessionSendPacket(m_arg1, m_arg2, packet, PacketType::RTPPACKET);
    }
}

void Sink::start()
{
    if(!m_isStart)
    {
        run(1000/(m_fps));
        m_isStart = true;
    }
}

void Sink::stop()
{
    if(m_isStart)
        EPoller::instance()->removeTimeEvent(m_timerId);
}

void Sink::timeoutCallback(void* arg)
{
    Sink* sink =(Sink*)arg;
    sink->handleTimeout();
}
void Sink::handleTimeout()
{
    // LOGI("handleTimeout\n");
    MediaFrame* frame = m_mediaSource->getFrameFromOutputQueue();
    if(!frame)
        return ;
    this->sendFrame(frame);
    // delete frame;
    // frame = new MediaFrame();
    m_mediaSource->putFrameIntoInputQueue(frame);
}