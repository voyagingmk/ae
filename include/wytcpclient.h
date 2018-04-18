#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{

class Client;
typedef std::shared_ptr<Client> PtrClient;
    
class TCPClient : public SocketBase
{
public: 
    sockaddr_in m_serSockaddr;
    struct hostent *h;
    PtrClient parent;
    bool connected;
public:
    std::shared_ptr<TCPClient> shared_from_this() {
        return FDRef::downcasted_shared_from_this<TCPClient>(); 
    }

    TCPClient(PtrClient client, const char *host, int port);
    
    void Close();
    
    void Send(uint8_t *data, size_t len);
    
    void Recvfrom();
    
    void onConnected();

private:
    
    friend void OnTcpWritable(struct aeEventLoop *eventLoop, void *clientData, int mask);
};

};

#endif
