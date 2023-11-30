#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/net/Socket.h"
#include "util/stage/net/Message.h"
#include "util/stage/threading/Timer.h"
#include "util/stage/stage/model/Inc.h"
#include "StageSocket.h"
#include "StageSocketListener.h"

#include <set>
#include <map>

#ifndef WIN32
#include "util/Util.h"
#endif

// 2E 0C 00 00 00 8E 7E 00 00 00 00 00 0D 27 0D 27 05 00 78 69 79 6f 75
// 2E 00 00 00 00 8F 7E 00 00 00 00

namespace ep
{
    namespace stage
    {
        class StageSocket;
        class StageSocketListener;
        class StageSocketCommunication;
        class ServerSendbackController;

        class StageSocketCommunication : public std::enable_shared_from_this<StageSocketCommunication>
        {
            friend class ClientStageSocketHandler;
            friend class ServerStageSocketHandler;

        public:
            class Server
            {
                friend class StageSocketCommunication;
                friend class ClientStageSocketHandler;
                friend class ServerStageSocketHandler;

            public:
                typedef std::shared_ptr<StageSocket>                                                        SocketObject;
                typedef HASHMAPDEFINE(UInt32, SocketObject)                                                 SocketVector;
                class SocketVectorObject
                {
                    friend class StageSocketCommunication;
                    friend class ClientStageSocketHandler;
                    friend class ServerStageSocketHandler;

                private:
                    SocketVector::iterator                                                                  current;
                    SocketVector                                                                            sockets;
                    SyncObjectBlock                                                                         syncobj;
                    Byte                                                                                    linkType;
                    bool                                                                                    allowAccept;

                public:
                    SocketVectorObject();
                    virtual ~SocketVectorObject();

                public:
                    inline Byte                                                                             GetLinkType() { return linkType; }
                    virtual void                                                                            Clear(bool closing);
                    virtual bool                                                                            Send(const std::shared_ptr<unsigned char>& packet, size_t counts);
                    virtual SyncObjectBlock&                                                                GetSynchronizingObject();
                    virtual void                                                                            Dispose();
                    virtual bool                                                                            RemoveByLinkNo(unsigned int linkNo, std::shared_ptr<StageSocket>& socket);
                    virtual SocketObject                                                                    NextSocket();
                    virtual boost::asio::ip::tcp::endpoint                                                  GetRemoteEndPoint();
                    virtual SocketObject                                                                    FindSocket(unsigned int linkNo);
                };
                typedef HASHMAPDEFINE(Byte, std::shared_ptr<SocketVectorObject>)                            SocketTable;

            private:
                UShort                                                                                      wSvrAreaNo;
                UShort                                                                                      wServerNo;
                UInt32                                                                                      wMaxSocketCount;
                std::string                                                                                 szPlatformName;
                SyncObjectBlock                                                                             syncobj;

            private:
                SocketTable                                                                                 oServerSocket;
                SocketTable                                                                                 oSocketClient;

            public:
                Server();
                virtual ~Server();

            public:
                virtual void                                                                                Clear();
                virtual SyncObjectBlock&                                                                    GetSynchronizingObject();
                virtual void                                                                                Dispose();
                virtual int                                                                                 GetAllLinkType(std::vector<Byte>& s, bool server);
                virtual int                                                                                 GetAllSocketVector(std::vector<std::shared_ptr<SocketVectorObject>/**/>& s, bool server);
                virtual std::shared_ptr<SocketVectorObject>                                                 FindSocketVector(Byte chType, bool server);
                virtual bool                                                                                RemoveByLinkNo(Byte chType, unsigned int linkNo, bool server, std::shared_ptr<SocketVectorObject>& socketVector);
            };
            typedef HASHMAPDEFINE(int, std::shared_ptr<Server>/**/)                                         ServerServerTable;
            typedef HASHMAPDEFINE(std::string, ServerServerTable)                                           PlatformServerTable;

        public:
            StageSocketCommunication(const std::shared_ptr<Hosting>& hosting, int backlog, int clientPort, int serverPort, bool onlyRoutingTransparent = true, bool allowInternalHostAbortCloseAllClient = true);
            virtual ~StageSocketCommunication();

