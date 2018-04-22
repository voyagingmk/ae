#ifndef WY_LOGGING_BUFFER_H
#define WY_LOGGING_BUFFER_H

#include <cstddef>
#include "wybuffer.h"

namespace wynet
{

class LoggingBuffer : public StaticBuffer<4 * 1024 * 1024>
{
  public:
    LoggingBuffer() : used(0)
    {
    }

    ~LoggingBuffer()
    {
    }

    void append(const char *buf, size_t len)
    {
        if (leftOpacity() > len)
        {
            memcpy(data() + used, buf, len);
            used += len;
        }
    }

    char *current()
    {
        return (char *)data() + used;
    }

    size_t leftOpacity() const
    {
        return length() - used;
    }

    void add(size_t len)
    {
        used += len;
    }

    size_t usedLength() const
    {
        return used;
    }

    void clean()
    {
        used = 0;
    }

    const char *debugString();

    std::string toString() const
    {
        return std::string((const char *)data(), usedLength());
    }

  private:
    const char *end() const
    {
        return (const char *)data() + usedLength();
    }

    size_t used;
};
};

#endif
