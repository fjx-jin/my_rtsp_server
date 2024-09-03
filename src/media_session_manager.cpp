#include "media_session_manager.h"
#include "media_session.h"

MediaSessionManager* MediaSessionManager::m_instance = new MediaSessionManager();

MediaSessionManager::MediaSessionManager()
{
}

MediaSessionManager::~MediaSessionManager()
{
}

bool MediaSessionManager::addSession(MediaSession* session)
{
    if (m_sessionMap.find(session->name()) != m_sessionMap.end())
        return false;
    else
    {
        m_sessionMap.insert(std::make_pair(session->name(),session));
    }
}

bool MediaSessionManager::removeSession(MediaSession* session)
{
    auto it = m_sessionMap.find(session->name());
    if(it != m_sessionMap.end())
    {
        m_sessionMap.erase(it);
    }
    else
        return false;
}

MediaSession* MediaSessionManager::getSession(const std::string& name)
{
    auto it = m_sessionMap.find(name);
    if(it != m_sessionMap.end())
        return it->second;
    else
        return nullptr;
}