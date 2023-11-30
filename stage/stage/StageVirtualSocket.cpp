#include "StageClient.h"
#include "StageVirtualSocket.h"

namespace ep
{
    namespace stage
    {
        StageVirtualSocket::StageVirtualSocket(const std::shared_ptr<StageClient>& transmission, UInt32 handle, UInt32 address, int port)
            : ISocket(transmission->GetHosting())
            , Tag(NULL)
            , UserToken(NULL)
            , _handle(handle)
            , _disposed(false)
            , _preventAbort(false)
            , _address(address)
            , _port(port)
            , _transmission(transmission)
        {

        }

        StageVirtualSocket::~StageVirtualSocket()
        {
            Dispose();
        }

        bool StageVirtualSocket::Available()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            auto transmission = GetTransmissionObject();
            if (!transmission) {
                return false;
            }
            if (transmission->GetAvailableChannels() > 0) {
                if (transmission->ContainsVirtualSocket(_handle)) {
                    return true;
                }
            }
            return false;
        }

        void StageVirtualSocket::Abort()
        {
            AbortINT3(true);
        }

        bool StageVirtualSocket::DestroyObject(std::shared_ptr<StageClient>& transmission)
        {
            if (!this) {
                return false;
            }
            if (!transmission) {
                return false;
            }
            Int64 handle = GetHandle();
            return transmission->AbortVirtualSocket(handle) ? true : false;
        }

        bool StageVirtualSocket::RebindEndPoint(UInt32 address, int port)
        {
            if (this) {
                _address = address;
                _port = port;
            }
            return true;
        }

        bool StageVirtualSocket::PreventAbort()
        {
            if (this) {
                _preventAbort = true;
                return true;
            }
            return false;
        }

        bool StageVirtualSocket::PostLastAbortIL()
        {
            return Send(model::Commands_AbortClient, NULL);
        }

        void StageVirtualSocket::Close()
        {
            Dispose();
        }

        void StageVirtualSocket::Dispose()
        {
            AbortINT3(false);
        }

        bool StageVirtualSocket::Send(const Message& message, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            std::shared_ptr<StageClient> transmission;
            if (!GetTransmissionObject(transmission)) {
                return false;
            }

            unsigned short linkNo = _handle;
            Byte linkType = transmission->GetLinkType();

            BulkManyMessage bulk;
            bulk.SequenceNo = message.SequenceNo;
            bulk.CommandId = message.CommandId;
            bulk.MemberType = linkType;
            bulk.Member = &linkNo;
            bulk.MemberCount = 1;
            bulk.PayloadSize = 0;
            bulk.PayloadPtr = NULL;
            bulk.Kf = transmission->GetKf();

            std::shared_ptr<BufferSegment> segment = message.Payload;
            if (segment) {
                std::shared_ptr<unsigned char> payload = segment->Buffer;
                if (segment->Offset < 0 || segment->Length < 0) {
                    AbortINT3(true);
                    return false;
                }

                if (!payload && segment->Length != 0) {
                    AbortINT3(true);
                    return false;
                }

                if (segment->Length > 0) {
                    bulk.PayloadSize = segment->Length;
                    bulk.PayloadPtr = segment->UnsafeAddrOfPinnedArray();
                }
            }

            return transmission->Send(bulk, callback);
        }

        bool StageVirtualSocket::Send(Commands commandId, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            Message message;
            message.CommandId = (UInt32)commandId;
            message.SequenceNo = Message::NewId();
            return Send(message, callback);
        }

        bool StageVirtualSocket::Send(const std::shared_ptr<unsigned char> packet, int counts, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (!packet && counts <= 0) {
                return false;
            }

            std::shared_ptr<BufferSegment> payload = make_shared_object<BufferSegment>();
            if (!payload) {
                return false;
            }
            else {
                payload->Reset(packet, 0, counts);
            }

            Message message;
            message.SequenceNo = Message::NewId();
            message.CommandId = model::Commands_RoutingTransparent;
            message.Payload = payload;

            return Send(message, callback);
        }

        bool StageVirtualSocket::Send(const unsigned char* packet, int counts, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (!packet && counts <= 0) {
                return false;
            }

            std::shared_ptr<unsigned char> payload = warp_shared_object<unsigned char>(packet);
            return Send(payload, counts, callback);
        }

