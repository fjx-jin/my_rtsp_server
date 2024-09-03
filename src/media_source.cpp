#include "media_source.h"
#include "Log.h"
#include "thread_pool.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

MediaSource::MediaSource(const std::string& file) :
    m_sourceName(file)
{
    m_fileFd = open(file.c_str(),O_RDONLY);
    for(int i = 0;i< DEFAULT_FRAME_NUM ;i++)
    {
        m_inputFrameQueue.push(&m_frames[i]);
    }
}

MediaSource::~MediaSource()
{
    //TODO
}

void MediaSource::putFrameIntoInputQueue(MediaFrame *frame)
{
    std::lock_guard <std::mutex> lck(m_mtx);

    m_inputFrameQueue.push(frame);

    //TODO 开一个子线程从文件中读取frame
    ThreadPool::instance()->addTask(inputCallback, this);
}

MediaFrame* MediaSource::getFrameFromOutputQueue()
{
    std::lock_guard <std::mutex> lck(m_mtx);
    if(m_outputFrameQueue.empty()){
        return NULL;
    }
    MediaFrame* frame = m_outputFrameQueue.front();
    m_outputFrameQueue.pop();

    return frame;
}

void MediaSource::inputCallback(void* arg)
{
    MediaSource* source = (MediaSource*) arg;
    // LOGI("%p\n",source);
    source->handleTask();
}
