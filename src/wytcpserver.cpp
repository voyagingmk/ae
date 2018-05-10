#include "wytcpserver.h"
#include "wyutils.h"
#include "wyserver.h"
#include "eventloop.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"
#include "wysockbuffer.h"

namespace wynet
{
void OnTcpNewConnection(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
	std::shared_ptr<FDRef> sfdRef = fdRef.lock();
	if (!sfdRef)
	{
		return;
	}
	std::shared_ptr<TCPServer> tcpServer = std::dynamic_pointer_cast<TCPServer>(sfdRef);

	tcpServer->acceptConnection();
}

TCPServer::TCPServer(PtrServer parent) : m_parent(parent),
										 onTcpConnected(nullptr),
										 onTcpDisconnected(nullptr),
										 onTcpRecvMessage(nullptr)
{
}

void TCPServer::startListen(int port)
{
	m_tcpPort = port;
	int listenfd, n;
	const int on = 1;
	struct addrinfo hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	const char *host = NULL;
	char buf[5];
	sprintf(buf, "%d", m_tcpPort);
	const char *serv = (char *)&buf;

	if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_listen error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do
	{
		listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (listenfd < 0)
			continue; /* error, try next one */

		int flags = Fcntl(listenfd, F_GETFL, 0);
		Fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

		int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (ret < 0)
		{
			log_error("[TCPServer][tcp] setsockopt SO_REUSEADDR err %d", errno);
			Close(listenfd);
			continue;
		}
		if (::bind(listenfd, res->ai_addr, res->ai_addrlen) < 0)
		{
			Close(listenfd); /* bind error, close and try next one */
			continue;
		}
		break; /* success */
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) /* errno from final socket() or bind() */
		err_sys("tcp_listen error for %s, %s", host, serv);

	SetSockRecvBufSize(listenfd, 32 * 1024);
	SetSockSendBufSize(listenfd, 32 * 1024);

	Listen(listenfd, LISTENQ);

	setSockfd(listenfd);
	m_family = res->ai_family;
	memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
	m_socklen = res->ai_addrlen; /* return size of protocol address */

	freeaddrinfo(ressave);

	char *str = Sock_ntop((struct sockaddr *)&m_sockaddr, m_socklen);
	log_info("TCP TCPServer created: %s", str);

	getLoop().createFileEvent(shared_from_this(), LOOP_EVT_READABLE,
							  OnTcpNewConnection);
}

TCPServer::~TCPServer()
{
	getLoop().deleteFileEvent(sockfd(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
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
	int listenfd = sockfd();
	struct sockaddr_storage cliAddr;
	socklen_t len = sizeof(cliAddr);
	int connfdTcp = accept(listenfd, (SA *)&cliAddr, &len);
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
	PtrConn conn = std::make_shared<TcpConnection>(connfdTcp);
	conn->setEventLoop(ioLoop);
	conn->setCallBack_Connected(onTcpConnected);
	conn->setCallBack_Disconnected(onTcpDisconnected);
	conn->setCallBack_Message(onTcpRecvMessage);
	//if (onTcpConnected)
	//	onTcpConnected(conn);
	ioLoop->runInLoop(std::bind(&TcpConnection::onEstablished, conn));
}

}; // namespace wynet
