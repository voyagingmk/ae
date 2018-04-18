#include "wyconnection.h"
#include "wyutils.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"

using namespace wynet;

int testOnTimerEvent(EventLoop *loop, TimerRef tr, std::weak_ptr<FDRef> fdRef, void *data)
{
    printf("testOnTimerEvent %lld\n", tr.Id());
    return LOOP_EVT_NOMORE;
}

void OnTcpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return;
    }
    log_debug("onTcpMessage connfd=%d", sfdRef->fd());
    std::shared_ptr<TcpConnectionForServer> conn = std::dynamic_pointer_cast<TcpConnectionForServer>(sfdRef);

    conn->onTcpMessage();

    eventLoop->createTimerInLoop(1000, testOnTimerEvent, std::weak_ptr<FDRef>(), nullptr);
}

void TcpConnectionForServer::onConnectEstablished()
{

    getLoop()->createFileEvent(shared_from_this(), LOOP_EVT_READABLE, OnTcpMessage);
    /*
    protocol::TcpHandshake handshake;
    handshake.connectId = connectId();
    handshake.udpPort = udpPort();
    handshake.key = key();
    sendByTcp(connectId, SerializeProtocol<protocol::TcpHandshake>(handshake));
    log_info("[Server][tcp] connected, connectId: %d, connfdTcp: %d, key: %d", connectId(), connectFd(), handshake.key);
    */
    LogSocketState(fd());
}

void TcpConnectionForServer::onTcpMessage()
{
}

void TcpConnectionForServer::send(const uint8_t *data, size_t len)
{
}
