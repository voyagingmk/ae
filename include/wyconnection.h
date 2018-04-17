#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "wykcp.h"
#include "wysockbase.h"
#include "noncopyable.h"

namespace wynet
{

class TcpConnection: public Noncopyable 
{
  public:
    uint32_t key;
    KCPObject *kcpObj;

    TcpConnection() : key(0),
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
    
    TcpConnection& operator=(TcpConnection && c) {
        key = c.key;
        kcpObj = c.kcpObj;
        c.key = 0;
        c.kcpObj = nullptr;
        return *this;
    }
};

class TcpConnectionForServer : public TcpConnection
{
  public:
    int connfdTcp;
    SockBuffer buf;
    TcpConnectionForServer& operator=(TcpConnectionForServer && c) {
        connfdTcp = c.connfdTcp;
        buf = std::move(c.buf);
        c.connfdTcp = 0;
        return *this;
    }
    void onConnectEstablished();
};

class TcpConnectionForClient : public TcpConnection
{
  public:
    int udpPort;
    uint32_t clientId;
};



typedef TcpConnectionForServer SerConn;

typedef TcpConnectionForClient CliConn;

typedef std::shared_ptr<TcpConnection> PtrConn;

typedef std::shared_ptr<SerConn> PtrSerConn;

typedef std::shared_ptr<CliConn> PtrCliConn;

};

#endif
