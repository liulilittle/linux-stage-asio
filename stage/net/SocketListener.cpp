#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <error.h>
#include <sys/poll.h>
#include <time.h>

#include "Socket.h"
#include "SocketListener.h"

namespace ep
{
    namespace net
    {
        SocketListener::SocketListener(const std::shared_ptr<Hosting>& hosting, Byte fk, int backlog, int port)
            : _port(port)
            , _backlog(0)
            , _fk(fk)
            , _listening(false)
            , _disposed(false)
            , _hosting(hosting) {
            if (port <= 0 || port > 65535) {
                throw std::runtime_error("The listening port must not be less than or equal to 0 and greater than 65535");
            }

            if (!hosting) {
                throw std::runtime_error("The Hosting does not allow references to null address identifiers");
            }

            _server = make_shared_object<boost::asio::ip::tcp::acceptor>(hosting->GetContextObject());
            _resolver = make_shared_object<boost::asio::ip::tcp::resolver>(hosting->GetContextObject());

            if (backlog <= 0) {
                backlog = 1 << 10; // 1024
            }

            // 最大接受队列数量
            _backlog = backlog;

            // 获取当前服务器地址
            _localEP = AnyAddress(port);
        }

        SocketListener::~SocketListener() {
            Dispose();
        }

        std::string SocketListener::GetHostName() {
            char sz[257]; // 域名规定不超过64字节（但是几乎大部分实现为64-1字节）
            if (gethostname(sz, 256) < 0) {
                *(int*)sz = 0x00000000;
            }
            return sz;
        }

        boost::asio::ip::tcp::endpoint
            SocketListener::NewAddress(const char* address, int port) {
            if (!address || *address == '\x0') {
                address = "0.0.0.0";
            }

            if (port < 0 || port > 65535) {
                port = 0;
            }

            boost::asio::ip::tcp::endpoint defaultEP(boost::asio::ip::address::from_string(address), port);
            return defaultEP;
        }

        boost::asio::ip::tcp::endpoint
            SocketListener::AnyAddress(int port) {
            return NewAddress(NULL, port);
        }

        boost::asio::ip::tcp::endpoint
            SocketListener::LocalAddress(boost::asio::ip::tcp::resolver& resolver, int port) {
            std::string hostname = GetHostName();
            return GetAddressByHostName(resolver, hostname, port);
        }

        boost::asio::ip::tcp::endpoint
            SocketListener::GetAddressByHostName(boost::asio::ip::tcp::resolver& resolver, const std::string& hostname, int port) {
            boost::asio::ip::tcp::resolver::query q(hostname.c_str(), std::to_string(port).c_str());
#ifndef WIN32
            boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(q);
            boost::asio::ip::tcp::resolver::iterator l;

            if (i == l) {
                return AnyAddress(port);
            }
#else
            boost::asio::ip::tcp::resolver::results_type results = resolver.resolve(q);
            if (results.empty()) {
                return AnyAddress(port);
            }

            boost::asio::ip::tcp::resolver::iterator i = results.begin();
            boost::asio::ip::tcp::resolver::iterator l = results.end();
#endif
            for (; i != l; ++i) {
                const boost::asio::ip::tcp::endpoint& localEP = *i;
                if (!localEP.
                    address().is_v4()) {
                    continue;
                }

                return localEP;
            }

            return AnyAddress(port);
        }

        bool
            SocketListener::Equals(const boost::asio::ip::tcp::endpoint& x, const boost::asio::ip::tcp::endpoint& y) {
            if (x != y) {
                return false;
            }

            if (x.address() != y.address()) {
                return false;
            }

            return x.port() == y.port();
        }

        int SocketListener::GetPort() {
            return _port;
        }

        void SocketListener::Close() {
            Dispose();
        }

        void SocketListener::Dispose() {
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_disposed) {
                return;
            }

            _disposed = true;
            _listening = false;

            // 撤销并关闭网络套接字接收器
            if (_server) {
                boost::system::error_code ec;
                _server->cancel(ec);
                _server->close(ec);
                _server.reset();
            }

            // 撤销域名解析请求
            if (_resolver) {
                _resolver->cancel();
                _resolver.reset();
            }

            // 清除对于本对象进行监听的事件委托
            AbortEvent = NULL;
            OpenEvent = NULL;
            CloseEvent = NULL;
            MessageEvent = NULL;

            // 释放持有的宿主实例引用指针
            _hosting.reset();
        }

