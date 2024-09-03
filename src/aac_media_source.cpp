#include "aac_media_source.h"
#include "Log.h"
#include "thread_pool.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

AACMeidaSource::AACMeidaSource(const std::string& file) :
    MediaSource(file)
{
    setFps(43);

    //开四个任务把inputqueue里面的frame全弄好
    for(int i = 0;i< DEFAULT_FRAME_NUM ;i++)
    {
        ThreadPool::instance()->addTask(inputCallback, this);
    }
}

AACMeidaSource::~AACMeidaSource()
{
    //TODO
}

void AACMeidaSource::handleTask()
{
    // LOGI("handleTask\n");
    std::lock_guard <std::mutex> lck(m_mtx);
    if(m_inputFrameQueue.empty())
        return ;
    
    MediaFrame* frame = m_inputFrameQueue.front();
    frame->mSize = getFrameFromAACFile(frame->temp,FRAME_MAX_SIZE);
    if(frame->mSize < 0)
        return;

    frame->mBuf = frame->temp;
    m_inputFrameQueue.pop();
    m_outputFrameQueue.push(frame);
    // LOGI("handleTask end\n");
}

bool AACMeidaSource::parseAdtsHeader(uint8_t* in, struct AdtsHeader* res)
{
    // LOGI("parseAdtsHeader\n");
    memset(res, 0, sizeof(*res));

    if (in[0] == 0xFF && ((in[1] & 0xF0) == 0xF0))
    {
        res->id = ((uint8_t)in[1] & 0x08) >> 3;                                   // 0000 1000 >> 3
        res->layer = ((uint8_t)in[1] & 0x06) >> 1;                                // 0000 0110 >> 1
        res->protectionAbsent = ((uint8_t)in[1] & 0x01);                          // 0000 0001 >> 0
        res->profile = ((uint8_t)in[2] & 0xC0) >> 6;                              // 1100 0000 >> 6
        res->samplingFreqIndex = ((uint8_t)in[2] & 0x3C) >> 2;                    // 0011 1100 >> 2
        res->privateBit = ((uint8_t)in[2] & 0x02) >> 1;                           // 0000 0010 >> 1
        res->channelCfg = (((uint8_t)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xC0) >> 6); // 第一个字节0000 0001 << 2;第二个字节1100 0000 >> 6
        res->originalCopy = ((uint8_t)in[3] & 0x20) >> 5;                         // 0010 0000 >> 5
        res->home = ((uint8_t)in[3] & 0x10) >> 4;                                 // 0001 0000 >> 4
        res->copyrightIdentificationBit = ((uint8_t)in[3] & 0x08) >> 3;           // 0000 1000 >> 3
        res->copyrightIdentificationStart = ((uint8_t)in[3] & 0x04) >> 2;         // 0000 0100 >> 2
        res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
            (((unsigned int)in[4] & 0xFF) << 3) |
            ((unsigned int)in[5] & 0xE0) >> 5);
        // res->aacFrameLength = (((unsigned int)in[3] && 0x03) << 11)          // 前两个字节 0000 0000 0000 0011 << 11
        //                       | (((unsigned int)in[4] && 0xFF) << 3)         // 中间8个字节 0000 0000 1111 1111 << 3
        //                       | (((unsigned int)in[5] && 0xE0) >> 5);        // 最后三个字节 0000 0000 1110 0000 >> 5
        res->adtsBufferFullness = (((unsigned int)in[5] && 0x1F) << 6)       // 0000 0000 0001 1111 << 6
                                  | (((unsigned int)in[6] && 0xFC) >> 2);    // 0000 0000 1111 1100 >> 2
        res->numberOfRawDataBlockInFrame = ((uint8_t)in[6] & 0x03);               // 0000 0011
        return true;
    }
    else
    {
        LOGI("failed to parse adts header\n");
        return false;
    }
}

int AACMeidaSource::getFrameFromAACFile(uint8_t* buf, int maxSize)
{
    // LOGI("getFrameFromAACFile\n");
    if (m_fileFd < 0)
        return -1;
    uint8_t tmpBuf[7];
    int ret;
    ret = read(m_fileFd,tmpBuf,7);
    if(ret <= 0)
    {
        lseek(m_fileFd,0,SEEK_SET);
        ret =  read(m_fileFd,tmpBuf,7);
        if(ret <= 0)
            return -1;
    }
    if(!parseAdtsHeader(tmpBuf,&m_adtsHeader))
        return -1;

    if(m_adtsHeader.aacFrameLength > maxSize)
        return -1;
    memcpy(buf,tmpBuf,7);
    ret = read(m_fileFd,buf+7,m_adtsHeader.aacFrameLength-7);
    if(ret < 0)
    {
        LOGE("getFrameFromAACFile read error\n");
        return -1;
    }
    return m_adtsHeader.aacFrameLength;
}