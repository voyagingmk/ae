#include "wynet.h"
#include "logger/logger.h"
#include "wyutils.h"
using namespace wynet;

WyNet *g_net;

void Stop(int signo)
{
    g_net->stopLoop();
}

int OnHeartbeat(EventLoop *loop, TimerRef tr, std::weak_ptr<FDRef> fdRef, void *data)
{  
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return 0;
    }
    PtrConn conn = std::dynamic_pointer_cast<TcpConnection>(sfdRef);
    
    conn->send((const uint8_t *)"hello", 5);
    return 1000;
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected] %d", conn->connectId());
    SetSockSendBufSize(conn->fd(), 3, true);
    //client->getTcpClient();
    //log_info("OnTcpConnected: %d", client->getTcpClient()->sockfd());
    conn->getLoop()->createTimer(1000, OnHeartbeat, conn, nullptr);
}

void OnTcpDisconnected(PtrConn conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
    //log_info("OnTcpDisconnected: %d", client->getTcpClient()->sockfd());
    g_net->stopLoop();
}

void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
{
    log_debug("[test.OnTcpRecvMessage] readableSize=%d", sockBuf.readableSize());
    sockBuf.readOut(sockBuf.readableSize());
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("test_client");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_start();

    WyNet net(1);
    g_net = &net;

    log_info("aeGetApiName: %s", aeGetApiName());
    std::shared_ptr<Client> client = std::make_shared<Client>(&net);
    std::shared_ptr<TCPClient> tcpClient = client->initTcpClient("127.0.0.1", 9998);
    tcpClient->onTcpConnected = &OnTcpConnected;
    tcpClient->onTcpDisconnected = &OnTcpDisconnected;
    tcpClient->onTcpRecvMessage = &OnTcpRecvMessage;
    net.addClient(client);
    net.startLoop();
    log_info("exit");
    return 0;
}
