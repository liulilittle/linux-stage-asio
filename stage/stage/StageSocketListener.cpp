#include "StageSocket.h"
#include "StageSocketListener.h"
#include "IStageSocketHandler.h"

namespace ep
{
    namespace stage
    {
        StageSocketListener::StageSocketListener(
            const std::shared_ptr<Hosting>&                             hosting,
            Byte                                                        fk,
            int                                                         backlog,
            int                                                         port,
            bool                                                        transparent,
            bool                                                        needToVerifyMaskOff,
            bool                                                        onlyRoutingTransparent)
            : SocketListener(hosting, fk, backlog, port)
            , _transparent(transparent)
            , _needToVerifyMaskOff(needToVerifyMaskOff)
            , _compressedMessages(true)
            , _onlyRoutingTransparent(onlyRoutingTransparent) {

        }

        StageSocketListener::~StageSocketListener() {
            Dispose();
        }

        void StageSocketListener::Dispose() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_socketHandler) {
                _socketHandler.reset();
            }
            SocketListener::Dispose();
        }

        std::shared_ptr<IStageSocketHandler> StageSocketListener::GetSocketHandler() {
            if (this) {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (!IsDisposed()) {
                    if (!_socketHandler) {
                        _socketHandler = CreateSocketHandler();
                    }
                }
                return _socketHandler;
            }
            return make_shared_null<IStageSocketHandler>();
        }

        bool StageSocketListener::IsTransparent()
        {
            if (!this) {
                return false;
            }
            return _transparent;
        }

        bool StageSocketListener::IsOnlyRoutingTransparent()
        {
            if (!this) {
                return false;
            }
            return _onlyRoutingTransparent;
        }

        bool StageSocketListener::IsNeedToVerifyMaskOff()
        {
            if (!this) {
                return false;
            }
            return _needToVerifyMaskOff;
        }

        bool StageSocketListener::IsEnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            return _compressedMessages;
        }

        bool StageSocketListener::EnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            _compressedMessages = true;
            return true;
        }

        bool StageSocketListener::DisabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            _compressedMessages = false;
            return true;
        }

        std::shared_ptr<Socket> StageSocketListener::CreateSocket(std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
            std::shared_ptr<StageSocket> client = make_shared_null<StageSocket>();
            if (NULL != this) {
                client = make_shared_object<StageSocket>(GetHostingObject(), GetSocketHandler(), GetKf(), socket, IsTransparent(), IsOnlyRoutingTransparent());
            }
            if (client) {
                StageSocketBuilder* builder = client->GetBuilder();
                if (NULL == builder) {
                    client.reset();
                }
                else {
                    if (IsEnabledCompressedMessages()) {
                        client->EnabledCompressedMessages();
                    }
                    else {
                        client->DisabledCompressedMessages();
                    }
                    builder->SetNeedToVerifyMaskOff(IsNeedToVerifyMaskOff());
                }
            }
            return Object::as<Socket>(client);
        }

        std::shared_ptr<IStageSocketHandler> StageSocketListener::CreateSocketHandler() {
            std::shared_ptr<IStageSocketHandler> client = make_shared_null<IStageSocketHandler>();
            if (this) {
                std::shared_ptr<StageSocketListener> listener = Object::as<StageSocketListener>(GetPtr());
                if (listener) {
                    client = make_shared_object<IStageSocketHandler>(listener);
                }
            }
            return client;
        }

        StageSocketListener2::StageSocketListener2(const std::shared_ptr<Hosting>& hosting, Byte fk, int backlog, int port, bool transparent, bool needToVerifyMaskOff, bool onlyRoutingTransparent, const CreateSocketHandlerObject& createSocketHandler)
            : StageSocketListener(hosting, fk, backlog, port, transparent, needToVerifyMaskOff, onlyRoutingTransparent)
            , _createSocketHandler(createSocketHandler)
        {

        }

        StageSocketListener2::~StageSocketListener2()
        {
            Dispose();
        }

        void StageSocketListener2::Dispose()
        {
            StageSocketListener2* self = this;
            if (self)
            {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (_createSocketHandler)
                {
                    _createSocketHandler = NULL;
                }
                StageSocketListener::Dispose();
            }
        }

        std::shared_ptr<IStageSocketHandler> StageSocketListener2::CreateSocketHandler()
        {
            StageSocketListener2* self = this;
            if (!self)
            {
                return make_shared_null<IStageSocketHandler>();
            }

            if (!_createSocketHandler)
            {
                return StageSocketListener::CreateSocketHandler();
            }

            std::shared_ptr<StageSocketListener> listener = Object::as<StageSocketListener>(GetPtr());
            return _createSocketHandler(listener);
        }
    }
}