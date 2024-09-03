#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include <stdint.h>
#include <queue>
#include <mutex>
#include <string>

#define FRAME_MAX_SIZE (1024*200)
#define DEFAULT_FRAME_NUM 10
class MediaFrame
{

public:
    MediaFrame() :
        temp(new uint8_t[FRAME_MAX_SIZE]),
        mBuf(nullptr),
        mSize(0){
        
    }
    ~MediaFrame(){
        delete []temp;
    }
    
    uint8_t* temp;// 容器
    uint8_t* mBuf;// 引用容器
    int startCode;
    int mSize;
};

class MediaSource{
public:
    MediaSource(const std::string& file);
    virtual ~MediaSource();

    void putFrameIntoInputQueue(MediaFrame *frame);
    MediaFrame* getFrameFromOutputQueue();
    int getFps() const {return m_fps;}
    void setFps(int fps) {m_fps = fps;}
protected:
    virtual void handleTask() = 0;
    static void inputCallback(void* arg);

    int m_fileFd;
    std::mutex m_mtx;
    std::queue<MediaFrame*> m_inputFrameQueue; //运行时会逐次取出一个frame 将媒体数据写进去 然后放到outputqueue
    std::queue<MediaFrame*> m_outputFrameQueue; //储存处理好的frame 后续将其放到rtppacket的载荷中发出
private:
    MediaFrame m_frames[DEFAULT_FRAME_NUM];
    std::string m_sourceName;
    int m_fps;
    
};
#endif