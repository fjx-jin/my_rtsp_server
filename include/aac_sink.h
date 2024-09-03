#ifndef AAC_SINK_SOURCE_H
#define AAC_SINK_SOURCE_H
#include "sink.h"

class AACSink : public Sink
{
public:
    static AACSink* createNew(MediaSource* mediaSource);
    virtual ~AACSink();

    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();
protected:
    virtual void sendFrame(MediaFrame* frame);
private:
    AACSink(MediaSource* mediaSource,int payloadType);

    uint32_t m_sampleRate;   // 采样频率
    uint32_t m_channels;         // 通道数
};

#endif