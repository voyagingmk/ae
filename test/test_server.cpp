#include "wynet.h"
#include "logger/logger.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.stopLoop();
}

void OnTcpConnected(PtrServer server, PtrSerConn conn)
{
    log_debug("[OnTcpConnected] %d", conn->connectId());
}

void OnTcpDisconnected(PtrServer server, PtrSerConn conn)
{
    log_debug("[OnTcpDisconnected] %d", conn->connectId());
}

void OnTcpRecvMessage(PtrServer server, PtrSerConn conn, uint8_t *p, size_t len)
{

    log_debug("[OnTcpRecvMessage] %d, %s", conn->connectId(), (const char *)p);
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
    if (argc > 1)
    {
        log_set_level((int)(*argv[1]));
    }
    Logger logger("test");
    log_set_file("./server.log", "w+");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);
    //  UDPServer server(9999);
    //  KCPObject kcpObject(9999, &server, &SocketOutput);
    log_info("aeGetApiName: %s", aeGetApiName());
    std::shared_ptr<Server> server = std::make_shared<Server>(&net, 9998, 9999);
    server->onTcpConnected = &OnTcpConnected;
    server->onTcpDisconnected = &OnTcpDisconnected;
    server->onTcpRecvMessage = &OnTcpRecvMessage;
    net.addServer(server);
    net.startLoop();
    return 0;
}
