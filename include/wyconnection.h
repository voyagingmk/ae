#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "wykcp.h"
#include "wysockbase.h"
#include "noncopyable.h"

namespace wynet
{
class EventLoop;

class TcpConnection : public Noncopyable, public FDRef
{
  public:
    EventLoop *m_loop;
    uint32_t key;
    KCPObject *kcpObj;
    uint32_t connectId;

    TcpConnection() : m_loop(nullptr),
                      key(0),
                      kcpObj(nullptr),
                      connectId(0)
    {
    }

    inline void setEventLoop(EventLoop *l)
    {
        m_loop = l;
    }

    inline void setConnectFd(int fd)
    {
        setfd(fd);
    }  

    inline void setConnectId(uint32_t _connectId)
    {
        connectId = _connectId;
    }  

    inline void setKey(uint32_t k)
    {
        key = k;
    }

    inline ConvID convId()
    {
        return key & 0x0000ffff;
    }

    inline uint16_t passwd()
    {
        return key >> 16;
    }

    inline uint16_t connectFd()
    {
        return fd();
    }

    TcpConnection &operator=(TcpConnection &&c)
    {
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
    SockBuffer buf;
    TcpConnectionForServer &operator=(TcpConnectionForServer &&c)
    {
        buf = std::move(c.buf);
        return *this;
    }
    void onConnectEstablished();

    void onTcpMessage();

    void send(const uint8_t *data, size_t len);

    friend void OnTcpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);
};

class TcpConnectionForClient : public TcpConnection
{
  public:
    int udpPort;
};

typedef TcpConnectionForServer SerConn;

typedef TcpConnectionForClient CliConn;

typedef std::shared_ptr<TcpConnection> PtrConn;

typedef std::shared_ptr<SerConn> PtrSerConn;

typedef std::shared_ptr<CliConn> PtrCliConn;
};

#endif
