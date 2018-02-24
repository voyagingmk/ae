#ifndef WY_PROTOCOL_DEFINE_H
#define WY_PROTOCOL_DEFINE_H
#include "common.h"
#include "wybuffer.h"

namespace wynet
{

namespace protocol
{

struct Handshake
{
    uint32_t clientID;
    uint16_t udpPort;
    static const Protocol protocol = Protocol::Handshake;
};
};

template<class P>
PacketHeader* SerializeProtocol(P content)
{
    PacketHeader header;
    header.setProtocol(P::protocol);
    header.setFlag(HeaderFlag::PacketLen, true);
    header.updateHeaderLength();
    size_t packetLen = header.getHeaderLength() + sizeof(P);
    header.setUInt32(HeaderFlag::PacketLen, packetLen);
    char* buf = (char*)gBufferSet.getBuffer(0, packetLen);
    memcpy(buf, (uint8_t *)&header, header.getHeaderLength());
    memcpy(buf + header.getHeaderLength(),
           (uint8_t *)&content, sizeof(P));
    return (PacketHeader*)buf;
}

    
};

#endif
