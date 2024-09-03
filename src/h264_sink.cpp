#include "h264_sink.h"
#include "Log.h"
#include <string.h>

H264Sink* H264Sink::createNew(MediaSource* mediaSource)
{
    return new H264Sink(mediaSource,RTP_PAYLOAD_TYPE_H264);
}

H264Sink::H264Sink(MediaSource* mediaSource,int payloadType) :
    Sink(mediaSource,payloadType),
    m_clockRate(90000)
{

}

H264Sink::~H264Sink()
{

}

std::string H264Sink::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=video %hu RTP/AVP %d", port, m_payloadType);

    return std::string(buf);
}

std::string H264Sink::getAttribute()
{
    char buf[100];
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", m_payloadType, m_clockRate);
    sprintf(buf + strlen(buf), "a=framerate:%d", m_fps);

    return std::string(buf);
}

void H264Sink::sendFrame(MediaFrame* frame)
{
    RtpPacket* rtpPacket = m_packetData->m_rtpPacket;
    uint8_t naluType = frame->mBuf[0];
    
    if(frame->mSize <= RTP_MAX_PKT_SIZE)
    {
        memcpy(rtpPacket->payload,frame->mBuf,frame->mSize);
        m_packetData->mSize = RTP_HEADER_SIZE + frame->mSize;
        sendRtpPacket(m_packetData);
        m_seq++;

        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8 || (naluType & 0x1F) == 6) // 如果是SPS、PPS就不需要加时间戳
            return;
        // if(frame->startCode == 3)
        //     return ;
    }
    else
    {
        int pktNum = frame->mSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frame->mSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            /*
            *     FU Indicator
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |F|NRI|  Type   |
            *   +---------------+
            * */
            rtpPacket->payload[0] = (naluType & 0x60) | 28; //(naluType & 0x60)表示nalu的重要性，28表示为分片

            /*
            *      FU Header
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |S|E|R|  Type   |
            *   +---------------+
            * */
            rtpPacket->payload[1] = naluType & 0x1F;

            rtpPacket->header.marker = 0;
            if (i == 0) //第一包数据
                rtpPacket->payload[1] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
            {
                rtpPacket->payload[1] |= 0x40; // end
                rtpPacket->header.marker = 1;
            }
                

            memcpy(rtpPacket->payload + 2, frame->mBuf + pos, RTP_MAX_PKT_SIZE);
            m_packetData->mSize = RTP_HEADER_SIZE + 2 + RTP_MAX_PKT_SIZE;
            sendRtpPacket(m_packetData);

            m_seq++;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; //end
            rtpPacket->header.marker = 1;

            memcpy(rtpPacket->payload + 2, frame->mBuf + pos, remainPktSize);
            m_packetData->mSize = RTP_HEADER_SIZE + 2 + remainPktSize;
            sendRtpPacket(m_packetData);

            m_seq++;
        }

        // if(frame->startCode == 3)
        //     return ;
    }

    m_timestamp += m_clockRate/m_fps;
}