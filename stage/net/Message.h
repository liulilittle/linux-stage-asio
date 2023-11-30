#pragma once 

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"

#include <stdio.h>
#include <atomic>
#include <sstream>
#include <strstream>

namespace ep
{
    namespace net
    {
#pragma pack(push, 1)
        struct socket_packet_hdrc
        {
        public:
            char                                                                                            fk;
            union 
            {
                unsigned int                                                                                len;
                struct
                {
                    unsigned short                                                                          plsz; // 对外客户端
                    unsigned short                                                                          mask; // 对外客户端
                };
            };
            struct
            {
                unsigned short                                                                              cmd : 15;
                unsigned char                                                                               compressed : 1; 
            };
            unsigned int                                                                                    seq;

        public:
            socket_packet_hdrc()
            {
                Clear();
            }

        public:
            inline socket_packet_hdrc*                                                                      get() { return this; }
            inline void                                                                                     Clear()
            {
                fk = 0;
                len = 0;
                plsz = 0;
                mask = 0;
                cmd = 0;
                compressed = false;
                seq = 0;
            }
        };
#pragma pack(pop)

        class Message : public EventArgs
        {
        public:
            Message();
            Message(const std::shared_ptr<BufferSegment>& payload);
            Message(unsigned short commandId, unsigned int sequenceNo, const std::shared_ptr<BufferSegment>& payload);
            virtual ~Message();

        public:
            unsigned short                                                                                  CommandId;          // 命令编号
            unsigned int                                                                                    SequenceNo;         // 序列编号
            std::shared_ptr<BufferSegment>                                                                  Payload;            // 载荷数据

        public:
            inline Message*                                                                                 GetPtr() { return this; }
            static unsigned int                                                                             NewId();
            static std::shared_ptr<Message>                                                                 Create(
                unsigned short                                                                              commandId,
                unsigned int                                                                                sequenceNo,
                const std::shared_ptr<BufferSegment>&                                                       payload);
            void																		                    Reset(const std::shared_ptr<BufferSegment>& payload);

        public:
            template<typename TMessage, typename TPayloadObject>
            inline static bool                                                                              New(TMessage& packet, UShort commandId, Int64 sequenceNo, TPayloadObject& message)
            {
                if (!Object::addressof(packet))
                {
                    return false;
                }

                if (!Object::addressof(message))
                {
                    return false;
                }

                std::strstream messagestream;
                int messagesize = Serialize(message, messagestream);

                packet.CommandId = commandId;
                packet.SequenceNo = sequenceNo;
                packet.Payload = make_shared_object<BufferSegment>();
                if (!packet.Payload)
                {
                    return false;
                }

                if (messagesize > 0)
                {
                    std::shared_ptr<unsigned char> payload_data = make_shared_array<unsigned char>(messagesize);
                    if (!payload_data)
                    {
                        return false;
                    }

                    unsigned char* psz = (unsigned char*)messagestream.str();
                    if (!psz)
                    {
                        return false;
                    }
                    else
                    {
                        memcpy(payload_data.get(), psz, messagesize);
                    }

                    packet.Payload->Reset(payload_data, 0, messagesize);
                }

                return true;
            }
            template<typename TMessage>
            inline static std::shared_ptr<TMessage>										                    New(UShort commandId, Int64 sequenceNo, const void* buffer = NULL, int length = 0)
            {
                if (NULL == buffer && 0 != length)
                {
                    return Object::Nulll<TMessage>();
                }

                if (length < 0)
                {
                    return Object::Nulll<TMessage>();
                }

                if (NULL == buffer)
                {
                    buffer = (Byte*)("");
                }

                std::shared_ptr<unsigned char> payload_data = make_shared_array<unsigned char>(length);
                std::shared_ptr<TMessage> message_data = make_shared_object<TMessage>(commandId,
                    sequenceNo,
                    make_shared_object<BufferSegment>(payload_data, 0, length));
                if (length > 0)
                {
                    memcpy(Object::addressof(payload_data), buffer, (size_t)length);
                }

                return message_data;
            }
            template<typename TMessage>
            inline static int                                                                               Serialize(const std::shared_ptr<TMessage>& message, std::strstream& stream)
            {
                if (Object::IsNulll(message))
                {
                    return 0;
                }

                TMessage& m = *message.get();
                int serialize_bytes = Serialize<TMessage>(m, stream);
                return serialize_bytes;
            }
            template<typename TMessage>
            inline static int															                    Serialize(const TMessage& message, std::strstream& stream)
            {
                TMessage& m = Object::constcast(message);
                int before_pos = (int)stream.tellp();
                {
                    m.Serialize(stream);
                }
                int current_pos = (int)stream.tellp();
                int serialize_bytes = (current_pos - before_pos);
#ifdef WIN32
                serialize_bytes--;
#endif
                if (serialize_bytes < 0)
                {
                    serialize_bytes = 0;
                }
                return serialize_bytes;
            }

