#include "rtsp_connection.h"
#include "Log.h"
#include "epoller.h"
#include "thread_pool.h"
#include "socket_util.h"
#include "media_session_manager.h"
#include "media_session.h"
#include "rtp.h"
#include <string.h>
#include <unistd.h>

#define PROJECT_VERSION "DBJ_RTSPSERVER"

RtspConnection* RtspConnection::createNew(int clientFd)
{
    return new RtspConnection(clientFd);
}

RtspConnection::RtspConnection(int clientFd) :
    m_clientFd(clientFd),
    m_sessionId(rand()),
    m_method(RtspConnection::Method::NONE),
    m_isOverTcp(false),
    m_trackId(MediaSession::TrackId::TrackIdNone)
{
    LOGI("RtspConnection mClientFd=%d", clientFd);

    epoll_event ev;
    ev.data.fd = clientFd;
    m_ioEvent = new IOEvent(ev,this);
    m_ioEvent->enableReadHandling();
    m_ioEvent->SetReadCallback(readCallback);
    EPoller::instance()->addEvent(m_ioEvent);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        m_rtpInstances[i] = NULL;
        m_rtcpInstances[i] = NULL;
    }
    m_destIp = sockets::getFdIp(clientFd);
    m_sessionName = "";
}

RtspConnection::~RtspConnection()
{
    //TODO
    EPoller::instance()->removeEvent(m_ioEvent);
    delete m_ioEvent;

    MediaSession* session = MediaSessionManager::instance()->getSession(m_sessionName);
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(m_rtpInstances[i])
        {
            m_rtpInstances[i]->setisAlive(false);
            if(session)
                session->removeRtpInstance((MediaSession::TrackId)i,m_rtpInstances[i]);
                
            delete m_rtpInstances[i];   
        }
    }
}

bool RtspConnection::parseOptions(char* buf)
{
    LOGI("parseOptions\n");
    char method[64] = { 0 };
    char url[512] = { 0 };
    char version[64] = { 0 };
    const char *sep = "\n";
    char *line = strtok(buf, sep);
    while (line)
    {
        if(strstr(line,"OPTIONS"))
        {
            if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                LOGE("prase method error\n");
                return false;
            }
            m_method = OPTIONS;
        }
        else if (strstr(line, "CSeq"))
        {
            if (sscanf(line, "CSeq: %d\r\n", &m_cseq) != 1)
            {
                LOGE("prase cseq error\n");
                return false;
            }
        }
        line = strtok(NULL, sep);
    }
    return true;
}

bool RtspConnection::parseCSeq(char* buf)
{

    return false;
}

bool RtspConnection::parseDescribe(char* buf)
{
    char method[64] = { 0 };
    char url[512] = { 0 };
    char ip[64] = { 0 };
    char suffix[64] = {0};
    char version[64] = { 0 };
    uint16_t port = 0;
    const char *sep = "\n";
    char *line = strtok(buf, sep);
    while (line)
    {
        if(strstr(line,"DESCRIBE"))
        {
            if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                LOGE("prase method error\n");
                return false;
            }
            if (strncmp(url, "rtsp://", 7) != 0)
            {
                return false;
            }
            if (!sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
            {

            }
            m_method = DESCRIBE;
            m_suffix = suffix;
            LOGI("suffix %s\n",suffix);
        }
        else if (strstr(line, "CSeq"))
        {
            if (sscanf(line, "CSeq: %d\r\n", &m_cseq) != 1)
            {
                LOGE("prase cseq error\n");
                return false;
            }
        }
        line = strtok(NULL, sep);
    }
    return true;
}

