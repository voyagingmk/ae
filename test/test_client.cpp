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
    // conn->send((const uint8_t *)"hello", 5);
    return 1000;
}

void OnTcpSendComplete(const PtrConn &conn)
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

void OnTcpConnected(const PtrConn &conn)
{
    log_debug("[test.OnTcpConnected]");
    // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
    conn->setCallBack_SendComplete(OnTcpSendComplete);

    input_fd = open("testdata", O_RDONLY);
    if (input_fd == -1)
    {
        log_fatal("open");
        return;
    }

    /* Create output file descriptor */
    output_fd = open("testdata.out", O_WRONLY | O_CREAT | O_TRUNC, 0666);
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
    else
    {
        g_net->stopLoop();
    }
    //client->getTcpClient();
    // conn->getLoop()->createTimer(1000, OnHeartbeat);
}

void OnTcpDisconnected(const PtrConn &conn)
{
    log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
    g_net->stopLoop();
}

void OnTcpRecvMessage(const PtrConn &conn, SockBuffer &sockBuf)
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
    // log_file_start();

    WyNet net(1);
    g_net = &net;

    log_info("aeGetApiName: %s", aeGetApiName());
    PtrClient client = Client::create(&net);
    EventLoop *loop = net.getThreadPool().getNextLoop();
    PtrTcpClient tcpClient = std::make_shared<TcpClient>(loop);
    tcpClient->onTcpConnected = &OnTcpConnected;
    tcpClient->onTcpDisconnected = &OnTcpDisconnected;
    tcpClient->onTcpRecvMessage = &OnTcpRecvMessage;
    tcpClient->connect("127.0.0.1", 9998);
    net.startLoop();
    ::close(input_fd);
    log_info("exit");
    return 0;
}