        bool SocketListener::IsDisposed() {
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            return _disposed;
        }

        bool SocketListener::IsListening() {
            return _listening;
        }

        bool SocketListener::IsNoDelay() {
            return true;
        }

        std::shared_ptr<SocketListener> SocketListener::GetPtr() {
            SocketListener* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<SocketListener>();
        }

        bool SocketListener::Listen() {
            std::shared_ptr<SocketListener> self = GetPtr();
            if (!self) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_listening || _disposed) {
                return false;
            }

            if (!_server) {
                return false;
            }

            // 设置端口可重用
            _server->open(_localEP.protocol()/*boost::asio::ip::tcp::v4()*/);
            _server->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

            // 设置为NoDealy
            NoDelay(*_server.get(), IsNoDelay());

            // 绑定本地网卡端口
            _server->bind(_localEP);

            // 侦听套接字客户端
            _server->listen(_backlog);

            // 设置为侦听状态
            _listening = true;

            // 启动套接字接收器
            return StartAccpet();
        }

        io_context& SocketListener::GetContextObject() {
            return _hosting->GetContextObject();
        }

        io_context::work& SocketListener::GetContextWorkObject() {
            return _hosting->GetContextWorkObject();
        }

        boost::asio::strand& SocketListener::GetStrandObject() {
            return _hosting->GetStrandObject();
        }

        boost::asio::ip::tcp::endpoint SocketListener::GetLocalEndPoint() {
            return _localEP;
        }

        SyncObjectBlock& SocketListener::GetSynchronizingObject() {
            return _syncobj;
        }

        std::shared_ptr<Hosting> SocketListener::GetHostingObject() {
            return _hosting;
        }

        int SocketListener::GetKf() {
            return _fk;
        }

        std::shared_ptr<boost::asio::ip::tcp::socket> SocketListener::NewSocketObject() {
            std::shared_ptr<boost::asio::ip::tcp::socket> socket =
                make_shared_object<boost::asio::ip::tcp::socket>(GetContextObject());
            return socket;
        }

        bool SocketListener::Closesocket(int fd) {
            if (fd == ~0 || fd == 0) {
                return false;
            }

#ifdef WIN32
            shutdown(fd, SD_BOTH);
            closesocket(fd);
#else  
            shutdown(fd, SHUT_RDWR);
            close(fd);
#endif
            return true;
        }

        bool SocketListener::Closesocket(const boost::asio::ip::tcp::socket& socket) {
            boost::asio::ip::tcp::socket& s = Object::constcast(socket);
            if (addressof(socket)) {
                return false;
            }

            boost::system::error_code ec;
            try {
                s.cancel(ec);
            }
            catch (std::exception&) {}
            try {
                s.shutdown(boost::asio::socket_base::shutdown_both, ec);
            }
            catch (std::exception&) {}
            try {
                s.close(ec);
            }
            catch (std::exception&) {}
            try {
                int ndfs = s.native_handle();
                if (~0 != ndfs) {
                    Closesocket(ndfs);
                }
            }
            catch (std::exception&) {
                return false;
            }
            return true;
        }

        bool SocketListener::NoDelay(int fd, bool nodelay) {
            if (fd == ~0 || fd == 0) {
                return false;
            }
            int flag = nodelay ? 1 : 0;
            int hr = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
            if (hr < 0) {
                return false;
            }
            return true;
        }

        bool SocketListener::NoDelay(const boost::asio::ip::tcp::acceptor& socket, bool nodealy) {
            boost::asio::ip::tcp::acceptor& s = const_cast<boost::asio::ip::tcp::acceptor&>(socket);
            if (!addressof(s)) {
                return false;
            }

            bool success = false;
            do {
                int ndfs = s.native_handle();
                if (ndfs != ~0 && ndfs != 0) {
                    success = NoDelay(ndfs, nodealy);
                }
            } while (0, 0);

            try {
                s.set_option(boost::asio::ip::tcp::no_delay(nodealy));
                success = true;
            }
            catch (std::exception&) {}
            return success;
        }

        bool SocketListener::NoDelay(const boost::asio::ip::tcp::socket& socket, bool nodealy) {
            boost::asio::ip::tcp::socket& s = const_cast<boost::asio::ip::tcp::socket&>(socket);
            if (!addressof(s)) {
                return false;
            }

            bool success = false;
            do {
                int ndfs = s.native_handle();
                if (ndfs != ~0 && ndfs != 0) {
                    success = NoDelay(ndfs, nodealy);
                }
            } while (0, 0);

            try {
                s.set_option(boost::asio::ip::tcp::no_delay(nodealy));
                success = true;
            }
            catch (std::exception&) {}
            return success;
        }

        void SocketListener::OnOpen(std::shared_ptr<ISocket>& socket, EventArgs& e) {
            OpenEventHandler events = OpenEvent;
            if (events) {
                std::shared_ptr<SocketListener> self = GetPtr();
                events(self, socket, e);
            }
        }

        void SocketListener::OnAbort(std::shared_ptr<ISocket>& socket, EventArgs& e) {
            AbortEventHandler events = AbortEvent;
            if (events) {
                std::shared_ptr<SocketListener> self = GetPtr();
                events(self, socket, e);
            }
        }

        void SocketListener::OnClose(std::shared_ptr<ISocket>& socket, EventArgs& e) {
            CloseEventHandler events = CloseEvent;
            if (events) {
                std::shared_ptr<SocketListener> self = GetPtr();
                events(self, socket, e);
            }
        }

        void SocketListener::OnMessage(std::shared_ptr<ISocket>& socket, std::shared_ptr<Message>& e) {
            MessageEventHandler events = MessageEvent;
            if (events) {
                std::shared_ptr<SocketListener> self = GetPtr();
                events(self, socket, e);
            }
        }

        bool SocketListener::Nonblocking(int fd, bool nonblocking) {
            if (fd == ~0 || fd == 0) {
                return false;
            }

            int flags = fcntl(fd, F_GETFD, 0);
            if (nonblocking) {
                flags |= O_NONBLOCK;
            }
            else {
                flags &= ~O_NONBLOCK;
            }

            if (fcntl(fd, F_SETFL, flags) < 0) {
                return false;
            }

            return true;
        }

        bool SocketListener::Closesocket(const std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
            if (!socket) {
                return false;
            }
            return Closesocket(*socket.get());
        }

        bool SocketListener::StartAccpet() {
            std::shared_ptr<SocketListener> self = GetPtr();
            if (!self) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(self->_syncobj);
            if (self->_disposed || !_server) {
                return false;
            }

            std::shared_ptr<boost::asio::ip::tcp::socket> socket = NewSocketObject();
            if (!socket) {
                throw std::runtime_error("Unable to instantiate a new socket object, may be out of memory or the allocation function has been incorrectly overwritten");
            }

            auto callbackf = GetStrandObject().wrap([self, socket](boost::system::error_code ec) {
                bool success = false;
                do {
                    if (!self) {
                        break;
                    }

                    if (!socket) {
                        break;
                    }

                    if (ec) {
                        break;
                    }

                    if (self->IsDisposed()) {
                        break;
                    }

                    if (!NoDelay(*socket.get(), self->IsNoDelay())) {
                        break;
                    }

                    success = self->ProcessAccept(Object::constcast(socket));
                } while (0, 0);
                if (!success) {
                    Closesocket(socket);
                }
                if (self) {
                    self->StartAccpet();
                }
            });
            try {
                _server->async_accept(*socket.get(), callbackf);
                return true;
            }
            catch (std::exception&) {
                return false;
            }
        }

        bool SocketListener::ProcessAccept(std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
            std::shared_ptr<Socket> client = CreateSocket(socket);
            if (!client) {
                return false;
            }

            std::shared_ptr<SocketListener> self = GetPtr();
            client->OpenEvent = [self](std::shared_ptr<ISocket>& sender, EventArgs& e) {
                if (self) {
                    self->OnOpen(sender, e);
                }
            };
            client->CloseEvent = [self](std::shared_ptr<ISocket>& sender, EventArgs& e) {
                if (self) {
                    self->OnClose(sender, e);
                }
            };
            client->AbortEvent = [self](std::shared_ptr<ISocket>& sender, EventArgs& e) {
                if (self) {
                    self->OnAbort(sender, e);
                }
            };
            client->MessageEvent = [self](std::shared_ptr<ISocket>& sender, std::shared_ptr<Message>& e) {
                if (self) {
                    self->OnMessage(sender, e);
                }
            };
            client->Open(true);
            return true;
        }

        std::shared_ptr<Socket> SocketListener::CreateSocket(std::shared_ptr<boost::asio::ip::tcp::socket>& socket) {
            std::shared_ptr<Socket> client = make_shared_null<Socket>();
            if (this) {
                client = make_shared_object<Socket>(GetHostingObject(), GetKf(), socket);
            }
            return client;
        }
    }
}