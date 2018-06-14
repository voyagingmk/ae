#include "net.h"
#include "utils.h"

namespace wynet
{
using namespace std::placeholders;

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
    if (!stopAllLoopWithTimeout(1000))
    {
        static PtrEvtListener listener = EventListener::create();
        m_loop.createTimer(listener, 1000, std::bind(&WyNet::onStopAgain, this, _1, _2, _3, _4), nullptr);
        return;
    }
    log_info("======= stopAllLoop ok =======");
}

bool WyNet::stopAllLoopWithTimeout(int ms)
{
    if (m_threadPool.stopAndJoinAll(ms))
    {
        m_loop.stopSafely();
        return true;
    }
    return false;
}

int WyNet::onStopAgain(EventLoop *, TimerRef tr, PtrEvtListener listener, void *data)
{
    stopAllLoop();
    return MP_HALT;
}

void WyNet::startLoop()
{
    m_loop.loop();
}
}; // namespace wynet
