#include "net.h"
#include "utils.h"

namespace wynet
{

PeerManager::PeerManager()
{
    log_ctor("PeerManager()");
}

PeerManager::~PeerManager()
{
    log_dtor("~PeerManager()");
}

UniqID PeerManager::addServer(PtrServer server)
{
    UniqID serverId = m_serverIdGen.getNewID();
    m_servers[serverId] = server;
    return serverId;
}

bool PeerManager::removeServer(UniqID serverId)
{
    Servers::iterator it = m_servers.find(serverId);
    if (it == m_servers.end())
    {
        return false;
    }
    m_servers.erase(it);
    m_serverIdGen.recycleID(serverId);
    return true;
}

WyNet::WyNet(int threadNum) : m_threadPool(&m_loop, "WyNet", threadNum)
{
    log_ctor("WyNet()");
    checkOpenFileNum(100000);
    ignoreSignalPipe();
    m_threadPool.start([](EventLoop *loop) -> void {
        log_info("ThreadInitCallback");
    });
}

WyNet::~WyNet()
{
    log_dtor("~WyNet()");
}

void WyNet::stopLoop()
{
    m_loop.stopSafely();
}

void WyNet::stopAllLoop()
{
    log_info("stopAllLoop()");
    m_threadPool.stopAndJoinAll();
    m_loop.stopSafely();
}

void WyNet::startLoop()
{
    m_loop.loop();
}
}; // namespace wynet
