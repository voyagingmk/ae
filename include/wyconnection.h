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

class TcpConnection : public SocketBase
{
  public:
    TcpConnection(int fd) : SocketBase(fd),
                            m_loop(nullptr),
                            m_key(0),
                            m_kcpObj(nullptr),
                            m_connectId(0)
    {
    }

    TcpConnection &operator=(TcpConnection &&c)
    {
        m_buf = std::move(c.m_buf);
        m_key = c.m_key;
        m_kcpObj = c.m_kcpObj;
        c.m_key = 0;
        c.m_kcpObj = nullptr;
        return *this;
    }

    inline void setEventLoop(EventLoop *l)
    {
        m_loop = l;
    }

    EventLoop *getLoop() const
    {
        return m_loop;
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

    inline SockBuffer &sockBuffer()
    {
        return m_buf;
    }

  private:
    EventLoop *m_loop;
    uint32_t m_key;
    KCPObject *m_kcpObj;
    UniqID m_connectId;
    SockBuffer m_buf;
};

class TcpConnectionForServer : public TcpConnection
{
  public:
    TcpConnectionForServer(int fd) : TcpConnection(fd)
    {
    }

    void onConnectEstablished();

    void onReadable();

    void onWritable();

    void send(const uint8_t *data, size_t len);

    friend void OnTcpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

  private:
    void close(bool force);
};

class TcpConnectionForClient : public TcpConnection
{
  public:
    TcpConnectionForClient(int fd) : TcpConnection(fd)
    {
    }
    void setUdpPort(int udpPort)
    {
        m_udpPort = udpPort;
    }

  private:
    int m_udpPort;
};

typedef TcpConnectionForServer SerConn;

typedef TcpConnectionForClient CliConn;

typedef std::shared_ptr<TcpConnection> PtrConn;

typedef std::shared_ptr<SerConn> PtrSerConn;

typedef std::shared_ptr<CliConn> PtrCliConn;
};

#endif
