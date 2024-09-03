#include "media_session.h"
#include "Log.h"
#include <string.h>

MediaSession::MediaSession(const std::string& sessionName) :
    m_sessionName(sessionName)
{
    LOGI("MediaSession() name=%s",sessionName.data());
    m_tracks[0].m_trackId = TrackId0;
    m_tracks[0].m_isAlive = false;

    m_tracks[1].m_trackId = TrackId1;
    m_tracks[1].m_isAlive = false;
}
MediaSession::~MediaSession()
{
    //TODO
}

std::string MediaSession::generateSDPDescription()
{
    if(!m_sdpinfo.empty())
        return m_sdpinfo;

    std::string ip = "0.0.0.0";
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
        "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "a=type:broadcast\r\n",
        (long)time(NULL), ip.c_str());
    
    for(int i =0;i< MEDIA_MAX_TRACK_NUM;i++)
    {
        uint16_t port = 0;

        if(m_tracks[i].m_isAlive != true)
            continue;

        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
        "%s\r\n", m_tracks[i].m_sink->getMediaDescription(port).c_str());
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
            "c=IN IP4 0.0.0.0\r\n");

        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
        "%s\r\n", m_tracks[i].m_sink->getAttribute().c_str());

        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),											
                "a=control:track%d\r\n", m_tracks[i].m_trackId);
    }
    m_sdpinfo = buf;
    return m_sdpinfo;
}

bool MediaSession::addSink(MediaSession::TrackId trackId, Sink* sink)
{
    Track* track = getTrack(trackId);
    if(!track)
        return false;
    
    track->m_sink =sink;
    track->m_isAlive = true;

    sink->setSessionCb(MediaSession::sendPacketCallback,this, track);
    return true;
}
bool MediaSession::addRtpInstance(MediaSession::TrackId trackId, RtpInstance* rtpInstance,RtcpInstance* rtcpInstance)
{
    Track* track = getTrack(trackId);
    if(!track || track->m_isAlive != true)
        return false;
    
    LOGI("addRtpInstance %x %x\n",rtpInstance,rtcpInstance);
    track->m_rtpInstances.push_back(rtpInstance);
    track->m_rtcpInstances.push_back(rtcpInstance);
    LOGI("ssssss %d %d",track->m_rtpInstances.size(),track->m_rtcpInstances.size());
    return true;
}
bool MediaSession::removeRtpInstance(MediaSession::TrackId trackId,RtpInstance* rtpInstance)
{
    Track* track = getTrack(trackId);
    if(!track || track->m_isAlive != true)
        return false;
    auto it = track->m_rtpInstances.begin();
    auto it1 = track->m_rtcpInstances.begin();
    while(it != track->m_rtpInstances.end())
    {
        if((*it) == rtpInstance)
        {
            track->m_rtpInstances.erase(it);
            track->m_rtcpInstances.erase(it1);
            break;
        }
        it++;
        it1++;
    }
    return false;
}

void MediaSession::startPlay()
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(m_tracks[i].m_isAlive)
            m_tracks[i].m_sink->start();
    }
}
void MediaSession::stopPlay()
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(m_tracks[i].m_isAlive)
            m_tracks[i].m_sink->stop();
    }
}

MediaSession::Track* MediaSession::getTrack(MediaSession::TrackId trackId)
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(m_tracks[i].m_trackId == trackId)
            return &m_tracks[i];
    }

    return NULL;
}

void MediaSession::sendPacketCallback(void* arg1, void* arg2, void* packet,Sink::PacketType packetType)
{
    RtpPacketData* rtpPacket = (RtpPacketData*)packet;

    MediaSession* session = (MediaSession*)arg1;
    MediaSession::Track* track = (MediaSession::Track*)arg2;
    
    session->handleSendRtpPacket(track, rtpPacket);
}
void MediaSession::handleSendRtpPacket(MediaSession::Track* track, RtpPacketData* rtpPacket)
{
    std::list<RtpInstance*>::iterator it;
    std::list<RtcpInstance*>::iterator it1 = track->m_rtcpInstances.begin();
    for(it = track->m_rtpInstances.begin(); it != track->m_rtpInstances.end(); ++it,++it1){
        RtpInstance* rtpInstance = *it;
        RtcpInstance* rtcpInstance = *it1;
        // LOGI("sendRtpPacket callback\n");
        if (rtpInstance->isAlive()){
            // LOGI("send rtpPacket\n");
            rtpInstance->send(rtpPacket);
            rtcpInstance->onRtp(rtpPacket);
            if(rtpInstance->getCount() == 10)
            {
                LOGI("send rtcp\n");
                rtcpInstance->send();
                rtpInstance->setCount(0);
            }  
        }
    }
}