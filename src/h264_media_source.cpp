#include "h264_media_source.h"
#include "Log.h"
#include "thread_pool.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static inline int startCode3(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

static inline int startCode4(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

static uint8_t* findNextStartCode(uint8_t* buf, int len)
{
    int i;

    if (len < 3)
        return NULL;

    for (i = 0; i < len - 3; ++i)
    {
        if (startCode3(buf) || startCode4(buf))
            return buf;

        ++buf;
    }

    if (startCode3(buf))
        return buf;

    return NULL;
}

H264MediaSource::H264MediaSource(const std::string& file) :
    MediaSource(file)
{
    setFps(25);
    for(int i = 0;i< DEFAULT_FRAME_NUM ;i++)
    {
        ThreadPool::instance()->addTask(inputCallback, this);
    }
}

H264MediaSource::~H264MediaSource()
{
    //TODO
}

void H264MediaSource::handleTask()
{
    std::lock_guard <std::mutex> lck(m_mtx);
    if(m_inputFrameQueue.empty())
        return ;
    MediaFrame* frame = m_inputFrameQueue.front();
    int startCode = 0;
    frame->mSize = getFrameFromH264File(frame->temp,FRAME_MAX_SIZE);
    if(frame->mSize < 0)
        return ;
    if(startCode3(frame->temp))
        startCode =3;
    else
        startCode = 4;
    frame->mBuf = frame->temp+startCode;
    frame->mSize -= startCode;
    frame->startCode = startCode;
    
    uint8_t naluType = frame->mBuf[0] & 0x1F;

    m_inputFrameQueue.pop();
    m_outputFrameQueue.push(frame);
}

int H264MediaSource::getFrameFromH264File(uint8_t* buf,int maxSize)
{
    if (m_fileFd < 0)
        return -1;
    
    int ret,frameSize;
    ret = read(m_fileFd,buf,maxSize);
    if(!startCode3(buf) && !startCode4(buf))
    {
        lseek(m_fileFd,0,SEEK_SET);
        return -1;
    }
    uint8_t* nextStartCode = findNextStartCode(buf+3,ret-3);
    if(!nextStartCode)
    {
        lseek(m_fileFd,0,SEEK_SET);
        frameSize = ret;
    }
    else
    {
        frameSize = nextStartCode - buf;
        lseek(m_fileFd,frameSize - ret,SEEK_CUR);
    }
    return frameSize;
}