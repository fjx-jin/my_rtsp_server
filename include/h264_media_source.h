#ifndef H264_MEDIA_SOURCE_H
#define H264_MEDIA_SOURCE_H
#include "media_source.h"

class H264MediaSource :public MediaSource
{
public:
    H264MediaSource(const std::string& file);
    virtual ~H264MediaSource();

protected:
    void handleTask();
private:
    int getFrameFromH264File(uint8_t* buf,int maxSize);
};


#endif