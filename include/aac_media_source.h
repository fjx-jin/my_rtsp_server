#ifndef AAC_MEDIA_SOURCE_H
#define AAC_MEDIA_SOURCE_H
#include <string>
#include "media_source.h"

class AACMeidaSource : public MediaSource
{
public:
    AACMeidaSource(const std::string& file);
    virtual ~AACMeidaSource();
protected:
    void handleTask();
private:
    struct AdtsHeader
    {
        unsigned int syncword;         // 12bit 同步字 '1111 1111 1111'表示一个adts帧的开始
        uint8_t id;                // 1bit 0表示MPEG-4 1表示MPEG-2
        uint8_t layer;             // 2bit 必须为0
        uint8_t protectionAbsent;  // 1bit 1表示没有CRC 0表示有CRC
        uint8_t profile;           // 2bit aac级别
        uint8_t samplingFreqIndex; // 4bit 采样频率
        uint8_t privateBit;        // 1bit编码时设置为0，解码时忽略
        uint8_t channelCfg;        // 3bit 声道数量
        uint8_t originalCopy;      // 1bit 编码时设置为0，解码时忽略
        uint8_t home;              // 1 bit 编码时设置为0，解码时忽略

        uint8_t copyrightIdentificationBit;   // 1bit
        uint8_t copyrightIdentificationStart; // 1bit
        unsigned int aacFrameLength;              // 13bit 表示一个adts帧的长度 包括adts头和aac原始数据流
        unsigned int adtsBufferFullness;          // 11bit 缓冲区充满度 0x7FF表示码率时可变的码率，不需要此字段。
                                            // CBR可能需要此字段，不同编码器使用情况不同。这个在使用音频编码的时候需要注意。

        /* number_of_raw_data_blocks_in_frame
        * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
        * 所以说number_of_raw_data_blocks_in_frame == 0
        * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
        */
        uint8_t numberOfRawDataBlockInFrame; // 2 bit
    };

    bool parseAdtsHeader(uint8_t* in, struct AdtsHeader* res);
    int getFrameFromAACFile(uint8_t* buf, int maxSize);

private:
    struct AdtsHeader m_adtsHeader;
};

#endif