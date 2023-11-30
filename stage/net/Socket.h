#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/threading/Timer.h"
#include "ISocket.h"
#include "Message.h"

namespace ep
{
    namespace net
    {
        class Socket : public ISocket
        {
        private:
            friend class SocketListener;

        public:
            static const int ServerKf                                                                       = 0x2a;
            static const int SessionKf                                                                      = 0x2e;

        public:
            Socket(const std::shared_ptr<Hosting>& hosting, Byte fk, std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
            Socket(const std::shared_ptr<Hosting>& hosting, Byte fk, const std::string& hostname, int port);
            virtual ~Socket();

        public:
            virtual std::shared_ptr<Socket>                                                                 GetPtr();
            virtual bool                                                                                    IsDisposed();
            virtual bool                                                                                    Available();

        public:
            virtual void                                                                                    Dispose();
            void                                                                                            Close();
            virtual void                                                                                    Abort();
            virtual Socket&                                                                                 Open();
            virtual Byte                                                                                    GetKf();
            virtual bool                                                                                    IsNoDelay();

        public:
            virtual bool                                                                                    Send(const Message& message, const SendAsyncCallback& callback);
            virtual bool                                                                                    Send(const std::shared_ptr<unsigned char>& packet, size_t packet_size, const SendAsyncCallback& callback);
            std::shared_ptr<unsigned char>                                                                  ToPacket(const Message& message, size_t& packet_size);
            virtual std::shared_ptr<unsigned char>                                                          ToPacket(UInt32 commandId, UInt32 sequenceNo, const unsigned char* payload, UInt32 payload_size, size_t& packet_size);

        public:
            virtual int                                                                                     GetHandle();
            virtual std::shared_ptr<boost::asio::ip::tcp::socket>                                           GetSocketObject();
            virtual boost::asio::ip::tcp::endpoint                                                          GetLocalEndPoint();
            virtual boost::asio::ip::tcp::endpoint                                                          GetRemoteEndPoint();
            virtual boost::asio::ip::tcp::endpoint                                                          ResolveEndPoint(const char* hostname, int port);
            virtual const std::string&                                                                      GetHostName();
            virtual int                                                                                     GetPort();
            virtual bool                                                                                    IsServeringMode();

        public:
            enum SelectMode
            {
                SelectMode_SelectRead,
                SelectMode_SelectWrite,
                SelectMode_SelectError,
            };
            static bool                                                                                     Poll(int s, int microSeconds, SelectMode mode);
            static bool                                                                                     Poll(const Socket& s, int microSeconds, SelectMode mode);
            static boost::asio::ip::tcp::endpoint                                                           ResolveEndPoint(boost::asio::ip::tcp::resolver& resolver, const char* hostname, int port);
            inline static UShort                                                                            GetPort(const boost::asio::ip::tcp::endpoint& ep)
            {
                return ep.port();
            }
            inline static UInt32                                                                            GetAddressV4(const boost::asio::ip::tcp::endpoint& ep)
            {
                boost::asio::ip::address address = ep.address();
                if (!address.is_v4()) {
                    return 0;
                }
                boost::asio::ip::address_v4 addressV4 = address.to_v4();
                return ntohl(addressV4.to_ulong());
            }
            inline static std::string                                                                       GetAddressTextV4(const boost::asio::ip::tcp::endpoint& ep)
            {
                UInt32 address = GetAddressV4(ep);
                return ToAddressV4(address);
            }
            inline static std::string                                                                       ToAddressV4(UInt32 address)
            {
                Byte* p = (Byte*)&address;
                char sz[65];
                sprintf(sz, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
                return sz;
            }
            inline static std::string                                                                       ToAddressV4(const boost::asio::ip::tcp::endpoint& ep)
            {
                char sz[536];
                sprintf(sz, "%s:%u", GetAddressTextV4(ep).data(), GetPort(ep));
                return sz;
            }
            inline static UShort                                                                            Checksum(void* dataptr, int len)
            {
                return (UShort)~MaskOff(dataptr, len);
            }
            static UShort                                                                                   MaskOff(void* dataptr, int len);
            static UInt32                                                                                   MaskOff32(void* dataptr, int len, UInt32 acc);

        private:
            int                                                                                             StartReceive();
            void                                                                                            CloseOrAbort(bool aborting);

        protected:
            void                                                                                            AbortINT3();
            virtual int                                                                                     PullUpListen();
            virtual bool                                                                                    CheckingMaskOff(socket_packet_hdrc* header, const std::shared_ptr<BufferSegment>& payload);
            virtual bool                                                                                    IsNeedToVerifyMaskOff();

        protected:
            virtual int                                                                                     ProcessReceive(const boost::system::error_code& ec, std::size_t sz);
            virtual int                                                                                     ProcessSend(const boost::system::error_code& ec, std::size_t sz);
            virtual void                                                                                    ProcessInput(std::shared_ptr<Message>& message);
            virtual std::shared_ptr<Message>                                                                DecryptMessage(const std::shared_ptr<Message>& message, bool compressed);
            virtual std::shared_ptr<unsigned char>                                                          EncryptMessage(UShort commandId, const std::shared_ptr<unsigned char>& payload, int& counts, bool& compressed);
            virtual bool                                                                                    ProcessSynced();

        protected:
            inline void                                                                                     Open(bool servering)
            {
                Open(servering, true);
            }
            virtual void                                                                                    Open(bool servering, bool pullUpListen);

        public:
            static const int                                                                                MTU = 1500;
            static const int                                                                                MSS = 1400;

        private:
            enum
            {
                HR_SUCCESSS,
                HR_ABORTING,
                HR_CLOSEING,
                HR_CANCELING,
            };
            bool                                                                                            _disposed : 1;
            bool                                                                                            _clientMode : 1;
            bool                                                                                            _opened : 1;
            bool                                                                                            _pullUp : 5;
            int                                                                                             _ndfs;
            Byte                                                                                            _fk;
            int                                                                                             _port;
            std::string                                                                                     _hostname;
            std::shared_ptr<boost::asio::ip::tcp::socket>                                                   _socket;
            boost::asio::ip::tcp::endpoint                                                                  _localEP;
            boost::asio::ip::tcp::endpoint                                                                  _remoteEP;
            class Receiver
            {
            public:
                socket_packet_hdrc                                                                          phdr;
                bool                                                                                        fhdr;
                unsigned int                                                                                ofs;
                std::shared_ptr<BufferSegment>                                                              payload;

            public:
                Receiver();
                ~Receiver();

            public:
                void                                                                                        Clear();
            }                                                                                               _receiver;
        };
    }
}