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

void OnTcpMessage(EventLoop *eventLoop,
                  int connfdTcp, std::weak_ptr<FDRef> fdRef, int mask)
{
    log_debug("onTcpMessage connfd=%d", connfdTcp);
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef) {
        return;
    }
    std::shared_ptr<TcpConnectionForServer> conn = std::dynamic_pointer_cast<TcpConnectionForServer>(sfdRef);
    
    conn->onTcpMessage();

    eventLoop->createTimerInLoop(1000, testOnTimerEvent, nullptr, nullptr);
}

void TcpConnectionForServer::onConnectEstablished() {
        
    m_loop->createFileEvent(connfdTcp, LOOP_EVT_READABLE,
                                   OnTcpMessage, shared_from_this());
    /*
    protocol::TcpHandshake handshake;
    handshake.clientId = clientId;
    handshake.udpPort = m_udpPort;
    handshake.key = conn->key;
    sendByTcp(clientId, SerializeProtocol<protocol::TcpHandshake>(handshake));
    log_info("[Server][tcp] connected, clientId: %d, connfdTcp: %d, key: %d", clientId, connfdTcp, handshake.key);
    */
    LogSocketState(connfdTcp);
}

void TcpConnectionForServer::onTcpMessage() {

}
