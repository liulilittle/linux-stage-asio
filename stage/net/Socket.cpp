#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <sys/types.h>
#ifdef WIN32
#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <error.h>
#include <sys/poll.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <strstream>

#include "Socket.h"
#include "Message.h"
#include "SocketListener.h"

namespace ep
{
    namespace net
    {
        const int Socket::ServerKf;
        const int Socket::SessionKf;

        Socket::Socket(const std::shared_ptr<Hosting>& hosting, Byte fk, std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
            : ISocket(hosting)
            , _disposed(false)
            , _clientMode(false)
            , _opened(false)
            , _pullUp(false)
            , _ndfs(~0)
            , _fk(fk)
            , _port(0)
            , _socket(socket) {
            if (!socket) {
                throw std::runtime_error("Instances of supplied sockets are not allowed to be NullReferences");
            }

            if (!hosting) {
                throw std::runtime_error("A local host instance hosting the current running instance is not allowed to be a NullReferences");
            }
            else {
                _receiver.Clear();
            }

            _ndfs = socket->native_handle();
            _localEP = socket->local_endpoint();
            _remoteEP = socket->remote_endpoint();
        }

        Socket::Socket(const std::shared_ptr<Hosting>& hosting, Byte fk, const std::string& hostname, int port)
            : ISocket(hosting)
            , _disposed(false)
            , _clientMode(true)
            , _opened(false)
            , _pullUp(false)
            , _ndfs(~0)
            , _fk(fk)
            , _port(port)
            , _hostname(hostname) {

            if (hostname.empty()) {
                throw std::runtime_error("The remote host name of the link is not allowed to be NullReferences");
            }

            if (port <= 0 || port > 65535) {
                throw std::runtime_error("Ports linked to remote hosts must not be less than or equal to 0 or greater than 65535");
            }

            if (!hosting) {
                throw std::runtime_error("A local host instance hosting the current running instance is not allowed to be a NullReferences");
            }
            else {
                _receiver.Clear();
            }
        }

        Socket::~Socket() {
            Dispose();
        }

        std::shared_ptr<Socket> Socket::GetPtr() {
            Socket* self = this;
            if (self) {
                std::shared_ptr<ISocket> basePtr = GetBasePtr();
                return Object::as<Socket>(basePtr);
            }
            return Object::Nulll<Socket>();
        }

        void Socket::Dispose() {
            CloseOrAbort(false);
        }

        void Socket::Close() {
            Dispose();
        }

        void Socket::Abort() {
            CloseOrAbort(true);
        }

        bool Socket::Send(const Message& message, const SendAsyncCallback& callback) {
            if (!this) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                AbortINT3();
                return false;
            }

            boost::asio::ip::tcp::socket* socket = _socket.get();
            if (!socket) {
                AbortINT3();
                return false;
            }

            size_t counts = 0;
            std::shared_ptr<unsigned char> packet = ToPacket(message, counts);

            return Send(packet, counts, callback);
        }

        bool Socket::Send(const std::shared_ptr<unsigned char>& packet, size_t packet_size, const SendAsyncCallback& callback) {
            std::shared_ptr<Socket> self = GetPtr();
            if (!self) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                AbortINT3();
                return false;
            }

            boost::asio::ip::tcp::socket* socket = _socket.get();
            if (!socket) {
                AbortINT3();
                return false;
            }

            if (0 == packet_size || !packet.get()) {
                AbortINT3();
                return false;
            }

            auto callbackf = GetStrandObject().wrap([self, callback](const boost::system::error_code& ec, std::size_t sz) {
                if (self) {
                    int hr = HR_CANCELING;
                    if (!self->IsDisposed()) {
                        hr = self->ProcessSend(ec, sz);
                        switch (hr)
                        {
                        case HR_SUCCESSS:
                            break;
                        case HR_ABORTING:
                            self->AbortINT3();
                            break;
                        case HR_CLOSEING:
                            self->Close();
                            break;
                        default:
                            self->AbortINT3();
                            break;
                        }
                    }
                    if (callback) {
                        std::shared_ptr<ISocket> sender = Object::as<ISocket>(self);
                        callback(sender, hr, sz);
                    }
                }
            });
            try { // 尝试以异步的方式发送一帧网络数据，假设失败或者抛出异常则中断链接
                socket->async_send(boost::asio::buffer(packet.get(), packet_size), callbackf);
                return true;
            }
            catch (std::exception&) {
                AbortINT3();
                return false;
            }
        }

