#include "wynet.h"
#include "logger/logger.h"
#include "wyutils.h"
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

void OnTcpSendComplete(PtrConn conn)
{
    log_debug("[test.OnTcpSendComplete]");
    int ret_in = read(input_fd, &buffer, BUF_SIZE);
    if (ret_in > 0)
    {
        std::string msg(buffer, ret_in);
        conn->send(msg);
    }
}

void OnTcpConnected(PtrConn conn)
{
    log_debug("[test.OnTcpConnected]");
    // SetSockSendBufSize(conn->fd(), 3, true);
    conn->setCallBack_SendComplete(OnTcpSendComplete);

    input_fd = open("testdata", O_RDONLY);
    if (input_fd == -1)
    {
        log_fatal("open");
        return;
    }

    /* Create output file descriptor */
    output_fd = open("testdata.out", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1)
    {
        log_fatal("open");
        return;
    }

    int ret_in = read(input_fd, &buffer, BUF_SIZE);
    if (ret_in > 0)
    {
        conn->send((const uint8_t *)buffer, ret_in);
    }
    //client->getTcpClient();
    //log_info("OnTcpConnected: %d", client->getTcpClient()->sockfd());
    // conn->getLoop()->createTimer(1000, OnHeartbeat, conn, nullptr);
}

void OnTcpDisconnected(PtrConn conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
    //log_info("OnTcpDisconnected: %d", client->getTcpClient()->sockfd());
    g_net->stopLoop();
    ::close(input_fd);
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
