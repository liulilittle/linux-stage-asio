#include "ISocket.h"

namespace ep
{
    namespace net
    {
        ISocket::ISocket(const std::shared_ptr<Hosting>& hosting)
            : _disposed(false)
            , _unopenEvent(true)
            , _hosting(hosting) {
            if (!hosting) {
                throw std::runtime_error("The Hosting does not allow references to null address identifiers");
            }
        }

        ISocket::~ISocket() {
            Dispose();
        }

        bool ISocket::IsDisposed() {
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            return _disposed;
        }

        std::shared_ptr<ISocket> ISocket::GetBasePtr() {
            ISocket* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<ISocket>();
        }

        void ISocket::Dispose() {
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (!_disposed) {
                _disposed = true;
                if (_hosting) {
                    _hosting.reset();
                }
                AbortEvent = NULL;
                OpenEvent = NULL;
                CloseEvent = NULL;
                MessageEvent = NULL;
            }
        }

        io_context& ISocket::GetContextObject() {
            return _hosting->GetContextObject();
        }

        io_context::work& ISocket::GetContextWorkObject() {
            return _hosting->GetContextWorkObject();
        }

        boost::asio::strand& ISocket::GetStrandObject() {
            return _hosting->GetStrandObject();
        }

        SyncObjectBlock& ISocket::GetSynchronizingObject() {
            return _syncobj;
        }

        std::shared_ptr<Hosting> ISocket::GetHostingObject() {
            return _hosting;
        }

        void ISocket::OnOpen(EventArgs& e) {
            OpenEventHandler events = NULL;
            Object::synchronize(_syncobj, [this, &events] {
                if (_unopenEvent) {
                    _unopenEvent = false;
                    events = OpenEvent;
                }
            });
            if (events) {
                std::shared_ptr<ISocket> self = GetBasePtr();
                events(self, e);
            }
        }

        void ISocket::OnAbort(EventArgs& e) {
            AbortEventHandler events = NULL;
            Object::synchronize(_syncobj, [this, &events] {
                events = AbortEvent;
            });
            if (events) {
                std::shared_ptr<ISocket> self = GetBasePtr();
                events(self, e);
            }
        }

        void ISocket::OnClose(EventArgs& e) {
            CloseEventHandler events = NULL;
            Object::synchronize(_syncobj, [this, &events] {
                events = CloseEvent;
            });
            if (events) {
                std::shared_ptr<ISocket> self = GetBasePtr();
                events(self, e);
            }
        }

        void ISocket::OnMessage(std::shared_ptr<Message>& e) {
            MessageEventHandler events = NULL;
            Object::synchronize(_syncobj, [this, &events] {
                events = MessageEvent;
            });
            if (events) {
                std::shared_ptr<ISocket> self = GetBasePtr();
                events(self, e);
            }
        }
    }
}