        std::shared_ptr<unsigned char> Socket::ToPacket(const Message& message, size_t& packet_size) {
            packet_size = 0;

            size_t size = 0;
            std::shared_ptr<BufferSegment> segment = message.Payload;
            if (segment) {
                if (segment->Length < 0 || segment->Offset < 0) {
                    AbortINT3();
                    return false;
                }
                size = segment->Length;
            }

            unsigned char* sz = segment->UnsafeAddrOfPinnedArray();
            return ToPacket(message.CommandId, message.SequenceNo, sz, size, packet_size);
        }

        std::shared_ptr<unsigned char> Socket::ToPacket(UInt32 commandId, UInt32 sequenceNo, const unsigned char* payload, UInt32 payload_size, size_t& packet_size) {
            packet_size = 0;
            if (NULL == payload && packet_size > 0) {
                AbortINT3();
                return make_shared_null<unsigned char>();
            }

            bool compressed = false;
            auto pl = warp_shared_object<unsigned char>(payload);
            do {
                int plsz = payload_size;
                if (plsz > 0) {
                    pl = EncryptMessage(commandId, pl, plsz, compressed);
                }
                if (plsz < 0) {
                    AbortINT3();
                    return make_shared_null<unsigned char>();
                }

                payload = pl.get();
                payload_size = plsz;
            } while (0, 0);
            if (NULL == payload && packet_size > 0) {
                AbortINT3();
                return make_shared_null<unsigned char>();
            }

            size_t offset = sizeof(socket_packet_hdrc);
            size_t counts = offset + payload_size;

            std::shared_ptr<unsigned char> packet = make_shared_null<unsigned char>();
            if (counts > 0) {
                packet = make_shared_array<unsigned char>(counts);
            }

            if (!packet) {
                AbortINT3();
                return false;
            }

            socket_packet_hdrc* hdr = (socket_packet_hdrc*)packet.get();
            hdr->fk = GetKf();
            hdr->compressed = compressed;
            hdr->cmd = commandId;
            hdr->seq = sequenceNo;
            hdr->mask = 0;
            hdr->len = payload_size;

            if (payload_size > 0) {
                unsigned char* psz = packet.get() + offset;
                memcpy(psz, payload, payload_size);
            }

            packet_size = counts;
            return packet;
        }

