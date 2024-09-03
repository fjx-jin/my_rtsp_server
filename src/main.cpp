#include "rtsp_server.h"
#include "epoller.h"
#include "media_session_manager.h"
#include "media_session.h"
#include "aac_sink.h"
#include "h264_sink.h"
#include "aac_media_source.h"
#include "h264_media_source.h"

int main(int argc,char* argv[])
{
    srand(time(NULL));//时间初始化
    Ipv4Address rtspAddr("127.0.0.1", 8554);
    RtspServer* rtspServer = new RtspServer(rtspAddr);
    {
        MediaSession* session = new MediaSession("test");

        MediaSource* h264Source =  new H264MediaSource("../test.h264");
        Sink* h264Sink =  H264Sink::createNew(h264Source);
        session->addSink(MediaSession::TrackId0,h264Sink);

        MediaSource* aacSource =  new AACMeidaSource("../test.aac");
        Sink* aacSink =  AACSink::createNew(aacSource);
        session->addSink(MediaSession::TrackId1,aacSink);

        MediaSessionManager::instance()->addSession(session);
    }
    rtspServer->start();
    EPoller::instance()->loop();
    return 0;
}