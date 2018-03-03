#ifndef WY_BUFFER_H
#define WY_BUFFER_H

#include "common.h"
#include "uniqid.h"

namespace wynet
{

const size_t MaxBufferSize = 1024 * 1024 * 256; // 256 MB

class Buffer
{
  public:
    
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
    
    static BufferSet& dynamicSingleton() {
        static BufferSet gBufferSet;
        return gBufferSet;
    }
    
    static BufferSet& constSingleton() {
        static BufferSet gBufferSet;
        return gBufferSet;
    }
    
    BufferSet(int defaultSize = 8)
    {
        buffers.resize(defaultSize);
    }
    
    UniqID newBuffer() {
        UniqID uid = uniqIDGen.getNewID();
        if (uid > buffers.size()) {
            buffers.resize(buffers.size() << 1);
        }
        return uid;
    }
    
    void recycleBuffer(UniqID uid) {
        uniqIDGen.recycleID(uid);
    }
    
    Buffer* getBuffer(UniqID uid) {
        int32_t idx = uid - 1;
        if(idx < 0 || idx >= buffers.size()) {
            return nullptr;
        }
        return &buffers[idx];
    }
    
    Buffer* getBufferByIdx(int32_t idx) {
        if (idx < 0) {
            return nullptr;
        }
        while((idx+1) > buffers.size()) {
            buffers.resize(buffers.size() << 1);
        }
        return &buffers[idx];
    }
};

class BufferRef {
    UniqID uniqID;
    
public:
    
    BufferRef() {
       uniqID = BufferSet::dynamicSingleton().newBuffer();
    }
    
    ~BufferRef() {
        BufferSet::dynamicSingleton().recycleBuffer(uniqID);
    }
    
    BufferRef(BufferRef&& b) {
        uniqID = b.uniqID;
        b.uniqID = 0;
    }
    
    BufferRef & operator = (BufferRef&& b) {
        uniqID = b.uniqID;
        b.uniqID = 0;
        return (*this);
    }
    
    
    BufferRef(const BufferRef&) = delete;
    
    BufferRef & operator = (const BufferRef&) = delete;
    
    Buffer* get() {
        if(!uniqID) {
            return nullptr;
        }
        return BufferSet::dynamicSingleton().getBuffer(uniqID);
    }
};
    
};

#endif
