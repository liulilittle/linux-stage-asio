#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/threading/Timer.h"

namespace ep
{
    class Message;
    class EventArgs;

    namespace net
    {
        class Socket;
        class ISocket;

        class SocketListener : public std::enable_shared_from_this<SocketListener>
        {
            friend class Socket;

        public:
            typedef std::function<void(std::shared_ptr<SocketListener>& sender, std::shared_ptr<ISocket>& socket, EventArgs& e)>                    AbortEventHandler;
            typedef std::function<void(std::shared_ptr<SocketListener>& sender, std::shared_ptr<ISocket>& socket, EventArgs& e)>                    OpenEventHandler;
            typedef std::function<void(std::shared_ptr<SocketListener>& sender, std::shared_ptr<ISocket>& socket, EventArgs& e)>                    CloseEventHandler;
            typedef std::function<void(std::shared_ptr<SocketListener>& sender, std::shared_ptr<ISocket>& socket, std::shared_ptr<Message>& e)>     MessageEventHandler;

        public:
            SocketListener(const std::shared_ptr<Hosting>& hosting, Byte fk, int backlog, int port);
            virtual ~SocketListener();

        public:
            virtual int                                                                                                                             GetPort();
            void                                                                                                                                    Close();
            virtual void                                                                                                                            Dispose();
            virtual bool                                                                                                                            IsDisposed();
            virtual bool                                                                                                                            IsListening();
            virtual bool                                                                                                                            IsNoDelay(); // TCP_NODELAY(Nagle)
            virtual std::shared_ptr<SocketListener>                                                                                                 GetPtr();
            virtual bool                                                                                                                            Listen();

        public:
            virtual io_context&                                                                                                                     GetContextObject();
            virtual io_context::work&                                                                                                               GetContextWorkObject();
            virtual boost::asio::strand&                                                                                                            GetStrandObject();
            virtual boost::asio::ip::tcp::endpoint                                                                                                  GetLocalEndPoint();
            virtual SyncObjectBlock&                                                                                                                GetSynchronizingObject();
            virtual std::shared_ptr<Hosting>                                                                                                        GetHostingObject();
            virtual int                                                                                                                             GetKf();

        public:
            static bool                                                                                                                             NoDelay(int fd, bool nodelay);
            static bool                                                                                                                             NoDelay(const boost::asio::ip::tcp::socket& socket, bool nodealy);
            static bool                                                                                                                             NoDelay(const boost::asio::ip::tcp::acceptor& socket, bool nodealy);

        public:
            static bool                                                                                                                             Nonblocking(int fd, bool nonblocking);
            static bool                                                                                                                             Closesocket(int fd);
            static bool                                                                                                                             Closesocket(const boost::asio::ip::tcp::socket& socket);
            static bool                                                                                                                             Closesocket(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket);

        protected:
            virtual void                                                                                                                            OnAbort(std::shared_ptr<ISocket>& socket, EventArgs& e);
            virtual void                                                                                                                            OnClose(std::shared_ptr<ISocket>& socket, EventArgs& e);
            virtual void                                                                                                                            OnOpen(std::shared_ptr<ISocket>& socket, EventArgs& e);
            virtual void                                                                                                                            OnMessage(std::shared_ptr<ISocket>& socket, std::shared_ptr<Message>& message);

        private:
            bool                                                                                                                                    StartAccpet();

        protected:
            virtual std::shared_ptr<boost::asio::ip::tcp::socket>                                                                                   NewSocketObject();
            virtual bool                                                                                                                            ProcessAccept(std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
            virtual std::shared_ptr<Socket>                                                                                                         CreateSocket(std::shared_ptr<boost::asio::ip::tcp::socket>& socket);

        public:
            static std::string                                                                                                                      GetHostName();
            static boost::asio::ip::tcp::endpoint                                                                                                   NewAddress(const char* address, int port);
            static boost::asio::ip::tcp::endpoint                                                                                                   AnyAddress(int port);
            static boost::asio::ip::tcp::endpoint                                                                                                   LocalAddress(boost::asio::ip::tcp::resolver& resolver, int port);
            static boost::asio::ip::tcp::endpoint                                                                                                   GetAddressByHostName(boost::asio::ip::tcp::resolver& resolver, const std::string& hostname, int port);
            static bool                                                                                                                             Equals(const boost::asio::ip::tcp::endpoint& x, const boost::asio::ip::tcp::endpoint& y);

        public:
            AbortEventHandler                                                                                                                       AbortEvent;
            OpenEventHandler                                                                                                                        OpenEvent;
            CloseEventHandler                                                                                                                       CloseEvent;
            MessageEventHandler                                                                                                                     MessageEvent;

        private:
            int                                                                                                                                     _port;      // Óû¼àÌýµÄÌ×½Ó×Ö¶Ë¿ÚºÅÂë
            int                                                                                                                                     _backlog;
            Byte                                                                                                                                    _fk;
            bool                                                                                                                                    _listening;
            bool                                                                                                                                    _disposed;
            SyncObjectBlock                                                                                                                         _syncobj;
            std::shared_ptr<Hosting>                                                                                                                _hosting;
            boost::asio::ip::tcp::endpoint                                                                                                          _localEP;
            std::shared_ptr<boost::asio::ip::tcp::acceptor>                                                                                         _server;
            std::shared_ptr<boost::asio::ip::tcp::resolver>                                                                                         _resolver;
        };
    }
}