        public:
            inline void                                                                                     Close() { Dispose(); }
            virtual void                                                                                    Dispose();
            virtual bool                                                                                    IsDisposed();
            virtual std::shared_ptr<StageSocketCommunication>                                               GetPtr();
            virtual bool                                                                                    Listen();
            virtual SyncObjectBlock&                                                                        GetSynchronizingObject();

        public:
            virtual std::shared_ptr<StageSocketListener>                                                    GetClientSocketListener();
            virtual std::shared_ptr<StageSocketListener>                                                    GetServerSocketListener();
            virtual bool                                                                                    IsOnlyRoutingTransparent();
            virtual bool                                                                                    IsAllowInternalHostAbortCloseAllClient();
            static bool                                                                                     LoadBulkMany(BulkManyMessage& bulk, const std::shared_ptr<Message>& message);
            virtual std::shared_ptr<ServerSendbackController>                                               GetSendbackController();

        public:
            virtual bool                                                                                    IsEnabledCompressedMessages();
            virtual bool                                                                                    EnabledCompressedMessages();
            virtual bool                                                                                    DisabledCompressedMessages();

        protected:
            virtual std::shared_ptr<IStageSocketHandler>                                                    CreateServerSocketHandler(std::shared_ptr<StageSocketListener>& listener);
            virtual std::shared_ptr<IStageSocketHandler>                                                    CreateClientSocketHandler(std::shared_ptr<StageSocketListener>& listener);
            virtual int                                                                                     GetAllPlatformNames(std::vector<std::string>& s);
            virtual int                                                                                     GetAllServerNo(const std::string& platform, std::vector<int>& s);
            virtual std::shared_ptr<Server>                                                                 GetServerObject(const std::string& platform, int serverNo);
            virtual std::shared_ptr<Server>                                                                 AddServerObject(const std::string& platform, int serverNo, const std::function<bool(std::shared_ptr<Server>&)> predicate);
            virtual unsigned short                                                                          NewLinkNo(const std::string& platform, int serverNo, Byte linkType, bool server);
            virtual std::shared_ptr<ServerSendbackController>                                               CreateSendbackController();

        protected:
            inline std::shared_ptr<Server::SocketVectorObject>                                              GetServerSocketVector(const std::string& platform, int serverNo, Byte linkType)
            {
                std::shared_ptr<Server> server;
                return GetServerSocketVector(platform, serverNo, linkType, server);
            }
            virtual std::shared_ptr<Server::SocketVectorObject>                                             GetServerSocketVector(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server);
            inline std::shared_ptr<Server::SocketVectorObject>                                              GetSocketClientVector(const std::string& platform, int serverNo, Byte linkType)
            {
                std::shared_ptr<Server> server;
                return GetSocketClientVector(platform, serverNo, linkType, server);
            }
            virtual std::shared_ptr<Server::SocketVectorObject>                                             GetSocketClientVector(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server);

        private:
            std::shared_ptr<Server::SocketVectorObject>                                                     GetUnknowSocketVector(bool serverMode, const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server);
            virtual std::shared_ptr<Server>                                                                 AddOrFindServerObject(const std::string& platform, int serverNo, bool adding, std::function<bool(std::shared_ptr<Server>&)> predicate);

        private:
            bool                                                                                            _disposed : 1;
            bool                                                                                            _onlyRoutingTransparent : 1;
            bool                                                                                            _allowInternalHostAbortCloseAllClient : 6;
            std::shared_ptr<Hosting>                                                                        _hosting;
            int                                                                                             _backlog; 
            int                                                                                             _clientPort;
            int                                                                                             _serverPort;
            std::shared_ptr<StageSocketListener>                                                            _clientListener;
            std::shared_ptr<StageSocketListener>                                                            _serverListener;
            SyncObjectBlock                                                                                 _syncobj;
            PlatformServerTable                                                                             _platformServerTable;
            std::atomic<unsigned short>                                                                     _linkNoAtomic;
            std::shared_ptr<ServerSendbackController>                                                       _sendbackController;
        };
    }
}