bool RtspConnection::parseSetup(char* buf)
{
    char method[64] = { 0 };
    char url[512] = { 0 };
    char ip[64] = { 0 };
    char suffix[64] = {0};
    char version[64] = { 0 };
    uint16_t port = 0;
    const char *sep = "\n";
    char *line = strtok(buf, sep);
    while (line)
    {
        if(strstr(line,"SETUP"))
        {
            if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                LOGE("prase method error\n");
                return false;
            }
            if (strncmp(url, "rtsp://", 7) != 0)
            {
                return false;
            }
            if (!sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
            {
                return false;
            }
            if(strstr(url,"track0"))
                m_trackId = MediaSession::TrackId::TrackId0;
            else if(strstr(url,"track1"))
                m_trackId = MediaSession::TrackId::TrackId1;
            m_method = SETUP;
            m_suffix = suffix;
            LOGI("suffix %s trackid %d\n",suffix,m_trackId);
        }
        else if (strstr(line, "CSeq"))
        {
            if (sscanf(line, "CSeq: %d\r\n", &m_cseq) != 1)
            {
                LOGE("prase cseq error\n");
                return false;
            }
        }
        else if(strstr(line,"Transport"))
        {
            char* tmp;
            if((tmp = strstr(line,"RTP/AVP/TCP")))
            {
                m_isOverTcp = true;
                uint8_t rtpChannel, rtcpChannel;

                if (sscanf(tmp, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu",
                        &rtpChannel, &rtcpChannel) != 2)
                {
                    return false;
                }
                m_rtpchannel = rtpChannel;
                m_rtcpchannel = rtcpChannel;
                LOGI("rtpChannel =%d\n",rtpChannel);
            }
            else if((tmp=strstr(line,"RTP/AVP")))
            {
                uint16_t rtpPort = 0, rtcpPort = 0;
                if(sscanf(tmp,"%*[^;];%*[^;];%*[^=]=%hu-%hu",&rtpPort,&rtcpPort) != 2)
                {
                    // LOGE("dwadwadwad\n");
                    return false;
                }
                m_rtpPort = rtpPort;
                m_rtcpPort = rtcpPort;
                m_isOverTcp = false;
                LOGI("tmp %s rtpPort=%d rtcpPort=%d\n",tmp,rtpPort,rtcpPort);
            }
            else
                return false;
        }
        line = strtok(NULL, sep);
    }
    return true;
}

