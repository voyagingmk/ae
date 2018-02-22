#include "wyclient.h"

namespace wynet {

Client::Client(aeEventLoop *aeloop, const char *host, int tcpPort):
    tcpClient(host, tcpPort),
    udpClient(NULL),
    kcpDict(NULL)
{

}

Client::~Client() 
{

}

};