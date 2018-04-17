#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "wykcp.h"
#include "wysockbase.h"
#include "noncopyable.h"

namespace wynet
{

// for server only
class Connection: public Noncopyable 
{
  public:
    uint32_t key;
    KCPObject *kcpObj;

    Connection() : key(0),
                   kcpObj(nullptr)
    {
    }

    ConvID convId()
    {
        return key & 0x0000ffff;
    }

    uint16_t passwd()
    {
        return key >> 16;
    }
    
    Connection& operator=(Connection && c) {
        key = c.key;
        kcpObj = c.kcpObj;
        c.key = 0;
        c.kcpObj = nullptr;
        return *this;
    }
};

class ConnectionForServer : public Connection
{
  public:
    int connfdTcp;
    SockBuffer buf;
    ConnectionForServer& operator=(ConnectionForServer && c) {
        connfdTcp = c.connfdTcp;
        buf = std::move(c.buf);
        c.connfdTcp = 0;
        return *this;
    }
};

class ConnectionForClient : public Connection
{
  public:
    int udpPort;
    uint32_t clientId;
};



typedef ConnectionForServer SerConn;
typedef std::shared_ptr<SerConn> PtrSerConn;

};

#endif
