#include "net.h"
#include "logger/logger.h"
#include "utils.h"
using namespace wynet;

WyNet *g_net;
int input_fd;
int output_fd;
#define BUF_SIZE 8
char buffer[BUF_SIZE];

void Stop(int signo)
{
    g_net->stopLoop();
}

int OnHeartbeat(EventLoop *loop, TimerRef tr, PtrEvtListener listener, void *data)
{
    return 1000;
}

void OnTcpSendComplete(PtrConn conn)
{
    log_debug("[test.OnTcpSendComplete]");
    int ret_in = read(input_fd, &buffer, BUF_SIZE);
    if (ret_in > 0)
    {
        std::string msg(buffer, ret_in);
        conn->send(msg);
    }
    else
    {
        g_net->stopLoop();
    }
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected]");
    // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
    conn->setCallBack_SendComplete(OnTcpSendComplete);
    // int ret_in = read(input_fd, &buffer, BUF_SIZE);
    const char *hello = "hello";
    conn->send((const uint8_t *)hello, sizeof(hello));
    // g_net->stopLoop();
    //client->getTcpClient();
    // conn->m_evtListener->createTimer(1000, OnHeartbeat);
}

void OnTcpDisconnected(PtrConn conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
    g_net->stopLoop();
}

void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
{
    int readOutSize = write(output_fd,
                            sockBuf.readBegin(),
                            sockBuf.readableSize());
    log_debug("[test.OnTcpRecvMessage] readableSize=%d, readOutSize=%d", sockBuf.readableSize(), readOutSize);
    sockBuf.readOut(sockBuf.readableSize());
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "cmd args: <address> <port>\n");
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, Stop);

    // log_file("bench_client");
    log_level(LOG_LEVEL::LOG_DEBUG);
    log_lineinfo(false);
    // log_file_start();

    const char *ip = argv[1];
    int port = static_cast<int>(atoi(argv[2]));
    const int threadsNum = 1;
    WyNet net(threadsNum);
    g_net = &net;

    log_info("aeGetApiName: %s", aeGetApiName());
    PtrClient client = std::make_shared<Client>(&net);
    PtrTCPClient tcpClient = client->initTcpClient(ip, port);
    tcpClient->onTcpConnected = &OnTcpConnected;
    tcpClient->onTcpDisconnected = &OnTcpDisconnected;
    tcpClient->onTcpRecvMessage = &OnTcpRecvMessage;
    net.addClient(client);
    net.startLoop();
    ::close(input_fd);
    log_info("exit");
    return 0;
}
