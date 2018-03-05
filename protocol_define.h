#ifndef WY_PROTOCOL_DEFINE_H
#define WY_PROTOCOL_DEFINE_H
#include "common.h"
#include "wybuffer.h"

namespace wynet
{

enum class Protocol
{
    Unknown = 0,
    TcpHandshake = 1,
    UdpHandshake = 2
};

namespace protocol
{
#define ProtoType(p) static const Protocol protocol = Protocol::p;

struct TcpHandshake
{
    uint32_t clientId;
    uint16_t udpPort;
    uint32_t key;
    ProtoType(TcpHandshake);
};

struct UdpHandshake
{
    uint32_t clientId;
    uint32_t key;
    ProtoType(UdpHandshake);
};
};

template <class P>
PacketHeader *SerializeProtocol(P content)
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
    memcpy(buf + header.getHeaderLength(),
           (uint8_t *)&content, sizeof(P));
    return (PacketHeader *)buf;
}
};

#endif
