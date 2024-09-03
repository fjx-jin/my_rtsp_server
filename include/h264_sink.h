#ifndef H264_SINK_H
#define H264_SINK_H
#include "sink.h"

class H264Sink : public Sink
{
public:
    static H264Sink* createNew(MediaSource* mediaSource);
    virtual ~H264Sink();

    virtual std::string getMediaDescription(uint16_t port);
    virtual std::string getAttribute();
protected:
    virtual void sendFrame(MediaFrame* frame);
private:
    H264Sink(MediaSource* mediaSource,int payloadType);

    int m_clockRate;
};

#endif