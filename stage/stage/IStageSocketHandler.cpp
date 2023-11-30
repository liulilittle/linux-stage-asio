#include "StageSocket.h"
#include "IStageSocketHandler.h"

namespace ep
{
    namespace stage
    {
        Error IStageSocketHandler::ProcessHeartbeat(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
            return model::Error_Success;
        }

        Error IStageSocketHandler::ProcessAuthentication(
            std::shared_ptr<StageSocket>&                               socket, 
            std::shared_ptr<AuthenticationAAARequest>&                  request, 
            unsigned int                                                ackNo, 
            const StageSocket::CompletedAuthentication&                 completedAuthentication) {
            if (completedAuthentication) {
                completedAuthentication(model::Error_Success);
                return model::Error_PendingAuthentication;
            }
            return model::Error_Success;
        }

        IStageSocketHandler::IStageSocketHandler(const std::shared_ptr<StageSocketListener>& communication)
            : _disposed(false)
            , _communication(communication) {
            if (!communication) {
                throw std::runtime_error("The communication layer is not allowed to be an null references");
            }
        }

        IStageSocketHandler::IStageSocketHandler()
            : _disposed(false) {

        }

        IStageSocketHandler::~IStageSocketHandler() {
            Dispose();
        }

        void IStageSocketHandler::Dispose() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_disposed) {
                _disposed = true;
                if (_communication) {
                    _communication.reset();
                }
            }
        }

        std::shared_ptr<IStageSocketHandler> IStageSocketHandler::GetBasePtr() {
            IStageSocketHandler* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<IStageSocketHandler>();
        }

        bool IStageSocketHandler::IsDisposed() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _disposed;
        }

        SyncObjectBlock& IStageSocketHandler::GetSynchronizingObject() {
            return _syncobj;
        }

        std::shared_ptr<StageSocketListener> IStageSocketHandler::GetSocketListener() {
            return _communication;
        }

        void IStageSocketHandler::AbortSocket(const std::shared_ptr<StageSocket>& socket) {
            if (socket) {
                socket->Abort();
            }
        }

        void IStageSocketHandler::CloseSocket(const std::shared_ptr<StageSocket>& socket) {
            if (socket) {
                socket->Close();
            }
        }

        void IStageSocketHandler::ProcessAbort(std::shared_ptr<StageSocket>& socket, bool aborting) {

        }

        bool IStageSocketHandler::ProcessMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
            return true;
        }

        bool IStageSocketHandler::ProcessAccept(std::shared_ptr<StageSocket>& socket) {
            return true;
        }

        bool IStageSocketHandler::ProcessOpen(std::shared_ptr<StageSocket>& socket) {
            return true;
        }
    }
}