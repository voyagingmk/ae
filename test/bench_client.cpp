#include "net.h"
#include "logger/logger.h"
#include "utils.h"
using namespace wynet;
using namespace std::placeholders;
class TestClient
{
  public:
    TestClient(WyNet *net, const char *ip, int port) : m_net(net),
                                                       m_client(Client::create(net))
    {
        PtrTcpClient tcpClient = m_client->initTcpClient(ip, port);
        tcpClient->onTcpConnected = std::bind(&TestClient::OnTcpConnected, this, _1);
        tcpClient->onTcpDisconnected = std::bind(&TestClient::OnTcpDisconnected, this, _1);
        tcpClient->onTcpRecvMessage = std::bind(&TestClient::OnTcpRecvMessage, this, _1, _2);
        m_net->getPeerManager().addClient(m_client);
        m_tcpClient = tcpClient;
    }

    static void OnTcpSendComplete(PtrConn conn)
    {
        log_debug("[test.OnTcpSendComplete]");
        //      std::string msg(buffer, ret_in);
        //      conn->send(msg);

        //      m_net->stopLoop();
        //
    }

    void OnTcpConnected(PtrConn conn)
    {
        log_debug("[test.OnTcpConnected]");
        // socketUtils::SetSockSendBufSize(conn->fd(), 3, true);
        conn->setCallBack_SendComplete(OnTcpSendComplete);
        const char *hello = "hello";
        conn->send((const uint8_t *)hello, sizeof(hello));
        // m_net->stopLoop();
        //client->getTcpClient();
    }

    void OnTcpDisconnected(PtrConn conn)
    {
        log_debug("[test.OnTcpDisconnected] %d", conn->connectId());
        m_net->stopLoop();
    }

    void OnTcpRecvMessage(PtrConn conn, SockBuffer &sockBuf)
    {
        /*
        int readOutSize = write(output_fd,
                                sockBuf.readBegin(),
                                sockBuf.readableSize());
        log_debug("[test.OnTcpRecvMessage] readableSize=%d, readOutSize=%d", sockBuf.readableSize(), readOutSize);
        sockBuf.readOut(sockBuf.readableSize());*/
    }

  private:
    WyNet *m_net;
    PtrClient m_client;
    PtrTcpClient m_tcpClient;
};

WyNet *g_net;

void Stop(int signo)
{
    g_net->stopLoop();
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
    TestClient testClient(&net, ip, port);
    net.startLoop();
    log_info("exit");
    return 0;
}
