#ifndef WY_BUFFER_H
#define WY_BUFFER_H

#include "common.h"
#include "uniqid.h"

namespace wynet
{

const size_t MaxBufferSize = 1024 * 1024 * 256; // 256 MB

class Buffer
{
    uint8_t *buffer;
    size_t size;

  public:
    Buffer()
    {
        size = 0xff;
        buffer = new uint8_t[size]{0};
    }
    ~Buffer()
    {
        size = 0;
        delete []buffer;
    }

    uint8_t *reserve(size_t n)
    {
        if (n > MaxBufferSize)
        {
            return nullptr;
        }
        if (!buffer || n > size)
        {
            while (n > size)
            {
                size = size << 1;
            }
            delete[] buffer;
            buffer = new uint8_t[size]{0};
        }
        return buffer;
    }
};

class BufferSet
{
    std::vector<Buffer> buffers;
    UniqIDGenerator uniqIDGen;
    
  public:
    
    static BufferSet& singleton() {
        static BufferSet gBufferSet;
        return gBufferSet;
    }
    
    BufferSet(int defaultCapacity = 8)
    {
        buffers.reserve(defaultCapacity);
    }
    
    UniqID newBuffer() {
        UniqID uid = uniqIDGen.getNewID();
        if (uid > buffers.capacity()) {
            buffers.reserve(buffers.capacity() << 1);
        }
        return uid;
    }
    
    void recycleBuffer(UniqID uid) {
        uniqIDGen.recycleID(uid);
    }
    
    Buffer* getBuffer(UniqID uid) {
        uint32_t idx = uid - 1;
        if(idx >= buffers.size()) {
            return nullptr;
        }
        return &buffers[idx];
    }
};

};

#endif
