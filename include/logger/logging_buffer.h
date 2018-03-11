#ifndef WY_LOGGING_BUFFER_H
#define WY_LOGGING_BUFFER_H

#include <cstddef>
#include "../wybuffer.h"

namespace wynet
{

class LoggingBuffer : public StaticBuffer
{
  public:
    LoggingBuffer():
        StaticBuffer<4 * 1024 * 1024>(),
        used(0)
    {
        setCookie(cookieStart);
    }

    ~LoggingBuffer()
    {
        setCookie(cookieEnd);
    }

    void append(const char *buf, size_t len)
    {
        if (implicit_cast<size_t>(leftOpacity()) > len)
        {
            memcpy(m_data + used, buf, len);
            used += len;
        }
    }

    const char *data() const { 
        return m_data; 
    }

    int length() const { 
        return used;
    }

    char *current() {
         return m_data + used;
    }

    size_t leftOpacity() const { 
        m_size - used; 
    }

    void add(size_t len) { 
        used += len; 
    }

    void reset() { 
        used = 0; 
    }

    const char *debugString();

    void setCookie(void (*cookie)()) { 
        m_cookie = cookie; 
    }

    string toString() const { 
        return string(m_data, length()); 
    }

  private:
    const char *end() const { 
        return m_data + m_size; 
    }

    static void cookieStart();

    static void cookieEnd();

    void (*m_cookie)();
    size_t used;
};
};

#endif
