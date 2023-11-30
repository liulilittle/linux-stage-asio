#pragma once 

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"

namespace ep
{
    namespace net
    {
        class Message;

        class ISocket : public std::enable_shared_from_this<ISocket>
        {
        public:
            typedef std::function<void(std::shared_ptr<ISocket>& sender, EventArgs& e)>                     AbortEventHandler;
            typedef std::function<void(std::shared_ptr<ISocket>& sender, EventArgs& e)>                     OpenEventHandler;
            typedef std::function<void(std::shared_ptr<ISocket>& sender, EventArgs& e)>                     CloseEventHandler;
            typedef std::function<void(std::shared_ptr<ISocket>& sender, std::shared_ptr<Message>& e)>      MessageEventHandler;
            typedef std::function<void(std::shared_ptr<ISocket>& sender, int error, unsigned int sz)>       SendAsyncCallback;

        public:
            ISocket(const std::shared_ptr<Hosting>& hosting);
            virtual ~ISocket();

        public:
            virtual bool                                                                                    IsDisposed();
            virtual bool                                                                                    Available() = 0;
            virtual std::shared_ptr<ISocket>                                                                GetBasePtr();

        public:
            virtual void                                                                                    Dispose();
            virtual void                                                                                    Abort() = 0;
            virtual void                                                                                    Close() = 0;
            inline bool                                                                                     Send(const Message* message, const SendAsyncCallback& callback) { return message ? Send(*message, callback) : false; }
            virtual bool                                                                                    Send(const Message& message, const SendAsyncCallback& callback) = 0;

        public:
            virtual io_context&                                                                             GetContextObject();
            virtual io_context::work&                                                                       GetContextWorkObject();
            virtual boost::asio::strand&                                                                    GetStrandObject();
            virtual SyncObjectBlock&                                                                        GetSynchronizingObject();
            virtual std::shared_ptr<Hosting>                                                                GetHostingObject();

        public:
            virtual boost::asio::ip::tcp::endpoint                                                          GetLocalEndPoint() = 0;
            virtual boost::asio::ip::tcp::endpoint                                                          GetRemoteEndPoint() = 0;

        protected:
            virtual void                                                                                    OnOpen(EventArgs& e);
            virtual void                                                                                    OnAbort(EventArgs& e);
            virtual void                                                                                    OnClose(EventArgs& e);
            virtual void                                                                                    OnMessage(std::shared_ptr<Message>& e);

        public:
            AbortEventHandler                                                                               AbortEvent;
            OpenEventHandler                                                                                OpenEvent;
            CloseEventHandler                                                                               CloseEvent;
            MessageEventHandler                                                                             MessageEvent;

        private:
            bool                                                                                            _disposed : 1;
            bool                                                                                            _unopenEvent : 7;
            std::shared_ptr<Hosting>                                                                        _hosting;
            SyncObjectBlock                                                                                 _syncobj;
        };
    }
}