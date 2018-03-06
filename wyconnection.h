#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "wykcp.h"
#include "wysockbase.h"

namespace wynet
{

// for server only
class Connection
{
  public:
    uint32_t key;
    KCPObject *kcpDict;

    ConvID convId()
    {
        return key & 0x0000ffff;
    }

    uint16_t passwd()
    {
        return key >> 16;
    }
};
    
class ConnectionForServer: public Connection
{
public:
    int connfd;
    SockBuffer buf;
};
    
class ConnectionForClient: public Connection
{
public:
    int udpPort;
    uint32_t clientId;
};
    
};

#endif
