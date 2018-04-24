#ifndef WY_SOCKET_BUFFER_H
#define WY_SOCKET_BUFFER_H

#include "common.h"
#include "noncopyable.h"
#include "wybuffer.h"

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

    inline int writableSize()
    {
        return m_bufRef->length() - m_pos;
    }

    inline int readableSize()
    {
        return m_pos;
    }

    inline void resetBuffer()
    {
        m_pos = 0;
    }

    int readIn(int sockfd, int *nreadTotal);
};
};

#endif