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
	m_convIdGen.setRecycleThreshold(2 << 15);
	m_convIdGen.setRecycleEnabled(true);
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

	LogSocketState(sockfd());
}

TCPServer::~TCPServer()
{
	getLoop().deleteFileEvent(sockfd(), LOOP_EVT_READABLE | LOOP_EVT_WRITABLE);
}

void TCPServer::closeConnect(UniqID connectId)
{
	auto it = m_connDict.find(connectId);
	if (it == m_connDict.end())
	{
		return;
	}
	_closeConnectByFd(it->second->fd(), true);
}

void TCPServer::_closeConnectByFd(int connfdTcp, bool force)
{
	if (force)
	{
		struct linger l;
		l.l_onoff = 1; /* cause RST to be sent on close() */
		l.l_linger = 0;
		Setsockopt(connfdTcp, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
	}
	_onTcpDisconnected(connfdTcp);
}

void TCPServer::sendByTcp(UniqID connectId, const uint8_t *m_data, size_t len)
{
	protocol::UserPacket *p = (protocol::UserPacket *)m_data;
	PacketHeader *header = SerializeProtocol<protocol::UserPacket>(*p, len);
	sendByTcp(connectId, header);
}

void TCPServer::sendByTcp(UniqID connectId, PacketHeader *header)
{
	assert(header != nullptr);
	auto it = m_connDict.find(connectId);
	if (it == m_connDict.end())
	{
		return;
	}
	PtrSerConn conn = it->second;
	int ret = send(conn->fd(), (uint8_t *)header, header->getUInt32(HeaderFlag::PacketLen), 0);
	if (ret < 0)
	{
		// should never EMSGSIZE ENOBUFS
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
		{
			return;
		}
		// close the client
		log_error("[TCPServer][tcp] sendByTcp err %d", errno);
		_closeConnectByFd(conn->fd(), true);
	}
}
WyNet *TCPServer::getNet() const
{
	return m_parent->getNet();
}

EventLoop &TCPServer::getLoop()
{
	return m_parent->getNet()->getLoop();
}

UniqID TCPServer::refConnection(PtrSerConn conn)
{
	UniqID connectId = m_connectIdGen.getNewID();
	UniqID convId = m_convIdGen.getNewID();
	m_connDict[connectId] = conn;
	conn->setConnectId(connectId);
	uint16_t password = random();
	conn->setKey((password << 16) | convId);
	m_connfd2cid[conn->connectFd()] = connectId;
	m_convId2cid[convId] = connectId;
	return connectId;
}

bool TCPServer::unrefConnection(UniqID connectId)
{
	if (m_connDict.find(connectId) == m_connDict.end())
	{
		return false;
	}
	PtrSerConn conn = m_connDict[connectId];
	m_connfd2cid.erase(conn->connectFd());
	m_convId2cid.erase(conn->convId());
	m_connDict.erase(connectId);
	return true;
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
	PtrSerConn conn = std::make_shared<SerConn>(shared_from_this(), connfdTcp);
	conn->setEventLoop(ioLoop);
	refConnection(conn);
	conn->setCallBack_Connected(onTcpConnected);
	conn->setCallBack_Disconnected(onTcpDisconnected);
	conn->setCallBack_Message(onTcpRecvMessage);
	//if (onTcpConnected)
	//	onTcpConnected(conn);
	ioLoop->runInLoop(std::bind(&SerConn::onEstablished, conn));
}

void TCPServer::_onTcpDisconnected(int connfdTcp)
{
	if (m_connfd2cid.find(connfdTcp) != m_connfd2cid.end())
	{
		UniqID connectId = m_connfd2cid[connfdTcp];
		PtrSerConn conn = m_connDict[connectId];
		unrefConnection(connectId);
		log_info("[TCPServer][tcp] closed, connectId: %d connfdTcp: %d", connectId, connfdTcp);
		if (onTcpDisconnected)
			onTcpDisconnected(conn);
	}
}

/*
void TCPServer::_onTcpMessage(int connfdTcp)
{
	UniqID connectId = m_connfd2cid[connfdTcp];
	PtrSerConn conn = m_connDict[connectId];
	SockBuffer &sockBuffer = conn->sockBuffer();
	do
	{
		int nreadTotal = 0;
		int ret = sockBuffer.readIn(connfdTcp, &nreadTotal);
		log_debug("[TCPServer][tcp] readIn connfdTcp %d ret %d nreadTotal %d", connfdTcp, ret, nreadTotal);
		if (ret <= 0)
		{
			// has error or has closed
			_closeConnectByFd(connfdTcp, true);
			return;
		}
		if (ret == 2)
		{
			break;
		}
		if (ret == 1)
		{
			BufferRef &bufRef = sockBuffer.getBufRef();
			PacketHeader *header = (PacketHeader *)(bufRef->data());
			Protocol protocol = static_cast<Protocol>(header->getProtocol());
			switch (protocol)
			{
			case Protocol::UdpHandshake:
			{
				break;
			}
			case Protocol::UserPacket:
			{
				protocol::UserPacket *p = (protocol::UserPacket *)(bufRef->data() + header->getHeaderLength());
				size_t dataLength = header->getUInt32(HeaderFlag::PacketLen) - header->getHeaderLength();
				// log_debug("getHeaderLength: %d", header->getHeaderLength());
				if (onTcpRecvMessage)
				{
					onTcpRecvMessage(conn, (uint8_t *)p, dataLength);
				}
				break;
			}
			default:
				break;
			}
			sockBuffer.resetBuffer();
		}
	} while (1);
}*/
};
