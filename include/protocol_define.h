#ifndef WY_PROTOCOL_DEFINE_H
#define WY_PROTOCOL_DEFINE_H
#include "common.h"
#include "buffer.h"

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

#define ProtoType(p)                    \
    inline Protocol GetProtocol() const \
    {                                   \
        return Protocol::p;             \
    }

#define ProtoSize(p)      \
    size_t Size()         \
    {                     \
        return sizeof(p); \
    }

struct ProtocolBase
{
    uint8_t *BeginPos()
    {
        return (uint8_t *)this;
    }
};

struct TcpHandshake : public ProtocolBase
{
    ProtoType(TcpHandshake);
    ProtoSize(TcpHandshake);
    uint32_t connectId;
    uint16_t udpPort;
    uint32_t key;
};

struct UdpProtocolBase : public ProtocolBase
{
    uint32_t connectId;
    uint32_t key;
};

struct UdpHandshake : public UdpProtocolBase
{
    ProtoType(UdpHandshake);
    ProtoSize(UdpHandshake);
};

struct UdpHeartbeat : public UdpProtocolBase
{
    ProtoType(UdpHeartbeat);
    ProtoSize(UdpHeartbeat);
};

struct UserPacket : public ProtocolBase
{
    ProtoType(UserPacket);
    ProtoSize(UserPacket);
};
}; // namespace protocol

template <class P>
PacketHeader *SerializeProtocol(P &p, size_t len = 0)
{
    if (!len)
    {
        len = p.Size();
    }
    PacketHeader header;
    header.setProtocol(static_cast<uint8_t>(p.GetProtocol()));
    header.setFlag(HeaderFlag::PacketLen, true);
    header.updateHeaderLength();
    size_t packetLen = header.getHeaderLength() + len;
    header.setUInt32(HeaderFlag::PacketLen, packetLen);
    BufferSet *bufferSet = BufferSet::getSingleton();
    std::shared_ptr<DynamicBuffer> buffer = bufferSet->getBufferByIdx(0);
    buffer->expand(packetLen);
    uint8_t *buf = buffer->data();
    memcpy(buf, (uint8_t *)&header, header.getHeaderLength());
    memcpy(buf + header.getHeaderLength(), p.BeginPos(), len);
    return (PacketHeader *)buf;
}
}; // namespace wynet

#endif