        public:
            template<typename TMessage>
            inline static std::shared_ptr<TMessage>										                    Deserialize(const Byte* buffer, int length)
            {
                unsigned char* stream_ptr = (unsigned char*)buffer;
                int stream_length = length;
                if (NULL == stream_ptr || stream_length <= 0)
                {
                    return Object::Nulll<TMessage>();
                }

                std::shared_ptr<TMessage> message_ptr = make_shared_object<TMessage>();
                if (!message_ptr->Deserialize(stream_ptr, stream_length))
                {
                    return Object::Nulll<TMessage>();
                }

                return message_ptr;
            }
            template<typename TMessage>
            inline static std::shared_ptr<TMessage>										                    Deserialize(const BufferSegment* buffer)
            {
                if (NULL == buffer)
                {
                    return Object::Nulll<TMessage>();
                }

                BufferSegment* segment = Object::constcast(buffer);
                return Deserialize<TMessage>(segment->UnsafeAddrOfPinnedArray(), segment->Length);
            }
            template<typename TMessage>
            inline static std::shared_ptr<TMessage>										                    Deserialize(const Message* message)
            {
                if (NULL == message)
                {
                    return Object::Nulll<TMessage>();
                }

                return Deserialize<TMessage>(Object::addressof(message->Payload));
            }

        private:
            static std::atomic<unsigned int>                                                                _sseqnoc;
        };

        template<typename TMember>
        class TBulkManyMessage : public Message
        {
        public:
            unsigned char                                                                                   Kf;
            Byte                                                                                            MemberType;
            TMember*                                                                                        Member;
            unsigned short                                                                                  MemberCount;
            unsigned char*                                                                                  PayloadPtr;
            unsigned int                                                                                    PayloadSize;

        public:
            inline void                                                                                     Clear()
            {
                if (this)
                {
                    Kf = (0);
                    CommandId = (0);
                    SequenceNo = (0);
                    MemberType = (0);
                    Member = (NULL);
                    MemberCount = (0);
                    PayloadPtr = (NULL);
                    PayloadSize = (0);
                    Payload.reset();
                }
            }
            inline std::shared_ptr<unsigned char>                                                           ToPacket(unsigned int& size)
            {
                if (!this)
                {
                    return make_shared_null<unsigned char>();
                }
                return ToPacket(Kf, CommandId, SequenceNo, MemberType, Member, MemberCount, PayloadPtr, PayloadSize, size);
            }
            inline bool                                                                                     LoadFrom(const std::shared_ptr<BufferSegment>& payload)
            {
                if (!this)
                {
                    return false;
                }

                if (!payload)
                {
                    return false;
                }

                unsigned char* psz = payload->UnsafeAddrOfPinnedArray();
                return LoadFrom(psz, payload->Length);
            }
            inline bool                                                                                     LoadFrom(const Message& message)
            {
                if (!this)
                {
                    return false;
                }

                if (!LoadFrom(message.Payload))
                {
                    return false;
                }

                CommandId = message.CommandId;
                SequenceNo = message.SequenceNo;
                return true;
            }
            inline bool                                                                                     LoadFrom(unsigned char* sz, unsigned int counts)
            {
                if (!this)
                {
                    return false;
                }

                Clear();

                if (!sz || counts <= 0)
                {
                    return false;
                }

                unsigned char* el = sz + counts;
                unsigned char* il = sz;

                Byte* memberType = il++;
                if (il > el)
                {
                    return false;
                }

                UShort* memberCount = (UShort*)il;
                TMember* memberValue = (TMember*)(memberCount + 1);

                il = (unsigned char*)memberValue;
                if (il > el)
                {
                    return false;
                }

                if (*memberCount == 0)
                {
                    return true;
                }

                il = (unsigned char*)(memberValue + *memberCount);
                if (il > el)
                {
                    return false;
                }

                MemberType = *memberType;
                Member = memberValue;
                MemberCount = *memberCount;
                PayloadPtr = il;
                PayloadSize = el - il;
                return true;
            }

