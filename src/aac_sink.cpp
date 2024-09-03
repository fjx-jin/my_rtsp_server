#include "aac_sink.h"
#include "Log.h"
#include <string.h>

static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};

AACSink* AACSink::createNew(MediaSource* mediaSource)
{
    return new AACSink(mediaSource,RTP_PAYLOAD_TYPE_AAC);
}

AACSink::AACSink(MediaSource* mediaSource,int payloadType)  :
    Sink(mediaSource,payloadType),
    m_sampleRate(44100),
    m_channels(2)
{
    m_marker =1;
    // run(1000/1);
}

AACSink::~AACSink()
{

}

std::string AACSink::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, m_payloadType);

    return std::string(buf);
}

std::string AACSink::getAttribute()
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", m_sampleRate, m_channels);

    uint8_t index = 0;
    for (index = 0; index < 16; index++)
    {
        if (AACSampleRate[index] == m_sampleRate)
            break;
    }
    if (index == 16)
        return "";

    uint8_t profile = 1;
    char configStr[10] = {0};
    sprintf(configStr, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1),
            (uint8_t)((index << 7)|(m_channels<< 3)));

    sprintf(buf+strlen(buf),
            "a=fmtp:%d profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            m_payloadType,
            atoi(configStr));

    return std::string(buf);
}

void AACSink::sendFrame(MediaFrame* frame)
{
    int frameSize = frame->mSize-7; //去掉aac头部
    RtpPacket* rtpPacket = m_packetData->m_rtpPacket;
    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; //高8位
    rtpPacket->payload[3] = (frameSize & 0x1F) << 3; //低5位

    memcpy(rtpPacket->payload+4, frame->mBuf+7, frameSize);

    m_packetData->mSize = RTP_HEADER_SIZE + 4 + frameSize;

    m_seq++;
    sendRtpPacket(m_packetData);
    m_timestamp += m_sampleRate * (1000/ m_fps)/1000;
}