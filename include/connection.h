#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "uniqid.h"
#include "kcp.h"
#include "sockbase.h"
#include "sockbuffer.h"
#include "noncopyable.h"
#include "event_listener.h"

namespace wynet
{
class EventLoop;

class TcpConnection;
class TCPServer;
class TCPClient;
class TcpConnectionEventListener;

typedef std::shared_ptr<TCPServer> PtrTCPServer;
typedef std::shared_ptr<TCPClient> PtrTCPClient;
typedef std::weak_ptr<TCPServer> WeakPtrTCPServer;
typedef std::weak_ptr<TCPClient> WeakPtrTCPClient;
typedef std::shared_ptr<TcpConnection> PtrConn;
typedef std::weak_ptr<TcpConnection> PtrConnWeak;
typedef std::shared_ptr<TcpConnectionEventListener> PtrConnEvtListener;

class TcpConnectionEventListener : public EventListener
{
  public:
    void setTcpConnection(PtrConn conn)
    {
        m_conn = conn;
    }
    PtrConn getTcpConnection()
    {
        PtrConn conn = m_conn.lock();
        return conn;
    }

    static PtrConnEvtListener create()
    {
        return std::make_shared<TcpConnectionEventListener>();
    }
    /*
    static PtrEvtListener create()
    {
        return std::static_pointer_cast<EventListener>(std::make_shared<TcpConnectionEventListener>());
    }*/

  protected:
    PtrConnWeak m_conn;
};

class TcpConnection : public Noncopyable, public std::enable_shared_from_this<TcpConnection>
{
    friend class TCPServer;
    friend class TCPClient;

  public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
    };
    typedef void (*OnTcpConnected)(PtrConn conn);

    typedef void (*OnTcpDisconnected)(PtrConn conn);

    typedef void (*OnTcpRecvMessage)(PtrConn conn, SockBuffer &sockBuf);

    typedef void (*OnTcpSendComplete)(PtrConn conn);

    TcpConnection(SockFd sockfd = 0) : m_loop(nullptr),
                                       m_sockFdCtrl(sockfd),
                                       m_state(State::Connecting),
                                       m_key(0),
                                       m_kcpObj(nullptr),
                                       m_connectId(0),
                                       onTcpConnected(nullptr),
                                       onTcpDisconnected(nullptr),
                                       onTcpRecvMessage(nullptr),
                                       onTcpSendComplete(nullptr)
    {
        m_evtListener = TcpConnectionEventListener::create();
        m_evtListener->setSockFd(m_sockFdCtrl.sockfd());
    }

    virtual ~TcpConnection()
    {
    }

    TcpConnection &operator=(TcpConnection &&c)
    {
        m_pendingRecvBuf = std::move(c.m_pendingRecvBuf);
        m_key = c.m_key;
        m_kcpObj = c.m_kcpObj;
        c.m_key = 0;
        c.m_kcpObj = nullptr;
        return *this;
    }

    inline void setEventLoop(EventLoop *l)
    {
        m_loop = l;
        m_evtListener->setEventLoop(m_loop);
    }

    void setCtrl(PtrTCPServer ctrl)
    {
        m_tcpServer = ctrl;
    }

    void setCtrl(PtrTCPClient ctrl)
    {
        m_tcpClient = ctrl;
    }

    // may not exist
    PtrTCPServer getCtrlAsServer()
    {
        return m_tcpServer.lock();
    }

    PtrTCPClient getCtrlAsClient()
    {
        return m_tcpClient.lock();
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

    inline SockFd connectFd()
    {
        return m_sockFdCtrl.sockfd();
    }

    inline SockBuffer &sockBuffer()
    {
        return m_pendingRecvBuf;
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

    void setCallBack_SendComplete(OnTcpSendComplete cb)
    {
        onTcpSendComplete = cb;
    }

    void close(bool force);

    void send(const uint8_t *data, const size_t len);

    void send(const std::string &);

    bool isPending();

    int getPendingSize();

  protected:
    static void OnConnectionEvent(EventLoop *eventLoop, PtrEvtListener listener, int mask);

    void onEstablished();

    void onReadable();

    void onWritable();

    void sendInLoop(const uint8_t *data, const size_t len);

    void sendInLoop(const std::string &);

    void closeInLoop(bool force);

  protected:
    EventLoop *m_loop;
    SocketFdCtrl m_sockFdCtrl;
    PtrConnEvtListener m_evtListener;
    WeakPtrTCPServer m_tcpServer;
    WeakPtrTCPClient m_tcpClient;
    State m_state;
    uint32_t m_key;
    KCPObject *m_kcpObj;
    UniqID m_connectId;
    SockBuffer m_pendingRecvBuf;
    SockBuffer m_pendingSendBuf;

    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;
    OnTcpRecvMessage onTcpRecvMessage;
    OnTcpSendComplete onTcpSendComplete;
};
}; // namespace wynet

#endif