        void Socket::Open(bool servering, bool pullUpListen) {

            int hr = 0;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (_disposed) {
                    hr = 1;
                    break;
                }
                if (_opened) {
                    hr = 2;
                    break;
                }
                if (servering != IsServeringMode()) {
                    hr = 3;
                    break;
                }
                _opened = true;
                if (servering) {
                    OnOpen(EventArgs::Empty);
                    if (pullUpListen) {
                        PullUpListen();
                    }
                }
                else {
                    _socket = make_shared_object<boost::asio::ip::tcp::socket>(GetContextObject());
                    do {
                        if (!_socket) {
                            AbortINT3();
                            break;
                        }

                        boost::asio::ip::tcp::socket& socket = *_socket;
                        boost::asio::ip::tcp::endpoint server = ResolveEndPoint(_hostname.data(), _port);

                        // 无法打开内核套接字对象句柄实例时，强制中断当前的套接字实例
                        boost::system::error_code ec;
                        try {
                            if (server.protocol() != boost::asio::ip::tcp::v6()) {
                                socket.open(boost::asio::ip::tcp::v4(), ec);
                            }
                            else {
                                socket.open(boost::asio::ip::tcp::v6(), ec);
                            }
                            if (ec) {
                                AbortINT3();
                                break;
                            }
                        }
                        catch (std::exception&) {
                            AbortINT3();
                            break;
                        }

                        _remoteEP = server;
                        _ndfs = socket.native_handle();

                        if (!SocketListener::NoDelay(socket, IsNoDelay())) { // 通常这发生在未向内核构建套接字对象或者已经从内核彻底关闭了这个套接字
                            AbortINT3();
                            break;
                        }

                        auto self = GetPtr();
                        auto callbackf = GetStrandObject().wrap([self](const boost::system::error_code& ec) {
                            if (self) {
                                std::unique_lock<SyncObjectBlock> scoped(self->GetSynchronizingObject());
                                do {
                                    bool closing = true;
                                    do {
                                        if (ec) {
                                            break;
                                        }

                                        if (self->IsDisposed()) {
                                            break;
                                        }

                                        if (!self->ProcessSynced()) {
                                            break;
                                        }
                                        else {
                                            auto socket = self->_socket;
                                            if (socket) {
                                                self->_localEP = socket->local_endpoint();
                                            }
                                        }
                                        closing = false;
                                    } while (0, 0);
                                    if (closing) {
                                        self->AbortINT3();
                                    }
                                } while (0, 0);
                            }
                        });

                        // 若无法对内核套接字执行异步的链接操作时，强制中断当前的套接字实例
                        try {
                            socket.async_connect(server, callbackf);
                        }
                        catch (std::exception&) {
                            AbortINT3();
                            break;
                        }
                    } while (0, 0);
                }
            } while (0, 0);
            switch (hr)
            {
            case 1:
                throw std::runtime_error("Managed and unmanaged resources held by the current socket object have been released");
                break;
            case 2:
                throw std::runtime_error("The current socket instance has been opened and cannot be opened repeatedly");
                break;
            case 3:
                throw std::runtime_error("Socket opening error the way the socket is opened does not match the socket type");
                break;
            }
        }

        bool Socket::ProcessSynced() {
            OnOpen(EventArgs::Empty);
            PullUpListen();
            return true;
        }

        boost::asio::ip::tcp::endpoint Socket::ResolveEndPoint(boost::asio::ip::tcp::resolver& resolver, const char* hostname, int port) {
            if (!hostname || !*hostname) {
                hostname = NULL;
            }

            boost::asio::ip::tcp::endpoint address = SocketListener::AnyAddress(0);
            if (NULL == hostname) {
                address = SocketListener::AnyAddress(port);
            }
            else {
                try {
                    address = SocketListener::GetAddressByHostName(resolver, hostname, port); // 无法进行NS/AAAA解释地址的时候
                }
                catch (std::exception&) {
                    if (NULL == hostname) {
                        address = SocketListener::AnyAddress(port);
                    }
                    else {
                        address = SocketListener::NewAddress(hostname, port);
                    }
                }
            }
            return address;
        }

        boost::asio::ip::tcp::endpoint Socket::ResolveEndPoint(const char* hostname, int port) {
            boost::asio::ip::tcp::resolver resolver(GetContextObject());
            return ResolveEndPoint(resolver, hostname, port);
        }

        const std::string& Socket::GetHostName() {
            return _hostname;
        }

        int Socket::GetPort() {
            return _port;
        }

        UShort Socket::MaskOff(void* dataptr, int len)
        {
            UInt32 acc = MaskOff32(dataptr, 0, 0);
            /* This maybe a little confusing: reorder sum using htons()
               instead of ntohs() since it has a little less call overhead.
               The caller must invert bits for Internet sum ! */
            return ntohs((UShort)acc);
        }

        UInt32 Socket::MaskOff32(void* dataptr, int len, UInt32 acc)
        {
            UShort src;
            Byte* octetptr;

            /* dataptr may be at odd or even addresses */
            octetptr = (Byte*)dataptr;
            while (len > 1)
            {
                /* declare first octet as most significant
                   thus assume network order, ignoring host order */
                src = (UShort)((*octetptr) << 8);
                octetptr++;
                /* declare second octet as least significant */
                src |= (*octetptr);
                octetptr++;
                acc += src;
                len -= 2;
            }
            if (len > 0)
            {
                /* accumulate remaining octet */
                src = (UShort)((*octetptr) << 8);
                acc += src;
            }
            /* add deferred carry bits */
            acc = (UInt32)((acc >> 16) + (acc & 0x0000ffffUL));
            if ((acc & 0xffff0000UL) != 0)
            {
                acc = (UInt32)((acc >> 16) + (acc & 0x0000ffffUL));
            }
            return acc;
        }

        bool Socket::IsServeringMode() {
            return !_clientMode;
        }

        Socket& Socket::Open() {
            Open(false);
            return *this;
        }

        Byte Socket::GetKf() {
            return _fk;
        }

        bool Socket::IsNoDelay() {
            return true;
        }

        int Socket::GetHandle() {
            return _ndfs;
        }

        std::shared_ptr<boost::asio::ip::tcp::socket> Socket::GetSocketObject() {
            return _socket;
        }

        boost::asio::ip::tcp::endpoint Socket::GetLocalEndPoint() {
            return _localEP;
        }

        bool Socket::IsDisposed() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _disposed;
        }

        bool Socket::Available() {
            if (IsDisposed()) {
                return false;
            }
            return Poll(*this, 0, SelectMode_SelectWrite);
        }

        boost::asio::ip::tcp::endpoint Socket::GetRemoteEndPoint() {
            return _remoteEP;
        }

        void Socket::AbortINT3() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!IsDisposed() || _socket) {
                Abort();
                Dispose();
            }
            else {
                Dispose();
            }
        }

        int Socket::PullUpListen() {
            return StartReceive();
        }

        int Socket::StartReceive() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                AbortINT3();
                return -1;
            }

            std::shared_ptr<boost::asio::ip::tcp::socket> socket = _socket;
            if (!socket) {
                AbortINT3();
                return -2;
            }

            if (_pullUp) {
                return 0;
            }

            boost::asio::mutable_buffer buffer;
            if (!_receiver.fhdr) {
                unsigned char* sz = (unsigned char*)_receiver.phdr.get();
                if (!sz) {
                    AbortINT3();
                    return -3;
                }

                unsigned long len = unchecked(sizeof(socket_packet_hdrc) - _receiver.ofs);
                if (!sz || 0 == len) {
                    AbortINT3();
                    return -4;
                }

                if (len > MSS) {
                    len = MSS;
                }

                sz += _receiver.ofs;
                buffer = boost::asio::buffer(sz, len);
            }
            else {
                std::shared_ptr<BufferSegment>& payload = _receiver.payload;
                if (!payload || payload->Length <= 0) {
                    AbortINT3();
                    return -5;
                }

                socket_packet_hdrc* hdr = _receiver.phdr.get();
                if (!hdr) {
                    AbortINT3();
                    return -6;
                }

                unsigned int plsz = hdr->len;
                if (IsNeedToVerifyMaskOff()) {
                    plsz = hdr->plsz;
                }

                unsigned char* sz = payload->UnsafeAddrOfPinnedArray();
                unsigned long len = unchecked(plsz - _receiver.ofs);
                if (!sz || 0 == len) {
                    AbortINT3();
                    return -7;
                }

                if (len > MSS) {
                    len = MSS;
                }

                sz += _receiver.ofs;
                buffer = boost::asio::buffer(sz, len);
            }

            std::shared_ptr<Socket> self = GetPtr();
            auto callbackf = GetStrandObject().wrap([self](const boost::system::error_code& ec, std::size_t sz) {
                if (self) {
                    std::unique_lock<SyncObjectBlock> scoped(self->GetSynchronizingObject());
                    if (self->IsDisposed()) {
                        return;
                    }
                    else {
                        self->_pullUp = false;
                    }
                    int hr = HR_SUCCESSS;
                    if (ec.value() != boost::system::errc::resource_unavailable_try_again) {
                        hr = self->ProcessReceive(ec, sz);
                    }
                    switch (hr)
                    {
                    case HR_SUCCESSS:
                        self->StartReceive();
                        break;
                    case HR_ABORTING:
                        self->AbortINT3();
                        break;
                    case HR_CLOSEING:
                        self->Close();
                        break;
                    default:
                        self->AbortINT3();
                        break;
                    }
                }
            });
            try { // 尝试开始接收一次数据，假设失败或者抛出异常则中断链接
                socket->async_receive(boost::asio::buffer(buffer), callbackf);
                return 1;
            }
            catch (std::exception&) {
                AbortINT3();
                return -8;
            }
        }

        int Socket::ProcessReceive(const boost::system::error_code& ec, std::size_t sz) {
            if (ec) {
                return HR_ABORTING;
            }

            if (0 == sz) { // FIN
                return HR_CLOSEING;
            }

            std::shared_ptr<Message> message = Object::Nulll<Message>();
            if (!_receiver.fhdr) { // 未收到帧头
                socket_packet_hdrc* hdr = _receiver.phdr.get();
                if (!hdr) {
                    return HR_ABORTING;
                }

                if (0 == _receiver.ofs) {
                    if (hdr->fk != GetKf()) {
                        return HR_ABORTING;
                    }
                }

                unsigned long len = sizeof(socket_packet_hdrc);
                if ((_receiver.ofs += sz) >= len) {
                    _receiver.ofs = 0;
                    _receiver.fhdr = true;

                    socket_packet_hdrc* hdr = _receiver.phdr.get();
                    assert(NULL != hdr);

                    unsigned int plsz = hdr->len;
                    if (IsNeedToVerifyMaskOff()) {
                        plsz = hdr->plsz;
                        if (0 == hdr->mask || (unsigned short)(~0) == hdr->mask) { // 不允许不提供MASK
                            return HR_ABORTING;
                        }
                    }

                    if (plsz > 0) {
                        std::shared_ptr<unsigned char> pz = make_shared_array<unsigned char>(plsz);
                        if (!pz) {
                            return HR_ABORTING;
                        }

                        _receiver.payload = make_shared_object<BufferSegment>(pz, 0, plsz);
                        if (!_receiver.payload) {
                            return HR_ABORTING;
                        }
                    }
                    else {
                        _receiver.ofs = 0;
                        _receiver.fhdr = false;

                        std::shared_ptr<BufferSegment> payload = _receiver.payload;
                        if (payload) {
                            _receiver.payload = make_shared_object<BufferSegment>();
                        }

                        message = Message::Create(hdr->cmd, hdr->seq, payload);
                        if (!message) {
                            return HR_ABORTING;
                        }
                    }
                }
            }
            else {
                socket_packet_hdrc* hdr = _receiver.phdr.get();
                if (!hdr) {
                    return HR_ABORTING;
                }

                unsigned int plsz = hdr->len;
                if (IsNeedToVerifyMaskOff()) {
                    plsz = hdr->plsz;
                    if (0 == hdr->mask || (unsigned short)(~0) == hdr->mask) { // 不允许不提供MASK
                        return HR_ABORTING;
                    }
                }

                if ((_receiver.ofs += sz) >= plsz) {
                    _receiver.ofs = 0;
                    _receiver.fhdr = false;

                    socket_packet_hdrc* hdr = _receiver.phdr.get();
                    assert(NULL != hdr);

                    std::shared_ptr<BufferSegment> payload = _receiver.payload;
                    if (payload) {
                        _receiver.payload = make_shared_object<BufferSegment>();
                    }

                    message = Message::Create(hdr->cmd, hdr->seq, payload);
                    if (!message) {
                        return HR_ABORTING;
                    }
                }
            }

            if (message) {
                socket_packet_hdrc* hdr = _receiver.phdr.get();
                if (!hdr) {
                    return HR_ABORTING;
                }

                std::shared_ptr<BufferSegment> payload = message->Payload;
                if (IsNeedToVerifyMaskOff()) {
                    bool checkingMaskOff = CheckingMaskOff(hdr, payload);
                    if (!checkingMaskOff) {
                        return HR_ABORTING;
                    }
                }

                if (payload) {
                    if (payload->Length) {
                        message = DecryptMessage(message, hdr->compressed);
                    }
                }

                if (!message) {
                    return HR_ABORTING;
                }
                else {
                    ProcessInput(message);
                }

                message = make_shared_null<Message>();
            }
            return HR_SUCCESSS;
        }

        bool Socket::CheckingMaskOff(socket_packet_hdrc* header, const std::shared_ptr<BufferSegment>& payload) {
            if (NULL == header) {
                return false;
            }

            if (0 == header->mask || (unsigned short)(~0) == header->mask) { // 套接字在客户端环境工作不允许提供(0/~0-mask)
                return false;
            }

            unsigned int plsz = header->len;
            if (IsNeedToVerifyMaskOff()) {
                plsz = header->plsz;
            }

            Byte* data = NULL;
            int datalen = 0;
            if (0 != plsz) {
                if (!payload) {
                    return false;
                }

                datalen = payload->Length;
                if (datalen <= 0) {
                    return false;
                }

                data = (Byte*)payload->UnsafeAddrOfPinnedArray();
                if (!data) {
                    return false;
                }
            }

            UInt32 inverseMask = 0;
            inverseMask = MaskOff32(header, sizeof(*header), inverseMask);
            inverseMask = MaskOff32(data, datalen, inverseMask);
            inverseMask = (UShort)~(inverseMask & 0xffffUL);
            return inverseMask == 0; // 测量的反码必须等于“0”则本次测量是未被篡改过的合法载荷
        }

        bool Socket::IsNeedToVerifyMaskOff() {
            return false;
        }

        int Socket::ProcessSend(const boost::system::error_code& ec, std::size_t sz) {
            if (ec) {
                return HR_ABORTING;
            }
            return HR_SUCCESSS;
        }

        void Socket::ProcessInput(std::shared_ptr<Message>& message) {
            OnMessage(message);
        }

        std::shared_ptr<Message> Socket::DecryptMessage(const std::shared_ptr<Message>& message, bool compressed) {
            return message;
        }

        std::shared_ptr<unsigned char> Socket::EncryptMessage(UShort commandId, const std::shared_ptr<unsigned char>& payload, int& counts, bool& compressed) {
            return payload;
        }

        void Socket::CloseOrAbort(bool aborting) {
            std::exception* exception = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                bool events = false;
                if (!_disposed) {
                    events = true;
                    if (_socket) {
                        SocketListener::Closesocket(_socket);
                    }
                    _socket.reset();
                    _disposed = true;
                }
                _receiver.Clear();
                OpenEvent = NULL;
                MessageEvent = NULL;
                if (aborting) {
                    CloseEvent = NULL;
                    if (!events) { // op: aborting or closing
                        AbortEvent = NULL;
                    }
                    else if (AbortEvent) {
                        try { // 触发中断事件可能导致触发异常，尝试捕获可能发现的异常避免内存与循环引用的问题
                            OnAbort(EventArgs::Empty);
                            AbortEvent = NULL;
                        }
                        catch (std::exception& e) { // 捕获异常，重置关闭事件处理器之后在抛出已捕获到异常给调用此函数的调用方或者操作系统
                            AbortEvent = NULL;
                            exception = Object::addressof(e);
                        }
                    }
                }
                else {
                    AbortEvent = NULL;
                    if (!events) {
                        CloseEvent = NULL;
                    }
                    else if (CloseEvent) {
                        try { // 触发关闭事件可能导致触发异常，尝试捕获可能发现的异常避免内存与循环引用的问题
                            OnClose(EventArgs::Empty);
                            CloseEvent = NULL;
                        }
                        catch (std::exception& e) { // 捕获异常，重置关闭事件处理器之后在抛出已捕获到异常给调用此函数的调用方或者操作系统
                            CloseEvent = NULL;
                            exception = Object::addressof(e);
                        }
                    }
                }
                ISocket::Dispose();
                if (events) {
                    Dispose();
                }
            } while (0, 0);
            if (NULL != exception) {
                throw *exception;
            }
        }

        bool Socket::Poll(int s, int microSeconds, SelectMode mode) {
            if (s == ~0 || s == 0) {
                return false;
            }

#ifdef WIN32
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(s, &fdset);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = microSeconds;

            int hr = -1;
            if (mode == SelectMode_SelectRead) {
                hr = select((s + 1), &fdset, NULL, NULL, &tv) > 0;
            }
            else if (mode == SelectMode_SelectWrite) {
                hr = select((s + 1), NULL, &fdset, NULL, &tv) > 0;
            }
            else {
                hr = select((s + 1), NULL, NULL, &fdset, &tv) > 0;
            }

            if (hr > 0) {
                return FD_ISSET(s, &fdset);
            }
#else
            struct pollfd fds[1];
            memset(fds, 0, sizeof(fds));

            int events = POLLERR;
            if (mode == SelectMode_SelectRead) {
                events = POLLIN;
            }
            else if (mode == SelectMode_SelectWrite) {
                events = POLLOUT;
            }
            else {
                events = POLLERR;
            }

            fds->fd = s;
            fds->events = events;

            int nfds = 1;
            int hr = poll(fds, nfds, microSeconds / 1000);
            if (hr > 0) {
                if ((fds->revents & events) == events) {
                    return true;
                }
            }
#endif
            return false;
        }

        bool Socket::Poll(const Socket& s, int microSeconds, SelectMode mode) {
            Socket* socket = (Socket*)addressof(s);
            if (!socket) {
                return false;
            }

            if (socket->IsDisposed()) {
                return false;
            }

            return Poll(socket->_ndfs, microSeconds, mode);
        }

        Socket::Receiver::Receiver() {
            Clear();
        }

        Socket::Receiver::~Receiver() {
            Clear();
        }

        void Socket::Receiver::Clear() {
            ofs = 0;
            fhdr = false;
            payload = make_shared_null<BufferSegment>();
            memset(phdr.get(), 0, sizeof(*phdr.get()));
        }
    }
}