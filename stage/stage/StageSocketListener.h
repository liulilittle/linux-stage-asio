#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/net/SocketListener.h"

namespace ep
{
    namespace stage
    {
        using ep::net::Socket;
        using ep::net::SocketListener;

        class StageSocket;
        class IStageSocketHandler;
        class StageSocketListener;

        class StageSocketListener : public SocketListener
        {
        public:
            StageSocketListener(
                const std::shared_ptr<Hosting>&                                                                             hosting,
                Byte                                                                                                        fk,
                int                                                                                                         backlog,
                int                                                                                                         port,
                bool                                                                                                        transparent,
                bool                                                                                                        needToVerifyMaskOff,
                bool                                                                                                        onlyRoutingTransparent);
            virtual ~StageSocketListener();

        public:
            virtual void                                                                                                    Dispose();
            virtual std::shared_ptr<IStageSocketHandler>                                                                    GetSocketHandler();
            virtual bool                                                                                                    IsTransparent();
            virtual bool                                                                                                    IsOnlyRoutingTransparent();
            virtual bool                                                                                                    IsNeedToVerifyMaskOff();

        public:
            virtual bool                                                                                                    IsEnabledCompressedMessages();
            virtual bool                                                                                                    EnabledCompressedMessages();
            virtual bool                                                                                                    DisabledCompressedMessages();

        protected:
            virtual std::shared_ptr<Socket>                                                                                 CreateSocket(std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
            virtual std::shared_ptr<IStageSocketHandler>                                                                    CreateSocketHandler();

        private:
            bool                                                                                                            _transparent : 1;
            bool                                                                                                            _needToVerifyMaskOff : 1;
            bool                                                                                                            _compressedMessages : 1;
            bool                                                                                                            _onlyRoutingTransparent : 5;
            std::shared_ptr<IStageSocketHandler>                                                                            _socketHandler;
        };

        class StageSocketListener2 : public StageSocketListener
        {
        public:
            typedef std::function<std::shared_ptr<IStageSocketHandler>(std::shared_ptr<StageSocketListener>& listener)>     CreateSocketHandlerObject;

        public:
            StageSocketListener2(
                const std::shared_ptr<Hosting>&                                                                             hosting,
                Byte                                                                                                        fk,
                int                                                                                                         backlog,
                int                                                                                                         port,
                bool                                                                                                        transparent,
                bool                                                                                                        needToVerifyMaskOff,
                bool                                                                                                        onlyRoutingTransparent,
                const CreateSocketHandlerObject&                                                                            createSocketHandler);
            virtual ~StageSocketListener2();

        public:
            virtual void                                                                                                    Dispose();
            virtual std::shared_ptr<IStageSocketHandler>                                                                    CreateSocketHandler();

        private:
            CreateSocketHandlerObject                                                                                       _createSocketHandler;
        };
    }
}