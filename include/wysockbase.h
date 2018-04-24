#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "logger/log.h"
#include "wybuffer.h"
#include "protocol.h"
#include "noncopyable.h"
#include "multiple_inherit.h"

namespace wynet
{

/*
    一个连接只在一开始有large级别的数据，之后就都是small数据或者idle了
    怎么利用这些空闲的内存：
    1.超时检查，resize小的，并把剩余copy，释放掉内存，还给系统
    2.
*/

// 支持读/写的buffer
class SockBuffer : public Noncopyable
{
  public:
    BufferRef m_bufRef;
    size_t m_pos;

  public:
    SockBuffer() : m_pos(0)
    {
    }
    SockBuffer &operator=(SockBuffer &&s)
    {
        m_bufRef = std::move(s.m_bufRef);
        m_pos = s.m_pos;
        s.m_pos = 0;
        return *this;
    }

    SockBuffer(SockBuffer &&s)
    {
        m_bufRef = std::move(s.m_bufRef);
        m_pos = s.m_pos;
        s.m_pos = 0;
    }

    BufferRef &getBufRef()
    {
        return m_bufRef;
    }

    inline int leftSpace()
    {
        return m_bufRef->length() - m_pos;
    }

    inline void resetBuffer()
    {
        m_pos = 0;
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
            /*
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
                m_bufRef->expand(m_bufRef->length() + npend);
            }
            log_debug("readIn npend %d", npend);
            int nread = recv(sockfd, m_bufRef->data() + m_pos, required, 0);
            log_debug("recv nread %d required %d m_pos %d", nread, required, m_pos);
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
            }*/
            // nreadTotal += nread;
            // m_pos += nread;
        } while (1);
        return -1;
    }

    // -2: receiving
    // -1: error
    //  0: ok
    int validatePacket(int *required)
    {
        if (m_pos < HeaderBaseLength)
        {
            // receiving pakcet header base
            *required = HeaderBaseLength - m_pos;
            return -2;
        }
        const uint8_t *pData = m_bufRef->data();

        for (int i = 0; i < m_pos; i++)
        {
            //    printf("--%hhu\n", *(pData + i));
        }
        PacketHeader *header = (PacketHeader *)(pData);
        for (int i = 0; i < m_pos; i++)
        {
            //    printf("==%hhu\n", *(pData + i));
        }
        if (m_pos < header->getHeaderLength())
        {
            // receiving pakcet header options
            *required = header->getHeaderLength() - m_pos;
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
        if (m_pos < packetLen)
        {
            // receiving pakcet pData
            *required = packetLen - m_pos;
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
    SocketBase(int fd = 0) : FDRef(fd),
                             m_socklen(0),
                             m_family(0)
    {
    }
    virtual ~SocketBase()
    {
        ::close(sockfd());
        setfd(0);
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
