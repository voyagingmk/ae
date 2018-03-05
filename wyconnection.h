#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "wykcp.h"
#include "wysockbase.h"

namespace wynet
{

class TCPConnection
{
  public:
    int connfd;
    SockBuffer buf;
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
};

#endif