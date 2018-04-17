#include "wyconnection.h"
#include "wyutils.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"

using namespace wynet;

int testOnTimerEvent(EventLoop *loop, TimerRef tr, void *userData)
{
    printf("testOnTimerEvent %lld\n", tr.Id());
    return LOOP_EVT_NOMORE;
}

void OnTcpMessage(EventLoop *eventLoop,
                  int connfdTcp, void *clientData, int mask)
{
    log_debug("onTcpMessage connfd=%d", connfdTcp);
    TcpConnectionForServer *conn = (TcpConnectionForServer *)(clientData);
    conn->onTcpMessage();

    eventLoop->createTimerInLoop(1000, testOnTimerEvent, NULL);
}

void TcpConnectionForServer::onConnectEstablished() {
        
    m_loop->createFileEvent(connfdTcp, LOOP_EVT_READABLE,
                                   OnTcpMessage, this);
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
