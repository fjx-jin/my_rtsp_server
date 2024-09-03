#ifndef MEDIA_SESSION_H
#define MEDIA_SESSION_H
#include <string>
#include <list>
#include "sink.h"
#include "rtp.h"
#define MEDIA_MAX_TRACK_NUM 2
class MediaSession
{
public:
    enum TrackId
    {
        TrackIdNone = -1,
        TrackId0    = 0,
        TrackId1    = 1,
    };

    MediaSession(const std::string& sessionName);
    ~MediaSession();

    std::string name() const {return m_sessionName;}
    std::string generateSDPDescription();
    bool addSink(MediaSession::TrackId trackId, Sink* sink);
    bool addRtpInstance(MediaSession::TrackId trackId, RtpInstance* rtpInstance,RtcpInstance* rtcpInstance);
    bool removeRtpInstance(MediaSession::TrackId trackId,RtpInstance* rtpInstance); //有链接断开时需要remove
    void startPlay();
    void stopPlay();

private:
    class Track{
    public:
        Sink* m_sink;
        int m_trackId;
        bool m_isAlive;
        std::list<RtpInstance*> m_rtpInstances;
        std::list<RtcpInstance*> m_rtcpInstances;
    };
    Track* getTrack(MediaSession::TrackId trackId);

    static void sendPacketCallback(void* arg1, void* arg2, void* packet,Sink::PacketType packetType);
    void handleSendRtpPacket(MediaSession::Track* track, RtpPacketData* rtpPacket);

    std::string m_sessionName;
    std::string m_sdpinfo;
    Track m_tracks[MEDIA_MAX_TRACK_NUM];
};

#endif