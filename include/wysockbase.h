#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "wybuffer.h"
#include "protocol.h"
#include "noncopyable.h"
#include "multiple_inherit.h"

namespace wynet
{

class SockBuffer : public Noncopyable
{
  public:
    BufferRef bufRef;
    size_t recvSize;

  public:
    SockBuffer() : recvSize(0)
    {
    }
    SockBuffer &operator=(SockBuffer &&s)
    {
        bufRef = std::move(s.bufRef);
        recvSize = s.recvSize;
        s.recvSize = 0;
        return *this;
    }

    SockBuffer(SockBuffer &&s)
    {
        bufRef = std::move(s.bufRef);
        recvSize = s.recvSize;
        s.recvSize = 0;
    }

    inline int leftSpace()
    {
        return bufRef->m_size - recvSize;
    }

    inline void resetBuffer()
    {
        recvSize = 0;
    }

    // Warning: must consume queued packet before call readIn
    //  0: closed
    // -1: error
    //  1: has available packet
    //  2: EAGAIN
    int readIn(int sockfd, int *nreadTotal)
    {
        *nreadTotal = 0;
        do
        {
            int required = -1;
            int ret = validatePacket(&required); // just to get required bytes
            log_debug("validatePacket ret %d required %d", ret, required);
            if (ret == -1)
            {
                // error
                return -1;
            }
            else if (ret == 0 && required == 0)
            {
                return 1;
            }
            int npend;
            ioctl(sockfd, FIONREAD, &npend);
            // make sure there is enough space for recv
            while (npend > leftSpace())
            {
                bufRef->expand(bufRef->m_size + npend);
            }
            log_debug("readIn npend %d", npend);
            int nread = recv(sockfd, bufRef->m_data + recvSize, required, 0);
            log_debug("recv nread %d required %d recvSize %d", nread, required, recvSize);
            if (nread == 0)
            {
                // closed
                return 0;
            }
            if (nread < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return 2;
                }
                else
                {
                    err_msg("[SockBuffer] sockfd %d readIn err: %d %s",
                            sockfd, errno, strerror(errno));
                    return -1;
                }
            }
            nreadTotal += nread;
            recvSize += nread;
        } while (1);
        return -1;
    }

    // -2: receiving
    // -1: error
    //  0: ok
    int validatePacket(int *required)
    {
        if (recvSize < HeaderBaseLength)
        {
            // receiving pakcet header base
            *required = HeaderBaseLength - recvSize;
            return -2;
        }
        uint8_t *pData = bufRef->m_data;

        for (int i = 0; i < recvSize; i++)
        {
            //    printf("--%hhu\n", *(pData + i));
        }
        PacketHeader *header = (PacketHeader *)(pData);
        for (int i = 0; i < recvSize; i++)
        {
            //    printf("==%hhu\n", *(pData + i));
        }
        if (recvSize < header->getHeaderLength())
        {
            // receiving pakcet header options
            *required = header->getHeaderLength() - recvSize;
            return -2;
        }
        if (!header->isFlagOn(HeaderFlag::PacketLen))
        {
            // log_error("header->isFlagOn PacketLen false, flag: %u", header->flag);
            // error: no packetLen flag
            return -1;
        }
        // check length
        uint32_t packetLen = header->getUInt(HeaderFlag::PacketLen);
        if (recvSize < packetLen)
        {
            // receiving pakcet pData
            *required = packetLen - recvSize;
            return -2;
        }
        *required = 0;
        // TODO:validate whether it is a legal packet
        return 0;
    }
};

class FDRef : public inheritable_enable_shared_from_this<FDRef>
{
  public:
    FDRef(int fd)
    {
        setfd(fd);
    }
    virtual ~FDRef() {}

    inline int fd() const { return m_fd; }

  protected:
    inline void setfd(int fd) { m_fd = fd; }

  private:
    int m_fd;
};

class SocketBase : public FDRef
{
  protected:
    SocketBase() : FDRef(0),
                   m_socklen(0),
                   m_family(0)
    {
    }

  public:
    inline void setSockfd(int fd) { setfd(fd); }
    inline int sockfd() const { return fd(); }
    inline bool valid() const { return fd() > 0; }
    inline bool isIPv4() const { return m_family == PF_INET; }
    inline bool isIPv6() const { return m_family == PF_INET6; }

  public:
    sockaddr_in6 m_sockaddr;
    socklen_t m_socklen;
    int m_family;
};
};

#endif
