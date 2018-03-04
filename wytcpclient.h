#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{

class Client;
    
class TCPClient : public SocketBase
{
public:
  sockaddr_in m_serSockaddr;
  struct hostent *h;
  Client* parent;
    
public:
    
  TCPClient(Client* client, const char *host, int port);
    
  void Close();
    
  void Send(const char *data, size_t len);
    
  void Recvfrom();
    
  void onConnected();
};
};

#endif
