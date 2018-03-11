#include "wynet.h"
#include "logger/logger.h"
using namespace wynet;

WyNet net;

void Stop(int signo)
{
    net.StopLoop();
    log_info("Stop. %d", net.aeloop->stop);
}


void OnTcpConnected(Server * server, UniqID clientId)
{
    log_debug("[OnTcpConnected] %d", clientId);
}

void OnTcpDisconnected(Server * server, UniqID clientId)
{
    log_debug("[OnTcpDisconnected] %d", clientId);
}

void OnTcpRecvUserData(Server * server, UniqID clientId, uint8_t* p, size_t len) {
    
    log_debug("[OnTcpRecvUserData] %d, %s", clientId, (const char*)p);
}


int main(int argc, char **argv)
{
    if (argc > 1) {
        log_set_level((int)(*argv[1]));
    }
    Logger logger("test");
    log_set_file("./server.log", "w+");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);
    //  UDPServer server(9999);
    //  KCPObject kcpObject(9999, &server, &SocketOutput);
    log_info("aeGetApiName: %s", aeGetApiName());
    Server *server = new Server(&net, 9998, 9999);
    server->onTcpConnected = &OnTcpConnected;
    server->onTcpDisconnected = &OnTcpDisconnected;
    server->onTcpRecvUserData = &OnTcpRecvUserData;
    net.AddServer(server);
    net.Loop();
    return 0;
}
