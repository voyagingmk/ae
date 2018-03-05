#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "wybuffer.h"
#include "protocol.h"

namespace wynet
{
    
class SockBuffer {
public:
    BufferRef bufRef;
    size_t recvSize;
public:
    SockBuffer():
        recvSize(0)
    {
    }
    
    inline int leftSpace() {
       return bufRef->size - recvSize;
    }
    
    // Warning: must consume queued packet before call readIn
    //  0: closed
    // -1: error
    //  1: has available packet
    //  2: EAGAIN
    int readIn(int sockfd, int* nreadTotal) {
        *nreadTotal = 0;
        Buffer* p = bufRef.get();
        int npend;
        ioctl(sockfd, FIONREAD, &npend);
        
        // 1. make sure there is enough space for recv
        while (npend > leftSpace()) {
            p->expand(p->size + npend);
        }
        log_info("readIn npend %d", npend);
        // 2. read into ring buffer (slicely)
        do {
            int required = -1;
            int ret = validatePacket(&required);// just to get required bytes
            log_info("validatePacket ret %d required %d", ret, required);
            if (ret == -1) {
                // error
                return -1;
            } else if (ret == 0 && required == 0) {
                return 1;
            }
            int nread = recv(sockfd, p->buffer + recvSize, required, 0);
            log_info("readIn nread %d required %d recvSize %d", nread, required, recvSize);
            if (nread == 0) {
                // closed
                return 0;
            }
            if (nread < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 2;
                } else {
                    err_msg("[SockBuffer] sockfd %d readIn err: %d %s",
                            sockfd, errno, strerror (errno));
                    return -1;
                }
            }
            nreadTotal += nread;
            recvSize += nread;
        } while(1);
        return -1;
    }
    
    // -2: receiving
    // -1: error
    //  0: ok
    int validatePacket(int* required) {
        if (recvSize < HeaderBaseLength) {
            // receiving pakcet header base
            *required = HeaderBaseLength - recvSize;
            return -2;
        }
        Buffer* p = bufRef.get();
        uint8_t* buffer = p->buffer;
        PacketHeader* header = new(buffer)PacketHeader();
        if (recvSize < header->getHeaderLength()) {
            // receiving pakcet header options
            *required = header->getHeaderLength() - recvSize;
            return -2;
        }
        if (!header->isFlagOn(HeaderFlag::PacketLen))
        {
            log_debug("%s", header->getHeaderDebugInfo().c_str());
            // error: no packetLen flag
            return -1;
        }
        // check length
        uint32_t packetLen = header->getUInt(HeaderFlag::PacketLen);
        if (recvSize < packetLen) {
            // receiving pakcet data
            *required = packetLen - recvSize;
            return -2;
        }
        *required = 0;
        // TODO:validate whether it is a legal packet
        return 0;
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
