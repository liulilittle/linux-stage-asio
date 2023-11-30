#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/net/Socket.h"
#include "util/stage/threading/Timer.h"
#include "model/Inc.h"
#include "StageSocket.h"

namespace ep
{
    namespace stage
    {
        using ep::net::Socket;
        using ep::net::Message;
        using ep::stage::model::Error;
        using ep::stage::model::Commands;
        using ep::stage::model::AuthenticationAAARequest;
        using ep::stage::model::AuthenticationAAAResponse;

        class StageSocket;
        class StageSocketListener; 
        class StageSocketCommunication;

        class IStageSocketHandler : public std::enable_shared_from_this<IStageSocketHandler>
        {
        public:
            IStageSocketHandler();
            IStageSocketHandler(const std::shared_ptr<StageSocketListener>& communication);
            virtual ~IStageSocketHandler();

        public:
            virtual void                                                                    Dispose();
            virtual std::shared_ptr<IStageSocketHandler>                                    GetBasePtr();
            virtual bool                                                                    IsDisposed();

        public:
            virtual SyncObjectBlock&                                                        GetSynchronizingObject();
            virtual std::shared_ptr<StageSocketListener>                                    GetSocketListener();

        public:
            virtual void                                                                    AbortSocket(const std::shared_ptr<StageSocket>& socket);
            virtual void                                                                    CloseSocket(const std::shared_ptr<StageSocket>& socket);

        public:
            virtual bool                                                                    ProcessAccept(std::shared_ptr<StageSocket>& socket);
            virtual bool                                                                    ProcessOpen(std::shared_ptr<StageSocket>& socket);
            virtual void                                                                    ProcessAbort(std::shared_ptr<StageSocket>& socket, bool aborting);
            virtual bool                                                                    ProcessMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message);

        public:
            virtual Error                                                                   ProcessHeartbeat(
                std::shared_ptr<StageSocket>&                                               socket,
                std::shared_ptr<Message>&                                                   message);
            virtual Error                                                                   ProcessAuthentication(
                std::shared_ptr<StageSocket>&                                               socket,
                std::shared_ptr<AuthenticationAAARequest>&                                  request,
                unsigned int                                                                ackNo,
                const StageSocket::CompletedAuthentication&                                 completedAuthentication);

        private:
            bool                                                                            _disposed;
            SyncObjectBlock                                                                 _syncobj;
            std::shared_ptr<StageSocketListener>                                            _communication;
        };
    }
}