        public:
            inline static std::shared_ptr<unsigned char>                                                    ToPacket(
                unsigned char                                                                               fk,
                unsigned short                                                                              cmd_id,
                unsigned int                                                                                ack_no,
                Byte                                                                                        member_type,
                TMember*                                                                                    member_ptr,
                unsigned short                                                                              member_len,
                unsigned char*                                                                              payload,
                unsigned int                                                                                payload_size,
                unsigned int&                                                                               packets_size)
            {
                packets_size = 0;
                if (NULL == member_ptr && member_len != 0) {
                    return make_shared_null<unsigned char>();
                }

                if (NULL == payload && payload_size != 0) { // 变长协议头
                    return make_shared_null<unsigned char>();
                }

                unsigned int packet_size = sizeof(socket_packet_hdrc);
                packet_size += sizeof(Byte);
                packet_size += sizeof(UShort);
                packet_size += sizeof(TMember) * member_len;
                packet_size += payload_size;

                std::shared_ptr<unsigned char> packet = make_shared_array<unsigned char>(packet_size + 1);
                if (!packet) {
                    return make_shared_null<unsigned char>();
                }

                socket_packet_hdrc* hdr = (socket_packet_hdrc*)packet.get();
                hdr->fk = fk;
                hdr->compressed = false;
                hdr->mask = 0;
                hdr->plsz = 0;
                hdr->cmd = cmd_id;
                hdr->seq = ack_no;
                hdr->len = packet_size - sizeof(socket_packet_hdrc);

                Byte* member_xtype = (Byte*)(hdr + 1);
                *member_xtype = member_type;

                UShort* members_size = (UShort*)(member_xtype + 1);
                *members_size = member_len;

                TMember* members_data = (TMember*)(members_size + 1);
                for (int i = 0; i < member_len; i++) {
                    members_data[i] = member_ptr[i];
                }

                unsigned char* payload_chunk = (unsigned char*)(members_data + member_len);
                memcpy(payload_chunk, payload, payload_size);

                packets_size = packet_size;
                return packet;
            }
        };

        class BulkManyMessage : public TBulkManyMessage<unsigned short> {};

        class Transitroute
        {
        public:
            UShort                                  CommandId;
            UInt32                                  SequenceNo;
            Byte                                    LinkType;
            UShort                                  ServerNo;
            std::string                             Platform;
            std::shared_ptr<BufferSegment>          Payload;

        private:
#pragma pack(push, 1)
            struct TransitrouteHeader
            {
                UShort                              CommandId;
                UInt32                              SequenceNo;
                Byte                                LinkType;
                UShort                              ServerNo;
                Byte                                PlatformLength;
            };
#pragma pack(pop)