bool RtspConnection::parsePlay(char* buf)
{
    LOGI("parsePlay\n");
    char method[64] = { 0 };
    char url[512] = { 0 };
    char version[64] = { 0 };
    const char *sep = "\n";
    char *line = strtok(buf, sep);
    while (line)
    {
        if(strstr(line,"PLAY"))
        {
            if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                LOGE("prase method error\n");
                return false;
            }
            m_method = PLAY;
        }
        else if (strstr(line, "CSeq"))
        {
            if (sscanf(line, "CSeq: %d\r\n", &m_cseq) != 1)
            {
                LOGE("prase cseq error\n");
                return false;
            }
        }
        line = strtok(NULL, sep);
    }
    return true;
}
bool RtspConnection::handleCmdOption()
{
    initBuf();
    snprintf(mBuffer, sizeof(mBuffer),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Public: DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n"
             "Server: %s\r\n"
             "\r\n", m_cseq,PROJECT_VERSION);

    // int ret = sockets::write(m_clientFd,mBuffer,strlen(mBuffer));
    // LOGI("%d\n",ret);
    // if(ret < 0)
    // //     return false;
    if (sockets::send(m_clientFd,mBuffer,strlen(mBuffer)) < 0)
        return false;
    // if (write(m_clientFd,mBuffer,strlen(mBuffer)) < 0)
    //     return false;
    // LOGI("%s\n",mBuffer);
    return true;
}
bool RtspConnection::handleCmdDescribe()
{
    LOGI("handleCmdDescribe\n");
    MediaSession* session = MediaSessionManager::instance()->getSession(m_suffix);
    if (!session) 
    {
        LOGE("can't find session:%s", m_suffix.c_str());
        return false;
    }
    m_sessionName = m_suffix;
    std::string sdp = session->generateSDPDescription();
    initBuf();
    snprintf((char*)mBuffer, sizeof(mBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Content-Length: %u\r\n"
            "Content-Type: application/sdp\r\n"
            "\r\n"
            "%s",
            m_cseq,
            (unsigned int)sdp.size(),
            sdp.c_str());

    if (sockets::send(m_clientFd,mBuffer,strlen(mBuffer)) < 0)
        return false;
    LOGI("%s\n",mBuffer);
    return true;
}
bool RtspConnection::handleCmdSetup()
{
    LOGI("handleCmdSetup\n");
    char sessionName[100];
    if (sscanf(m_suffix.c_str(), "%[^/]/", sessionName) != 1)
    {
        LOGE("sscanf error\n");
        return false;
    }
    MediaSession* session = MediaSessionManager::instance()->getSession(sessionName);
    if (!session) 
    {
        LOGE("can't find session:%s", m_suffix.c_str());
        return false;
    }
    if (m_trackId >= MEDIA_MAX_TRACK_NUM || m_rtpInstances[m_trackId] || m_rtcpInstances[m_trackId]) 
        return false;
    
    if(m_isOverTcp)
    {
        createRtpOverTcp(m_trackId,m_clientFd,m_rtpchannel);
        createRtcpOverTcp(m_trackId,m_clientFd,m_rtcpchannel);
        session->addRtpInstance(m_trackId,m_rtpInstances[m_trackId],m_rtcpInstances[m_trackId]);
        snprintf((char*)mBuffer, sizeof(mBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Server: %s\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
            "Session: %08x\r\n"
            "\r\n",
            m_cseq,PROJECT_VERSION,
            m_rtpchannel,
            m_rtpchannel + 1,
            m_sessionId);
    }
    else
    {
        if(!createRtpOverUdp(m_trackId,m_destIp,m_rtpPort,m_rtcpPort))
            return false;
        
        session->addRtpInstance(m_trackId,m_rtpInstances[m_trackId],m_rtcpInstances[m_trackId]);
        snprintf((char*)mBuffer, sizeof(mBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Server: %s\r\n"
            "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
            "Session: %08x\r\n"
            "\r\n",
            m_cseq,PROJECT_VERSION,
            m_rtpPort,
            m_rtcpPort,
            m_rtpInstances[m_trackId]->getLocalPort(),
            m_rtcpInstances[m_trackId]->getLocalPort(),
            m_sessionId);
    }

    if (sockets::send(m_clientFd,mBuffer,strlen(mBuffer)) < 0)
        return false;
    return true;
}
bool RtspConnection::handleCmdPlay()
{
    snprintf((char*)mBuffer, sizeof(mBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Server: %s\r\n"
            "Range: npt=0.000-\r\n"
            "Session: %08x; timeout=60\r\n"
            "\r\n",
            m_cseq, PROJECT_VERSION,
            m_sessionId);
    if (sockets::send(m_clientFd,mBuffer,strlen(mBuffer)) < 0)
        return false;

    for(int i = 0;i<MEDIA_MAX_TRACK_NUM;i++)
    {
        if(m_rtpInstances[i])
        {
            LOGI("set rtp live\n");
            m_rtpInstances[i]->setisAlive(true);
        }
            
    }
    LOGI("mediasession %s start play\n",m_sessionName.c_str());
    MediaSession* session = MediaSessionManager::instance()->getSession(m_sessionName);
    if(!session)
        return false;
    session->startPlay();
    return true;
}
bool RtspConnection::handleCmdTeardown()
{
    return false;
}

void RtspConnection::readCallback(void* arg)
{
    RtspConnection* connection = (RtspConnection*)arg;
    connection->handleRead();
}
void RtspConnection::handleRead()
{
    // ThreadPool::instance()->addTask([](){

    // });
    // LOGI("1\n");
    EPoller::instance()->removeEvent(m_ioEvent);
    char* rBuf = (char*)malloc(1024 * 1024);
    char method[64] = { 0 };
    char url[512] = { 0 };
    char version[64] = { 0 };
    // while(1)
    {
        int recvLen;
        // recvLen = recv(m_clientFd,rBuf,2000,0);
        recvLen = read(m_clientFd,rBuf,2000);
        if(recvLen <= 0)
        {
            //TODO 需要处理一些disconnect
            // LOGI("recv %d\n",recvLen);
            goto out;
        }
        rBuf[recvLen] = '\0';
        printf("---------------S->C--------------\n");
        printf("%s \n", rBuf);
        const char *sep = "\n";
        // char *line = strtok(rBuf, sep);
        // while (line)
        {
            // LOGI("line %s\n",line);
            if(strstr(rBuf,"OPTIONS"))
            {
                if(!parseOptions(rBuf))
                    return;
            }
            else if(strstr(rBuf,"DESCRIBE"))
            {
                if(!parseDescribe(rBuf)) 
                    return;
            }
            else if(strstr(rBuf,"SETUP"))
            {
                if(!parseSetup(rBuf))
                {
                    LOGE("parseSetup error\n");
                    return;
                }
            }
            else if(strstr(rBuf,"PLAY"))
            {
                if(!parsePlay(rBuf))
                {
                    LOGE("parsePlay error\n");
                    return;
                }
            }
            // else if (!strncmp(line, "Transport:", strlen("Transport:")))
            // {
            //     if (strstr(line,"RTP/AVP/TCP"))
            //     {
            //         uint8_t rtpChannel, rtcpChannel;
            //         if (sscanf(line, "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",&rtpChannel, &rtcpChannel) != 0) 
            //         {
            //             LOGE("prase Transport error\n");
            //             goto out;
            //         }
            //     }
            //     else if(strstr(line,"RTP/AVP"))
            //     {
            //         if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &m_rtpPort, &m_rtcpPort) != 2)
            //         {
            //             LOGE("prase Transport error\n");
            //             goto out;
            //         }
            //     }
            // }
            // line = strtok(NULL, sep);
        }
        
        switch (m_method)
        {
        case OPTIONS:
            handleCmdOption();
            break;
        case DESCRIBE:
            handleCmdDescribe();
            break;
        case SETUP:
            handleCmdSetup();
            break;
        case PLAY:
            handleCmdPlay();
            break;
        default:
            break;
        }
        // if (!strcmp(method, "OPTIONS")) 
        // {
        //     m_method = OPTIONS;
        //     handleCmdOption();
        //     // goto out;
        // }
        // else if (!strcmp(method, "DESCRIBE")) 
        // {
        //     m_method = DESCRIBE;
        //     handleCmdDescribe();
        // }
        // else if (!strcmp(method, "SETUP")) 
        // {
        //     m_method = SETUP;
        //     handleCmdSetup();
        // }
        // else if (!strcmp(method, "PLAY")) 
        // {
        //     m_method = PLAY;
        //     handleCmdPlay();
        // }
        // else if (!strcmp(method, "TEARDOWN")) 
        // {
        //     m_method = TEARDOWN;
        //     handleCmdTeardown();
        // }
        // else{
        //     m_method = NONE;
        //     return;
        // }
    }
out:
    free(rBuf);
    // LOGI("out\n");
    EPoller::instance()->addEvent(m_ioEvent);
}
void RtspConnection::writeCallback(void* arg)
{

}
void RtspConnection::errorCallback(void* arg)
{

}

void RtspConnection::initBuf()
{
    memset(mBuffer,0,2048);
}

void RtspConnection::createRtpOverTcp(MediaSession::TrackId trackId,int sockfd,int channel)
{
    m_rtpInstances[trackId] = RtpInstance::createNewOverTcp(sockfd,channel);
}

void RtspConnection::createRtcpOverTcp(MediaSession::TrackId trackId,int sockfd,int channel)
{
    m_rtcpInstances[trackId] = RtcpInstance::createNewOverTcp(sockfd,channel);
}
bool RtspConnection::createRtpOverUdp(MediaSession::TrackId trackId,std::string destip,uint16_t destRtpPort,uint16_t destRtcpPort)
{
    int rtpSockfd ,rtcpSockfd;
    uint16_t localRtpPort,localRtcpPort;
    bool ret;

    if(m_rtpInstances[trackId])
        return false;
    
    int i =0;
    for(;i<10;i++)
    {
        rtpSockfd = sockets::createUdpSock();
        if(rtpSockfd < 0)
            continue;

        rtcpSockfd = sockets::createUdpSock();
        if(rtcpSockfd < 0)
        {
            sockets::close(rtpSockfd);
            continue;
        }

        uint16_t port = rand() & 0xfffe;
        localRtpPort = port;
        localRtcpPort = port+1;

        ret = sockets::bind(rtpSockfd, "0.0.0.0", localRtpPort);
        if(!ret)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }
        ret = sockets::bind(rtcpSockfd, "0.0.0.0", localRtcpPort);
        if(!ret)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }
        break;
    }
    if(i == 10)
        return false;

    m_rtpInstances[trackId] = RtpInstance::createNewOverUdp(rtpSockfd,localRtpPort,destip,destRtpPort);
    m_rtcpInstances[trackId] = RtcpInstance::createNewOverUdp(rtcpSockfd,localRtcpPort,destip,destRtcpPort);
    return true;
    //TODO 添加rtcplianjie
}