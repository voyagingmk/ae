#ifndef WY_PROTOCOL_DEFINE_H
#define WY_PROTOCOL_DEFINE_H
#include "common.h"
#include "wybuffer.h"

namespace wynet
{

enum class Protocol
{
    Unknown,
    TcpHandshake,
    UdpHandshake,
    UdpHeartbeat,
    UserPacket,
};

namespace protocol
{
    
#define ProtoType(p) static const Protocol protocol = Protocol::p;
#define ProtoSize(p) size_t Size() { \
    return sizeof(p); \
}

    
    
struct ProtocolBase {
    uint8_t* BeginPos() {
        return (uint8_t*)this;
    }
};
    
struct TcpHandshake: public ProtocolBase
{
    ProtoType(TcpHandshake);
    ProtoSize(TcpHandshake);
    uint32_t clientId;
    uint16_t udpPort;
    uint32_t key;

};
    
struct UdpProtocolBase: public ProtocolBase
{
    uint32_t clientId;
    uint32_t key;
};
    
struct UdpHandshake: public UdpProtocolBase
{
    ProtoType(UdpHandshake);
    ProtoSize(UdpHandshake);
};
    
struct UdpHeartbeat: public UdpProtocolBase
{
    ProtoType(UdpHeartbeat);
    ProtoSize(UdpHeartbeat);
};
    
struct UserPacket: public ProtocolBase
{
    ProtoType(UserPacket);
    UserPacket(const uint8_t *d, uint32_t l):
        len(l),
        data(d)
    {}
    uint32_t len;
    const uint8_t *data;
    
    const uint8_t* BeginPos() {
        return data;
    }
    
    size_t Size() {
        return len;
    }
};

};

template <class P>
PacketHeader *SerializeProtocol(P p)
{
    PacketHeader header;
    header.setProtocol(static_cast<uint8_t>(P::protocol));
    header.setFlag(HeaderFlag::PacketLen, true);
    header.updateHeaderLength();
    size_t packetLen = header.getHeaderLength() + sizeof(P);
    header.setUInt32(HeaderFlag::PacketLen, packetLen);
    BufferSet &bufferSet = BufferSet::constSingleton();
    Buffer *buffer = bufferSet.getBufferByIdx(0);
    uint8_t *buf = buffer->reserve(packetLen);
    memcpy(buf, (uint8_t *)&header, header.getHeaderLength());
    memcpy(buf + header.getHeaderLength(), p.BeginPos(), p.Size());
    return (PacketHeader *)buf;
}
};

#endif
