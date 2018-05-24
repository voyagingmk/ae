#include "tcpserver.h"
#include "utils.h"
#include "server.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"
#include "net.h"
#include "sockbuffer.h"
#include "connection_manager.h"
#include "socket_utils.h"

namespace wynet
{

const size_t LISTENQUEUEMAX = 128;

void TCPServer::OnNewTcpConnection(EventLoop *eventLoop, PtrEvtListener listener, int mask)
{
	PtrTcpServerEvtListener l = std::static_pointer_cast<TCPServerEventListener>(listener);
	PtrTCPServer tcpServer = l->getTCPServer();
	tcpServer->acceptConnection();
}

TCPServer::TCPServer(PtrServer parent) : m_parent(parent),
										 m_tcpPort(0),
										 m_connMgr(nullptr),
										 m_sockFdCtrl(0),
										 onTcpConnected(nullptr),
										 onTcpDisconnected(nullptr),
										 onTcpRecvMessage(nullptr)
{
	m_evtListener = TCPServerEventListener::create();
	m_evtListener->setEventLoop(&getLoop());
	// optional
	initConnMgr();
}

PtrConnMgr TCPServer::initConnMgr()
{
	if (!m_connMgr)
	{
		m_connMgr = std::make_shared<ConnectionManager>();
	}
	return m_connMgr;
}

bool TCPServer::addConnection(PtrConn conn)
{
	return getConnMgr()->addConnection(conn);
}

bool TCPServer::removeConnection(PtrConn conn)
{
	return getConnMgr()->removeConnection(conn);
}

void TCPServer::init()
{
	m_evtListener->setTCPServer(shared_from_this());
}

void TCPServer::startListen(const char *host, int port)
{
	m_tcpPort = port;
	int listenfd, n;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char buf[6] = {0};
	sprintf(buf, "%d", m_tcpPort);
	const char *serv = (char *)&buf;
	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
		log_fatal("<TCPServer.startListen> getaddrinfo error for %s, %s: %s",
				  host, serv, gai_strerror(n));
	ressave = res;

	do
	{
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue; /* error, try next one */

		int flags = socketUtils ::sock_fcntl(listenfd, F_GETFL, 0);
		socketUtils ::sock_fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

		int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0)
		{
			log_error("[TCPServer][tcp] setsockopt SO_REUSEADDR err %d", errno);
			socketUtils ::sock_close(listenfd);
			continue;
		}
		if (::bind(listenfd, res->ai_addr, res->ai_addrlen) < 0)
		{
			socketUtils ::sock_close(listenfd); /* bind error, close and try next one */
			continue;
		}
		break; /* success */
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) /* errno from final socket() or bind() */
		err_sys("tcp_listen error for %s, %s", host, serv);

	socketUtils::SetSockRecvBufSize(listenfd, 32 * 1024);
	socketUtils::SetSockSendBufSize(listenfd, 32 * 1024);

	socketUtils::sock_listen(listenfd, LISTENQUEUEMAX);

	m_sockFdCtrl.setSockfd(listenfd);
	m_evtListener->setSockfd(listenfd);

	memcpy(&m_sockAddr.m_addr, res->ai_addr, res->ai_addrlen);
	m_sockAddr.m_socklen = res->ai_addrlen;

	freeaddrinfo(ressave);
	socketUtils::log_debug_addr(&m_sockAddr.m_addr, "<TcpServer.startListen>");
	m_evtListener->createFileEvent(LOOP_EVT_READABLE, TCPServer::OnNewTcpConnection);
}

TCPServer::~TCPServer()
{
}

WyNet *TCPServer::getNet() const
{
	return m_parent->getNet();
}

EventLoop &TCPServer::getLoop()
{
	return m_parent->getNet()->getLoop();
}

void TCPServer::acceptConnection()
{
	int listenfd = m_sockFdCtrl.sockfd();
	struct sockaddr_storage cliAddr;
	socklen_t len = sizeof(cliAddr);
	int connfdTcp = accept(listenfd, (struct sockaddr *)&cliAddr, &len);
	log_debug("acceptConnection fd:%d", connfdTcp);
	if (connfdTcp < 0)
	{
		if ((errno == EAGAIN) ||
			(errno == EWOULDBLOCK) ||
			(errno == ECONNABORTED) ||
#ifdef EPROTO
			(errno == EPROTO) ||
#endif
			(errno == EINTR))
		{
			// already closed
			return;
		}
		log_error("[TCPServer] Accept err: %d %s", errno, strerror(errno));
		return;
	}

	getLoop().assertInLoopThread();
	EventLoop *ioLoop = getNet()->getThreadPool()->getNextLoop();
	PtrConn conn;
	conn = std::make_shared<TcpConnection>();
	conn->setEventLoop(ioLoop);
	conn->setCtrl(shared_from_this());
	conn->m_sockFdCtrl.setSockfd(connfdTcp);
	conn->setCallBack_Connected(onTcpConnected);
	conn->setCallBack_Disconnected(onTcpDisconnected);
	conn->setCallBack_Message(onTcpRecvMessage);

	ioLoop->runInLoop(std::bind(&TcpConnection::onEstablished, conn));
}

}; // namespace wynet