        public:
            inline Transitroute()
            {
                Clear();
            }
            inline ~Transitroute()
            {
                Clear();
            }
            inline void                             Clear()
            {
                ServerNo = 0;
                SequenceNo = 0;
                CommandId = 0;
                LinkType = 0;
                Platform = "";
                Payload = make_shared_null<BufferSegment>();
            }
            inline bool                             Load(Message* message)
            {
                if (NULL == message)
                {
                    return false;
                }

                return Load(message->Payload.get());
            }
            inline bool                             Load(BufferSegment* segment)
            {
                if (NULL == segment)
                {
                    return false;
                }

                return Load(segment->Buffer, segment->Offset, segment->Length);
            }
            inline bool                             Load(std::shared_ptr<unsigned char>& packet, int offset, int length)
            {
                if (NULL == this)
                {
                    return false;
                }

                if (offset < 0)
                {
                    return false;
                }

                if (length < 0)
                {
                    return false;
                }

                if (!packet)
                {
                    return false;
                }

                unsigned char* el = NULL;
                unsigned char* fz = NULL;
                unsigned char* sz = packet.get();
                if (sz)
                {
                    sz += offset;
                    fz = sz;
                    el = sz + length;
                }

                length -= sizeof(TransitrouteHeader);
                if (length < 0)
                {
                    return false;
                }

                TransitrouteHeader* hdr = (TransitrouteHeader*)sz;
                this->ServerNo = hdr->ServerNo;
                this->SequenceNo = hdr->SequenceNo;
                this->CommandId = hdr->CommandId;
                this->LinkType = hdr->LinkType;
                this->Platform = "";

                sz += sizeof(TransitrouteHeader);
                if (0 != hdr->PlatformLength)
                {
                    length -= hdr->PlatformLength;
                    if (length < 0)
                    {
                        return false;
                    }
                    else
                    {
                        this->Platform = std::string((char*)sz, hdr->PlatformLength);
                    }
                    sz += hdr->PlatformLength;
                }

                int plszbytes = el - sz;
                if (plszbytes < 0)
                {
                    return false;
                }

                int hdrlen = sz - fz;
                if (hdrlen < 0)
                {
                    return false;
                }

                std::shared_ptr<BufferSegment> payload = make_shared_object<BufferSegment>(packet, offset + hdrlen, plszbytes);
                if (!payload)
                {
                    return false;
                }
                else
                {
                    this->Payload = payload;
                }
                return true;
            }
            inline int                              SizeOf()
            {
                if (NULL == this)
                {
                    return ~0;
                }

                int plsz = (int)this->Platform.size();
                if (plsz != (int)this->Platform.size())
                {
                    return ~0;
                }

                int length = sizeof(TransitrouteHeader);
                length += plsz;

                std::shared_ptr<BufferSegment> payload = this->Payload;
                if (payload)
                {
                    if (payload->Length < 0 || payload->Offset < 0)
                    {
                        return ~0;
                    }

                    std::shared_ptr<unsigned char> buffer = payload->Buffer;
                    if (!buffer)
                    {
                        if (payload->Length != 0 || payload->Offset != 0)
                        {
                            return ~0;
                        }
                    }

                    length += payload->Length;
                }
                return length;
            }
            inline bool                             ToArray(unsigned char* packet, int length)
            {
                int sizeOf = SizeOf();
                if (sizeOf < 0)
                {
                    return false;
                }

                if (length < 0)
                {
                    return false;
                }

                unsigned char* sz = packet;
                if (!sz)
                {
                    return false;
                }

                if (length < sizeOf)
                {
                    return false;
                }

                TransitrouteHeader* hdr = (TransitrouteHeader*)sz;
                hdr->CommandId = this->CommandId;
                hdr->LinkType = this->LinkType;
                hdr->SequenceNo = this->SequenceNo;
                hdr->ServerNo = this->ServerNo;
                hdr->PlatformLength = (int)this->Platform.size();
                if (hdr->PlatformLength != (int)this->Platform.size())
                {
                    return false;
                }

                sz += sizeof(TransitrouteHeader);
                memcpy(sz, (char*)this->Platform.data(), hdr->PlatformLength);
                sz += hdr->PlatformLength;

                std::shared_ptr<BufferSegment> payload = this->Payload;
                if (payload)
                {
                    unsigned char* data = payload->UnsafeAddrOfPinnedArray();
                    if (NULL == data && 0 != payload->Length)
                    {
                        return false;
                    }

                    memcpy(sz, data, payload->Length);
                }

                return true;
            }
            inline std::shared_ptr<unsigned char>   ToPacket(Byte fk, UShort cmd, UInt32 ackNo, unsigned int& packetSize)
            {
                packetSize = 0;
                Int64 sizeOf = SizeOf();
                if (sizeOf <= 0) {
                    return make_shared_null<unsigned char>();
                }

#pragma pack(push, 1)
                struct pkg
                {
                    socket_packet_hdrc  hdr;
                    unsigned char       asz[0xffff];
                };
#pragma pack(pop)
                Int32 pkgsz = sizeOf + sizeof(socket_packet_hdrc);
                if (pkgsz <= 0) {
                    return make_shared_null<unsigned char>();
                }

                std::shared_ptr<unsigned char> packet = make_shared_array<unsigned char>(pkgsz);
                if (!packet) {
                    return make_shared_null<unsigned char>();
                }

                pkg* po = (pkg*)packet.get();
                po->hdr.fk = fk;
                po->hdr.compressed = false;
                po->hdr.mask = 0;
                po->hdr.plsz = 0;
                po->hdr.cmd = cmd;
                po->hdr.seq = ackNo;
                po->hdr.len = sizeOf;

                if (!ToArray(po->asz, sizeOf)) {
                    return make_shared_null<unsigned char>();
                }

                packetSize = pkgsz;
                return packet;
            }
        };
    }
}