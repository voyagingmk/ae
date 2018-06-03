#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "uniqid.h"
#include "kcp.h"
#include "sockbase.h"
#include "sockbuffer.h"
#include "noncopyable.h"
#include "event_listener.h"
#include "utils.h"
#include "any.h"

namespace wynet
{
class EventLoop;

class TcpConnection;
class TcpServer;
class TcpClient;
class TcpConnectionEventListener;

using PtrTcpServer = std::shared_ptr<TcpServer>;
using PtrTcpClient = std::shared_ptr<TcpClient>;
using WeakPtrTcpServer = std::weak_ptr<TcpServer>;
using WeakPtrTcpClient = std::weak_ptr<TcpClient>;
using PtrConn = std::shared_ptr<TcpConnection>;
using PtrConnWeak = std::weak_ptr<TcpConnection>;
using PtrConnEvtListener = std::shared_ptr<TcpConnectionEventListener>;

class TcpConnectionEventListener final : public EventListener
{
  public:
    ctor_dtor_forlogging(TcpConnectionEventListener);

    void setTcpConnection(const PtrConn &conn)
    {
        m_conn = conn;
    }
    PtrConn getTcpConnection()
    {
        PtrConn conn = m_conn.lock();
        return conn;
    }

    static PtrConnEvtListener create(SockFd sockfd)
    {
        PtrConnEvtListener l = std::make_shared<TcpConnectionEventListener>();
        l->setSockfd(sockfd);
        return l;
    }
    /*
    static PtrEvtListener create()
    {
        return std::static_pointer_cast<EventListener>(std::make_shared<TcpConnectionEventListener>());
    }*/

  protected:
    PtrConnWeak m_conn;
};

class TcpConnection final : public Noncopyable, public std::enable_shared_from_this<TcpConnection>
{
    friend class TcpServer;
    friend class TcpClient;

  public:
    enum class State
    {
        Connecting,
        Connected,
        Disconnecting,
        Disconnected
    };
    using OnTcpConnected = std::function<void(const PtrConn &)>;

    using OnTcpDisconnected = std::function<void(const PtrConn &)>;

    using OnTcpSendComplete = std::function<void(const PtrConn &)>;

    using OnTcpClose = std::function<void(const PtrConn &)>;

    using OnTcpRecvMessage = std::function<void(const PtrConn &, SockBuffer &)>;

    TcpConnection(SockFd sockfd = 0);

    virtual ~TcpConnection();

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
        assert(!m_loop);
        m_loop = l;
        m_evtListener->setEventLoop(m_loop);
    }

    PtrConnEvtListener getListener()
    {
        return m_evtListener;
    }

    void setCtrl(PtrTcpServer ctrl)
    {
        assert(m_ctrlType == 0);
        m_ctrlType = 1;
        m_ctrl = ctrl;
    }

    void setCtrl(PtrTcpClient ctrl)
    {
        assert(m_ctrlType == 0);
        m_ctrlType = 2;
        m_ctrl = ctrl;
    }

    // may not exist
    PtrTcpServer getCtrlAsServer()
    {
        std::shared_ptr<void> p = m_ctrl.lock();
        return std::static_pointer_cast<TcpServer>(p);
    }

    PtrTcpClient getCtrlAsClient()
    {
        std::shared_ptr<void> p = m_ctrl.lock();
        return std::static_pointer_cast<TcpClient>(p);
    }

    EventLoop *getLoop() const
    {
        return m_loop;
    }

    void setUserData(Any userData)
    {
        m_userData = userData;
    }

    Any getUserData() const
    {
        return m_userData;
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

    inline SockFd sockfd()
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

    void setCallBack_Message(OnTcpRecvMessage cb)
    {
        onTcpRecvMessage = cb;
    }

    void setCallBack_SendComplete(OnTcpSendComplete cb)
    {
        onTcpSendComplete = cb;
    }

    void shutdown();

    void forceClose();

    void send(const uint8_t *data, const size_t len);

    void send(const std::string &);

    bool isPending();

    int getPendingSize();

    State getState()
    {
        return m_state;
    }

    // internal
    void setCloseCallback(const OnTcpClose &cb);

    // internal
    void onDestroy();

  protected:
    static void OnConnectionEvent(EventLoop *eventLoop, PtrEvtListener listener, int mask);

    void forceCloseInLoop();

    void close(const char *reason);

    void closeInLoop();

    void onEstablished();

    void onReadable();

    void onWritable();

    void sendInLoop(const uint8_t *data, const size_t len);

    void sendInLoop(const std::string &);

    void shutdownInLoop();

  protected:
    EventLoop *m_loop;
    SocketFdCtrl m_sockFdCtrl;
    PtrConnEvtListener m_evtListener;
    uint8_t m_ctrlType;         // 0: not set 1: PtrTcpServer 2: PtrTcpClient
    std::weak_ptr<void> m_ctrl; // PtrTcpServer / PtrTcpClient
    std::atomic<State> m_state;
    uint32_t m_key;
    KCPObject *m_kcpObj;
    UniqID m_connectId;
    SockBuffer m_pendingRecvBuf;
    SockBuffer m_pendingSendBuf;
    Any m_userData;
    bool m_shutdownWrite;

    OnTcpConnected onTcpConnected;
    OnTcpRecvMessage onTcpRecvMessage;
    OnTcpSendComplete onTcpSendComplete;
    OnTcpClose onTcpClose;
};
}; // namespace wynet

#endif
