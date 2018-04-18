#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wysockbase.h"
#include "noncopyable.h"

namespace wynet
{
class EventLoop;

class TcpConnection : public FDRef
{
  public:
    EventLoop *m_loop;
    uint32_t m_key;
    KCPObject *m_kcpObj;
    UniqID m_connectId;

    TcpConnection() : m_loop(nullptr),
                      m_key(0),
                      m_kcpObj(nullptr),
                      m_connectId(0)
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

    inline void setConnectId(UniqID _connectId)
    {
        m_connectId = _connectId;
    }  

    inline void setKey(uint32_t k)
    {
        m_key = k;
    }

    inline ConvID convId()
    {
        return m_key & 0x0000ffff;
    }

    inline uint16_t passwd()
    {
        return m_key >> 16;
    }

    inline UniqID connectId()
    {
        return m_connectId;
    }

    inline uint16_t connectFd()
    {
        return fd();
    }

    TcpConnection &operator=(TcpConnection &&c)
    {
        m_key = c.m_key;
        m_kcpObj = c.m_kcpObj;
        c.m_key = 0;
        c.m_kcpObj = nullptr;
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
    void setUdpPort(int udpPort) {
        m_udpPort = udpPort;
    }
  public:
    int m_udpPort;
    
};

typedef TcpConnectionForServer SerConn;

typedef TcpConnectionForClient CliConn;

typedef std::shared_ptr<TcpConnection> PtrConn;

typedef std::shared_ptr<SerConn> PtrSerConn;

typedef std::shared_ptr<CliConn> PtrCliConn;
};

#endif
