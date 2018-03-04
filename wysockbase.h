#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "wybuffer.h"

namespace wynet
{
    
class SockBuffer {
    BufferRef bufRef;
    size_t len;
public:
    SockBuffer() {
        len = 0;
    }
    int readIn(int sockfd) {
        Buffer* buf = bufRef.get();
        int space = buf->size - len;
        const size_t RecvSize = 1400;
        if (space <= 0) {
            buf->expand(buf->size + RecvSize);
        }
        int nread = recv(sockfd, buf->buffer + len, RecvSize, 0);
        if (nread > 0) {
            len += nread;
        }
        if (nread < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
            } else {
                err_msg("[SockBuffer] sockfd %d readIn err: %d %s",
                        sockfd, errno, strerror (errno));
            }
        }
        // nread = 0 : remote peer closed
        return nread;
    }
};

class SocketBase
{
protected:
  SocketBase() {}

public:
  sockaddr_in6 m_sockaddr;
  socklen_t m_socklen;
  int m_sockfd;
  int m_family;
  SockBuffer buf;
  bool isIPv4() { return m_family == PF_INET; }
  bool isIPv6() { return m_family == PF_INET6; }
};
};

#endif