        boost::asio::ip::tcp::endpoint StageVirtualSocket::GetLocalEndPoint()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (!_disposed) {
                auto transmission = GetTransmissionObject();
                if (transmission) {
                    return transmission->GetRemoteEndPoint();
                }
            }
            return SocketListener::AnyAddress(0);
        }

        boost::asio::ip::tcp::endpoint StageVirtualSocket::GetRemoteEndPoint()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (!_disposed) {
                boost::asio::ip::tcp::endpoint defaultEP(boost::asio::ip::address_v4(_address), _port);
                return defaultEP;
            }
            return SocketListener::AnyAddress(0);
        }

        Byte StageVirtualSocket::GetLinkType()
        {
            std::shared_ptr<StageClient> transmission = make_shared_null<StageClient>();
            if (!GetTransmissionObject(transmission)) {
                return '\xff';
            }
            return transmission->GetLinkType();
        }

        void StageVirtualSocket::AbortINT3(bool abortingMode)
        {
            std::exception* exception = NULL;
            std::shared_ptr<StageClient> transmission = make_shared_null<StageClient>();
            Object::synchronize(GetSynchronizingObject(), [&] {
                if (!_disposed) {
                    transmission = _transmission;
                    if (transmission) {
                        if (!_preventAbort) {
                            PostLastAbortIL();
                        }
                    }
                    try {
                        OnAbort(EventArgs::Empty);
                    }
                    catch (std::exception& e) {
                        exception = Object::addressof(e);
                    }
                }
                _disposed = true;
                _transmission.reset();
            });
            DestroyObject(transmission);
            if (abortingMode) {
                Abort();
            }
            ISocket::Dispose();
            if (exception) {
                throw *exception;
            }
        }

        void StageVirtualSocket::ProcessInput(std::shared_ptr<Message>& message)
        {
            OnMessage(message);
        }

        void StageVirtualSocket::OnOpen(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageVirtualSocket> socket = GetPtr();
                transmission->OnOpen(socket, e);
            }
            ISocket::OnOpen(e);
        }

        void StageVirtualSocket::OnAbort(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageVirtualSocket> socket = GetPtr();
                transmission->OnAbort(socket, e);
            }
            ISocket::OnAbort(e);
        }

        void StageVirtualSocket::OnClose(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageVirtualSocket> socket = GetPtr();
                transmission->OnAbort(socket, e);
            }
            ISocket::OnClose(e);
        }

        void StageVirtualSocket::OnMessage(std::shared_ptr<Message>& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageVirtualSocket> socket = GetPtr();
                transmission->OnMessage(socket, e);
            }
            ISocket::OnMessage(e);
        }

        std::shared_ptr<StageClient> StageVirtualSocket::GetTransmissionObject()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            return _transmission;
        }

        bool StageVirtualSocket::GetTransmissionObject(std::shared_ptr<StageClient>& transmission)
        {
            StageVirtualSocket* socket = this;
            if (!socket) {
                return false;
            }
            transmission = make_shared_null<StageClient>();
            if (!Object::synchronize<bool>(GetSynchronizingObject(), [&] {
                if (_disposed) {
                    return false;
                }

                transmission = GetTransmissionObject();
                if (!transmission) {
                    return false;
                }

                if (transmission->IsDisposed()) {
                    return false;
                }
                return true;
            })) {
                return false;
            }
            return true;
        }

        std::shared_ptr<Hosting> StageVirtualSocket::GetHosting()
        {
            std::shared_ptr<StageClient> transmission;
            if (GetTransmissionObject(transmission)) {
                return transmission->GetHosting();
            }
            return make_shared_null<Hosting>();
        }

        SyncObjectBlock& StageVirtualSocket::GetSynchronizingObject()
        {
            return _syncobj;
        }

        bool StageVirtualSocket::IsDisposed()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            return _disposed;
        }

        std::shared_ptr<StageVirtualSocket> StageVirtualSocket::GetPtr()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            return Object::as<StageVirtualSocket>(GetBasePtr());
        }
    }
}