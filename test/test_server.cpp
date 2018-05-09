#include "wynet.h"
#include "logger/logger.h"
using namespace wynet;

WyNet *g_net;
PtrConn g_conn1, g_conn2;
char lineBuffer[1024];

void Stop(int signo)
{
    g_net->stopLoop();
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected] %d", conn->connectId());
    if (!g_conn1)
    {
        g_conn1 = conn;
    }
    if (!g_conn2)
    {
        g_conn2 = conn;
    }
}

void OnTcpDisconnected(PtrConn conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
}

void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
{

    log_debug("[test.OnTcpRecvMessage] readableSize=%d", sockBuf.readableSize());

    /*
    if (g_conn1 && g_conn2)
    {
        if (g_conn1 == conn)
        {
            g_conn2->send((const uint8_t *)"hello conn2", 11);
        }
        else
        {
            g_conn1->send((const uint8_t *)"hello conn1", 11);
        }
    }
    */
    conn->send(sockBuf.begin() + sockBuf.headFreeSize(), sockBuf.readableSize());
    memcpy(lineBuffer, sockBuf.begin() + sockBuf.headFreeSize(), sockBuf.readableSize());
    lineBuffer[sockBuf.readableSize()] = '\0';
    log_info("%d:%s", sockBuf.readableSize(), lineBuffer);
    sockBuf.readOut(sockBuf.readableSize());
}

/*
static int SocketOutput(const char *buf, int len, ikcpcb *kcp, void *user)
{
    SocketBase *s = (SocketBase *)user;
    assert(s);
    // s->send(buf, len);
    return len;
}
*/

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("test_server");
    log_level(LOG_LEVEL::LOG_INFO);
    log_lineinfo(false);
    // log_start();

    WyNet net;
    g_net = &net;

    //  UDPServer server(9999);
    //  KCPObject kcpObject(9999, &server, &SocketOutput);
    log_info("aeGetApiName: %s", aeGetApiName());
    std::shared_ptr<Server> server = std::make_shared<Server>(&net);
    std::shared_ptr<TCPServer> tcpServer = server->initTcpServer(9998);
    server->initUdpServer(9999);
    tcpServer->onTcpConnected = &OnTcpConnected;
    tcpServer->onTcpDisconnected = &OnTcpDisconnected;
    tcpServer->onTcpRecvMessage = &OnTcpRecvMessage;
    net.addServer(server);
    net.startLoop();
    return 0;
}
