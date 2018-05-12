#ifndef WY_CONNECTION_H
#define WY_CONNECTION_H

#include "common.h"
#include "uniqid.h"
#include "kcp.h"
#include "sockbase.h"
#include "sockbuffer.h"
#include "noncopyable.h"

namespace wynet
{
class EventLoop;

class TcpConnection;
class TCPServer;
class TCPClient;

typedef std::shared_ptr<TCPServer> PtrTCPServer;
typedef std::shared_ptr<TCPClient> PtrTCPClient;
typedef std::shared_ptr<TcpConnection> PtrConn;
typedef std::weak_ptr<TcpConnection> PtrConnWeak;

class TcpConnection : public SocketBase
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

    TcpConnection(int fd = 0) : SocketBase(fd),
                                m_loop(nullptr),
                                m_state(State::Connecting),
                                m_key(0),
                                m_kcpObj(nullptr),
                                m_connectId(0),
                                onTcpConnected(nullptr),
                                onTcpDisconnected(nullptr),
                                onTcpRecvMessage(nullptr),
                                onTcpSendComplete(nullptr)
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
    }

    void setCtrl(PtrCtrl ctrl)
    {
        m_ctrl = ctrl;
    }

    // may not exist
    PtrCtrl getCtrl()
    {
        PtrCtrl ctrl(m_ctrl.lock());
        return ctrl;
    }

    PtrTCPServer getCtrlAsServer()
    {
        PtrTCPServer tcpServer = std::dynamic_pointer_cast<TCPServer>(getCtrl());
        return tcpServer;
    }

    PtrTCPClient getCtrlAsClient()
    {
        PtrTCPClient tcpClient = std::dynamic_pointer_cast<TCPClient>(getCtrl());
        return tcpClient;
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
    static void OnConnectionEvent(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask);

    void onEstablished();

    void onReadable();

    void onWritable();

    void sendInLoop(const uint8_t *data, const size_t len);

    void sendInLoop(const std::string &);

    void closeInLoop(bool force);

  protected:
    EventLoop *m_loop;
    PtrCtrlWeak m_ctrl;
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
}; // namespace wynet
}; // namespace wynet

#endif
