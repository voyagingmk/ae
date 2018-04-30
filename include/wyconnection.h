#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "uniqid.h"
#include "wykcp.h"
#include "wysockbase.h"
#include "wysockbuffer.h"
#include "noncopyable.h"

namespace wynet
{
class EventLoop;

class TCPServer;
class TCPClient;

typedef std::shared_ptr<TCPServer> PtrTCPServer;
typedef std::shared_ptr<TCPClient> PtrTCPClient;

class TcpConnection;

typedef std::shared_ptr<TcpConnection> PtrConn;

class TcpConnectionForServer;

class TcpConnectionForClient;

typedef TcpConnectionForServer SerConn;

typedef TcpConnectionForClient CliConn;

typedef std::shared_ptr<TcpConnection> PtrConn;

typedef std::shared_ptr<SerConn> PtrSerConn;

typedef std::shared_ptr<CliConn> PtrCliConn;

class TcpConnection : public SocketBase
{
  public:
    typedef void (*OnTcpConnected)(PtrConn conn);

    typedef void (*OnTcpDisconnected)(PtrConn conn);

    typedef void (*OnTcpRecvMessage)(PtrConn conn, SockBuffer &sockBuf);

    TcpConnection(int fd) : SocketBase(fd),
                            m_loop(nullptr),
                            m_key(0),
                            m_kcpObj(nullptr),
                            m_connectId(0)
    {
    }

    virtual ~TcpConnection()
    {
    }

    std::shared_ptr<TcpConnection> shared_from_this()
    {
        return FDRef::downcasted_shared_from_this<TcpConnection>();
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

    inline void setConnectId(UniqID connectId)
    {
        m_connectId = connectId;
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

    void setCallBack_Connected(OnTcpConnected cb)
    {
        onTcpConnected = cb;
    }

    void setCallBack_Disconnected(OnTcpDisconnected cb)
    {
        onTcpDisconnected = cb;
    }

    void setCallBack_Message(OnTcpRecvMessage cb)
    {
        onTcpRecvMessage = cb;
    }

  protected:
    virtual void onEstablished() {}

    virtual void onReadable() {}

    virtual void onWritable() {}

    void close(bool force);

  protected:
    EventLoop *m_loop;
    uint32_t m_key;
    KCPObject *m_kcpObj;
    UniqID m_connectId;
    SockBuffer m_buf;
    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;
    OnTcpRecvMessage onTcpRecvMessage;
};

class TcpConnectionForServer : public TcpConnection
{
  public:
    TcpConnectionForServer(PtrTCPServer tcpServer, int fd) : TcpConnection(fd),
                                                             m_tcpServer(tcpServer)
    {
    }

    void onEstablished() override;

    void onReadable() override;

    void onWritable() override;

    void send(const uint8_t *data, size_t len);

    friend void OnTcpMessage(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

  private:
    PtrTCPServer m_tcpServer;
};

class TcpConnectionForClient : public TcpConnection
{
  public:
    TcpConnectionForClient(PtrTCPClient tcpClient, int fd) : TcpConnection(fd),
                                                             m_tcpClient(tcpClient)
    {
    }

    void onEstablished() override;

    void onReadable() override;

    void onWritable() override;

  private:
    PtrTCPClient m_tcpClient;
};
};

#endif
