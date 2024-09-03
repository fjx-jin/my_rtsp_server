#ifndef MEDIA_SESSION_MANAGER_H
#define MEDIA_SESSION_MANAGER_H
#include <string>
#include <map>

class MediaSession;
class MediaSessionManager
{
public:
    static MediaSessionManager* instance() {return m_instance;}

    MediaSessionManager();
	~MediaSessionManager();

	bool addSession(MediaSession* session);
	bool removeSession(MediaSession* session);
	MediaSession* getSession(const std::string& name);
private:
    static MediaSessionManager* m_instance;
    std::map<std::string,MediaSession*> m_sessionMap;
};

#endif