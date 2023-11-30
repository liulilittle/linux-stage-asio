#include "StageSocket.h"
#include "StageClient.h"
#include "IStageSocketHandler.h"
#include "StageSocketCommunication.h"
#include "ServerSendbackController.h"

namespace ep
{
    namespace stage
    {
        using ep::net::Message;
        using ep::net::BulkManyMessage;
        using ep::net::TBulkManyMessage;

        class ClientStageSocketHandler : public IStageSocketHandler
        {
        public:
            ClientStageSocketHandler(const std::shared_ptr<StageSocketCommunication>& communication, const std::shared_ptr<StageSocketListener>& listener)
                : IStageSocketHandler(listener)
                , _communication(communication) {

            }
            virtual ~ClientStageSocketHandler() {
                Dispose();
            }

        public:
            virtual void                                                                            Dispose() {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (_communication) {
                    _communication.reset();
                }
                IStageSocketHandler::Dispose();
            }
            virtual bool                                                                            ProcessRoutingTransparent(std::shared_ptr<StageSocketCommunication>& communication, std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> server;
                std::shared_ptr<Server::SocketVectorObject> socketVector = communication->GetServerSocketVector(socket->PlatformName, socket->ServerNo, socket->LinkType, server);
                if (!communication || communication->IsAllowInternalHostAbortCloseAllClient()) {
                    if (!socketVector) {
                        return false;
                    }

                    if (!server) {
                        return false;
                    }
                }
                else {
                    if (!socketVector) {
                        return true;
                    }

                    if (!server) {
                        return true;
                    }
                }

                auto forwarding = [&] {
                    std::shared_ptr<BufferSegment> payload = message->Payload;
                    unsigned int payload_size = 0;
                    if (payload) {
                        payload_size = payload->Length;
                    }

                    unsigned int packet_size = 0;
                    unsigned short link_pid = socket->LinkNo;
                    unsigned char* payload_data = payload->UnsafeAddrOfPinnedArray();

                    std::shared_ptr<unsigned char> packet = BulkManyMessage::ToPacket(
                        StageSocket::ServerKf,
                        message->CommandId,
                        message->SequenceNo,
                        socket->LinkType,
                        &link_pid,
                        1,
                        payload_data,
                        payload_size,
                        packet_size);
                    if (!packet) {
                        return false;
                    }

                    return socketVector->Send(packet, packet_size);
                };
                Object::synchronize<bool>(socketVector->syncobj, forwarding);
                return true;
            }
            virtual std::shared_ptr<StageSocketCommunication>                                       GetSocketCommunication() {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                return _communication;
            }
            virtual void                                                                            ProcessAbort(std::shared_ptr<StageSocket>& socket, bool aborting) {
                if (ResourcesCleanup(socket, false, NULL)) {
                    SendAbortClient(GetServerObject(socket->PlatformName, socket->ServerNo), socket->LinkType, socket->LinkNo);
                }
            }
            virtual Error                                                                           ProcessAuthentication(
                std::shared_ptr<StageSocket>&                                                       socket,
                std::shared_ptr<AuthenticationAAARequest>&                                          request,
                unsigned int                                                                        ackNo,
                const StageSocket::CompletedAuthentication&                                         completedAuthentication) {
                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return model::Error_TheObjectHasBeenFreed;
                }

                std::string platform = request->GetPlatform();
                int serverNo = request->GetServerNo();
                int svrAreaNo = request->GetSvrAreaNo();
                Byte chLinkType = request->GetLinkType();

                typedef StageSocketCommunication::Server Server;

                // 基础的套接字鉴权前置检查
                Error error = model::Error_Success;
                std::shared_ptr<Server> server = NULL;
                if ((error = Object::synchronize<Error>(communication->GetSynchronizingObject(), [&] {
                    if (communication->IsDisposed()) {
                        return model::Error_TheObjectHasBeenFreed;
                    }

                    server = communication->GetServerObject(platform, serverNo);
                    if (!server) {
                        return model::Error_UnableToFindThisServer;
                    }

                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                // 检查当前套接字提供的鉴权请求的有效性
                unsigned short linkNo = 0;
                std::shared_ptr<Server::SocketVectorObject> serverVector = make_shared_null<Server::SocketVectorObject>();
                std::shared_ptr<Server::SocketVectorObject> socketVector = make_shared_null<Server::SocketVectorObject>();
                if ((error = Object::synchronize<Error>(server->syncobj, [&]() {
                    if (server->wSvrAreaNo != svrAreaNo) {
                        return model::Error_SvrAreaNoItIsIllegal;
                    }

                    if (server->oServerSocket.empty()) {
                        return model::Error_NoLinkTypeIsHostedOnTheStage;
                    }

                    serverVector = server->oServerSocket[chLinkType];
                    if (!serverVector) {
                        return model::Error_UnableTryGetValueSocketVectorObject;
                    }

                    if (serverVector->sockets.empty()) {
                        return model::Error_TheInternalServerIsNotOnline;
                    }

                    if (!serverVector->allowAccept) {
                        return model::Error_InternalServerActivelyRefused; // 内部服务器积极拒绝
                    }

                    linkNo = communication->NewLinkNo(platform, serverNo, chLinkType, false);
                    if (0 == linkNo) {
                        return model::Error_TheLinkNoCouldNotBeAssigned;
                    }

                    socketVector = server->oSocketClient[chLinkType];
                    if (!socketVector) {
                        socketVector = make_shared_object<Server::SocketVectorObject>();
                        if (!socketVector) {
                            return model::Error_UnableToAllocSocketVectorObject;
                        }
                        socketVector->linkType = chLinkType;
                        server->oSocketClient[chLinkType] = socketVector;
                    }
                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                // 对当前套接字实例托管的数据进行标记部署
                if ((error = Object::synchronize<Error>(socketVector->syncobj, [&] {
                    if (0 == server->wMaxSocketCount ||
                        socketVector->sockets.size() >= server->wMaxSocketCount) {
                        return model::Error_LimitTheNumberOfInternalServerClients;
                    }

                    socket->LinkNo = linkNo;
                    socket->LinkType = chLinkType;
                    socket->ServerNo = serverNo;
                    socket->SvrAreaNo = svrAreaNo;
                    socket->PlatformName = platform;
                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                typedef StageClient::AcceptClient AcceptClient;

                // 异步投递客户端建立链接请求到内部服务器通信层
                boost::asio::ip::tcp::endpoint ep = socket->GetRemoteEndPoint();
                AcceptClient ac;
                ac.Address  = Socket::GetAddressV4(ep);
                ac.Port     = Socket::GetPort(ep);
                ac.LinkNo   = linkNo;

                unsigned int counts = 0;
                std::shared_ptr<unsigned char> packet = TBulkManyMessage<AcceptClient>::ToPacket(
                    StageSocket::ServerKf,
                    model::Commands_AcceptClient,
                    Message::NewId(), 
                    chLinkType,
                    &ac,
                    1,
                    NULL,
                    0,
                    counts);
                if (0 == counts || !packet) {
                    return model::Error_SeriousServerInternalError;
                }

                if (!serverVector->Send(packet, counts)) {
                    return model::Error_UnableToPostEstablishLinkToInternalServer;
                }

                // 当前套接字鉴权请求正确完成，服务器对其实例进行部署托管
                return (error = Object::synchronize<Error>(socketVector->syncobj, [&] {
                    unsigned int dwCurrentKey = 0;

                    // 修饰迭代器，抗迭代器损坏
                    Server::SocketVector::iterator& rl = socketVector->current;
                    Server::SocketVector::iterator el = socketVector->sockets.end(); // 抗当前迭代器被破坏
                    if (socketVector->sockets.empty()) {
                        rl = el;
                    }
                    else if (rl != el) {
                        dwCurrentKey = rl->first; // 保存当前迭代器的命中键值
                    }

                    // 修改或者插入映射关系（可能破坏当前迭代器）
                    socketVector->sockets[linkNo] = socket;

                    // 重新修饰正确的迭代器（散列重构BUCKET与元素时可能造成当前迭代器双向链表破损，通常这只会发生在插入与删除键值的时候）
                    if (0 == dwCurrentKey) {
                        rl = el;
                    }
                    else {
                        rl = socketVector->sockets.find(dwCurrentKey);
                    }
                    return model::Error_Success;
                }));
            }
            virtual bool                                                                            ProcessMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return false;
                }

                if (communication->IsDisposed()) {
                    return false;
                }

                return ProcessRoutingTransparent(communication, socket, message);
            }

        protected:
            virtual std::shared_ptr<StageSocketCommunication::Server>                               GetServerObject(const std::string platform, int serverNo) {
                auto communication = GetSocketCommunication();
                if (!communication) {
                    return false;
                }

                if (communication->IsDisposed()) {
                    return false;
                }

                return communication->GetServerObject(platform, serverNo);
            }
            virtual bool                                                                            SendAbortClient(const std::shared_ptr<StageSocketCommunication::Server>& server, Byte linkType, unsigned short linkNo) {
                if (!server) {
                    return false;
                }

                auto serverVector = server->FindSocketVector(linkType, true);
                if (!serverVector) {
                    return false;
                }

                // 异步投递客户端关闭链接请求到内部服务器通信层
                unsigned int counts = 0;
                std::shared_ptr<unsigned char> packet = BulkManyMessage::ToPacket(
                    StageSocket::ServerKf,
                    model::Commands_AbortClient,
                    Message::NewId(),
                    linkType,
                    &linkNo,
                    1,
                    NULL,
                    0, 
                    counts);
                if (0 == counts || !packet) {
                    return false;
                }

                return serverVector->Send(packet, counts);
            }
            inline bool                                                                             ResourcesCleanup(
                std::shared_ptr<StageSocket>&                                                       socket,
                bool                                                                                server,
                bool*                                                                               cleanAll) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> servero;
                return ResourcesCleanup(socket, server, cleanAll, servero);
            }
            inline bool                                                                             ResourcesCleanup(
                std::shared_ptr<StageSocket>&                                                       socket,
                bool                                                                                server,
                bool*                                                                               cleanAll,
                std::shared_ptr<StageSocketCommunication::Server>&                                  servero) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server::SocketVectorObject> socketVector;
                return ResourcesCleanup(socket, server, cleanAll, servero, socketVector);
            }
            virtual bool                                                                            ResourcesCleanup(
                std::shared_ptr<StageSocket>&                                                       socket,
                bool                                                                                server, 
                bool*                                                                               cleanAll,
                std::shared_ptr<StageSocketCommunication::Server>&                                  servero,
                std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject>&              socketVector) {
                typedef StageSocketCommunication::Server Server;

                servero = GetServerObject(socket->PlatformName, socket->ServerNo);
                if (!servero) {
                    return false;
                }
                socketVector = make_shared_null<Server::SocketVectorObject>();
                if (!servero->RemoveByLinkNo(socket->LinkType, socket->LinkNo, server, socketVector)) {
                    return false;
                }
                else {
                    bool deletedAll = Object::synchronize<bool>(socketVector->syncobj, [&] {
                        bool releasedAll = socketVector->sockets.empty();
                        if (releasedAll) {
                            socketVector->allowAccept = false;
                        }
                        return releasedAll;
                    });
                    if (cleanAll) {
                        *cleanAll = deletedAll;
                    }
                }
                return true;
            }
            virtual bool                                                                            SendAllAcceptClient(
                const std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject>&        sender, 
                const std::shared_ptr<StageSocketCommunication::Server>&                            servero, 
                Byte                                                                                linkType,
                bool                                                                                server) {
                typedef StageSocketCommunication::Server Server;

                if (!servero) {
                    return false;
                }

                if (!sender) {
                    return false;
                }

                auto po = Object::addressof(servero->oSocketClient);
                if (server) {
                    po = Object::addressof(servero->oServerSocket);
                }

                std::shared_ptr<Server::SocketVectorObject> socketVector = make_shared_null<Server::SocketVectorObject>();
                if (!Object::synchronize<bool>(servero->syncobj, [&] {
                    auto il = po->find(linkType);
                    auto el = po->end();
                    if (il == el) {
                        return false;
                    }

                    socketVector = il->second;
                    return NULL != socketVector.get();
                })) {
                    return true;
                }

                typedef StageClient::AcceptClient AcceptClient;

                std::vector<AcceptClient> acVector;
                Object::synchronize(socketVector->syncobj, [&] {
                    auto il = socketVector->sockets.begin();
                    auto el = socketVector->sockets.end();
                    for (; il != el; ++il) {
                        Server::SocketObject socket = il->second;
                        if (!socket) {
                            continue;
                        }

                        AcceptClient ac;
                        boost::asio::ip::tcp::endpoint ep = socket->GetRemoteEndPoint();
                        ac.Address      = Socket::GetAddressV4(ep);
                        ac.Port         = Socket::GetPort(ep);
                        ac.LinkNo       = socket->LinkNo;
                        acVector.push_back(ac);
                    }
                });

                // 异步投递客户端建立链接请求到内部服务器通信层
                unsigned int counts = 0;
                AcceptClient* pac = NULL;
                if (!acVector.empty()) {
                    pac = &(acVector[0]);
                }

                std::shared_ptr<unsigned char> packet = TBulkManyMessage<AcceptClient>::ToPacket(StageSocket::ServerKf,
                    model::Commands_AcceptClient,
                    Message::NewId(),
                    linkType,
                    pac, 
                    acVector.size(),
                    NULL,
                    0,
                    counts);
                if (0 == counts || !packet) {
                    return false;
                }

                return sender->Send(packet, counts);
            }

        private:
            std::shared_ptr<StageSocketCommunication>                                               _communication;
        };

        class ServerStageSocketHandler : public ClientStageSocketHandler
        {
        public:
            ServerStageSocketHandler(const std::shared_ptr<StageSocketCommunication>& communication, const std::shared_ptr<StageSocketListener>& listener)
                : ClientStageSocketHandler(communication, listener) {

            }
            virtual ~ServerStageSocketHandler() {
                Dispose();
            }

        public:
            virtual bool                                                                            CloseAllClient(const std::string& platform, int serverNo, Byte linkType) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> servero = GetServerObject(platform, serverNo);
                if (!servero) {
                    return false;
                }
                std::shared_ptr<Server::SocketVectorObject> socketVector = servero->FindSocketVector(linkType, false);
                if (!socketVector) {
                    return false;
                }
                else {
                    socketVector->Clear(true);
                }
                return true;
            }
            virtual void                                                                            ProcessAbort(std::shared_ptr<StageSocket>& socket, bool aborting) {
                bool cleanAll = false;
                if (ResourcesCleanup(socket, true, &cleanAll) && cleanAll) {
                    std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                    if (!communication || communication->IsAllowInternalHostAbortCloseAllClient()) {
                        CloseAllClient(socket->PlatformName, socket->ServerNo, socket->LinkType);
                    }

                    if (communication) {
                        std::shared_ptr<ServerSendbackController> controller = communication->GetSendbackController();
                        if (controller) {
                            controller->On(ServerSendbackController::kOffline, socket->PlatformName, socket->ServerNo, socket->LinkType, NULL);
                        }
                    }

                    Transitroute transitroute;
                    transitroute.CommandId = model::Commands_AbortClient;
                    transitroute.LinkType = socket->LinkType;
                    transitroute.Platform = socket->PlatformName;
                    transitroute.ServerNo = socket->ServerNo;
                    transitroute.SequenceNo = Message::NewId();
                    PushToAllServer(transitroute);
                }
            }
            virtual Error                                                                           ProcessAuthentication(
                std::shared_ptr<StageSocket>&                                                       socket,
                std::shared_ptr<AuthenticationAAARequest>&                                          request,
                unsigned int                                                                        ackNo,
                const StageSocket::CompletedAuthentication&                                         completedAuthentication) {
                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return model::Error_TheObjectHasBeenFreed;
                }

                std::string platform = request->GetPlatform();
                int serverNo = request->GetServerNo();
                int svrAreaNo = request->GetSvrAreaNo();
                int maxFD = request->GetMaxFD();
                Byte chLinkType = request->GetLinkType();

                if (maxFD < StageSocketBuilder::MIN_FILEDESCRIPTOR) {
                    maxFD = StageSocketBuilder::MIN_FILEDESCRIPTOR;
                }
                else if (maxFD > StageSocketBuilder::MAX_FILEDESCRIPTOR) {
                    maxFD = StageSocketBuilder::MAX_FILEDESCRIPTOR;
                }

                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> server = NULL;
                Error error = model::Error_Success;

                if ((error = Object::synchronize<Error>(communication->GetSynchronizingObject(), [&] {
                    if (communication->IsDisposed()) {
                        return model::Error_TheObjectHasBeenFreed;
                    }
                    server = communication->AddServerObject(platform, serverNo, [&](std::shared_ptr<Server>& server) {
                        server->wSvrAreaNo = svrAreaNo;
                        server->wServerNo = serverNo;
                        server->szPlatformName = platform;
                        server->wMaxSocketCount = maxFD;
                        return true;
                    });
                    if (!server) {
                        return model::Error_UnableToAllocServerObject;
                    }
                    else {
                        server->wMaxSocketCount = maxFD; // 允许动态设置最大文件描述符数量(不允许小于MIN_FILEDESCRIPTOR且大于MAX_FILEDESCRIPTOR)
                    }
                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                unsigned short linkNo = 0;
                std::shared_ptr<Server::SocketVectorObject> serverVector = make_shared_null<Server::SocketVectorObject>();
                if ((error = Object::synchronize<Error>(server->syncobj, [&]() {
                    linkNo = communication->NewLinkNo(platform, serverNo, chLinkType, true);
                    if (0 == linkNo) {
                        return model::Error_TheLinkNoCouldNotBeAssigned;
                    }

                    serverVector = server->oServerSocket[chLinkType];
                    if (!serverVector) {
                        serverVector = make_shared_object<Server::SocketVectorObject>();
                        if (!serverVector) {
                            return model::Error_UnableToAllocSocketVectorObject;
                        }
                        serverVector->linkType = chLinkType;
                        server->oServerSocket[chLinkType] = serverVector;
                    }

                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                if (socket) { // 分离此部分赋值的计算在外部预防超限与额外增加临界开销的问题
                    socket->LinkNo = linkNo; // 标记鉴权成功的套接字实例设定
                    socket->LinkType = chLinkType;
                    socket->ServerNo = serverNo;
                    socket->SvrAreaNo = svrAreaNo;
                    socket->PlatformName = platform;
                }

                bool pushAllClient = false;
                if ((error = Object::synchronize<Error>(serverVector->syncobj, [&] { // 临界
                    // 保存当前迭代器命中键值的变量
                    unsigned int dwCurrentKey = 0;

                    // 修饰迭代器，抗迭代器损坏
                    Server::SocketVector::iterator& rl = serverVector->current;
                    Server::SocketVector::iterator el = serverVector->sockets.end(); // 抗当前迭代器被破坏
                    if (serverVector->sockets.empty()) {
                        rl = el;
                    }
                    else if (rl != el) {
                        dwCurrentKey = rl->first; // 保存当前迭代器的命中键值
                    }

                    // 修改或者插入映射关系（可能破坏当前迭代器）
                    serverVector->sockets[linkNo] = socket;

                    // 重新修饰正确的迭代器（散列重构BUCKET与元素时可能造成当前迭代器双向链表破损，通常这只会发生在插入与删除键值的时候）
                    if (0 == dwCurrentKey) {
                        rl = el;
                    }
                    else {
                        rl = serverVector->sockets.find(dwCurrentKey);
                    }

                    // 需要向内部服务器推送持有全部客户端
                    pushAllClient = serverVector->sockets.size() <= 1;

                    // 当前匿名函数处理成功返回控制位状态
                    return model::Error_Success;
                })) != model::Error_Success) {
                    return error;
                }

                // 向内部服务器推送持有的全部客户端实例
                if (!pushAllClient) {
                    return model::Error_Success;
                }

                if (completedAuthentication) {
                    completedAuthentication(model::Error_Success);
                }

                if (!SendAllAcceptClient(serverVector, server, chLinkType, false)) {
                    return model::Error_UnableToPushAllAcceptClient;
                }

                error = PushAllAcceptServer(socket);
                if (error != model::Error_Success) {
                    return error;
                }
                return model::Error_PendingAuthentication;
            }
            virtual bool                                                                            ProcessMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return false;
                }

                if (communication->IsDisposed()) {
                    return false;
                }

                Commands commandId = (Commands)message->CommandId;
                switch (commandId)
                {
                case model::Commands_AbortClient:                                       // 关闭客户端实例
                    return ProcessAbortClient(communication, socket, message);
                case model::Commands_AcceptClient:                                      // 接受客户端指令
                    return true;
                case model::Commands_PermittedAccept:                                   // 允许接受客户端
                    return MarkAllowAccept(communication, socket, false);
                case model::Commands_PreventAccept:                                     // 阻止接受客户端
                    return MarkAllowAccept(communication, socket, true);
                case model::Commands_Transitroute:
                    return ProcessTransitroute(communication, socket, message);         // 服务器交换路由
                case model::Commands_AbortAllClient:
                    return CloseAllClient(socket->PlatformName, socket->ServerNo, socket->LinkType);
                default:
                    return ProcessRoutingTransparent(communication, socket, message);   // 客户端交换路由
                }
            }

        protected:
            virtual bool                                                                            IsAllowAccept(const std::string& platform, int serverNo, Byte linkType) {
                return IsOrMarkAllowAccept(platform, serverNo, linkType, false, false);
            }
            virtual bool                                                                            MarkAllowAccept(
                std::shared_ptr<StageSocketCommunication>&                                          communication, 
                std::shared_ptr<StageSocket>&                                                       socket, 
                bool                                                                                preventAccept) {
                return IsOrMarkAllowAccept(socket->PlatformName, socket->ServerNo, socket->LinkType, preventAccept, true);
            }

        private:
            inline bool                                                                             CreateAcceptServer(
                Transitroute&                                                                       transitroute,
                std::shared_ptr<StageSocket>&                                                       socket,
                std::shared_ptr<StageSocketCommunication::Server>&                                  server,
                std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject>&              socketVector,
                int                                                                                 op) {
                typedef StageClient::AcceptClient AcceptClient;

                if (0 == op) {
                    if (!socket) {
                        return false;
                    }
                }
                else if (1 == op) {
                    if (!server) {
                        return false;
                    }

                    if (!socketVector) {
                        return false;
                    }
                }
                else {
                    return false;
                }

                transitroute.CommandId = model::Commands_AcceptClient;
                if (0 == op) {
                    transitroute.LinkType = socket->LinkType;
                    transitroute.Platform = socket->PlatformName;
                    transitroute.ServerNo = socket->ServerNo;
                }
                else {
                    transitroute.LinkType = socketVector->linkType;
                    transitroute.Platform = server->szPlatformName;
                    transitroute.ServerNo = server->wServerNo;
                }
                transitroute.SequenceNo = Message::NewId();
                transitroute.Payload = make_shared_object<BufferSegment>(make_shared_array<unsigned char>(sizeof(AcceptClient)), 0, sizeof(AcceptClient));

                if (transitroute.Payload) {
                    AcceptClient* pac = (AcceptClient*)transitroute.Payload->UnsafeAddrOfPinnedArray();
                    if (NULL == pac) {
                        transitroute.Payload->Clear();
                    }
                    else {
                        boost::asio::ip::tcp::endpoint ep = socket ? socket->GetRemoteEndPoint() : socketVector->GetRemoteEndPoint();
                        pac->Port = Socket::GetPort(ep);
                        pac->Address = Socket::GetAddressV4(ep);
                        pac->LinkNo = socket ? socket->LinkNo : socketVector->linkType;
                    }
                }
                return true;
            }
            inline bool                                                                             IsOrMarkAllowAccept(const std::string& platform, int serverNo, Byte linkType, bool preventAccept, bool markOp) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> servero = GetServerObject(platform, serverNo);
                if (!servero) {
                    return false;
                }

                std::shared_ptr<Server::SocketVectorObject> socketVector = servero->FindSocketVector(linkType, true);
                if (!socketVector) {
                    return false;
                }

                return Object::synchronize<bool>(socketVector->syncobj, [&] {
                    if (markOp) {
                        socketVector->allowAccept = preventAccept ? false : true;
                        return true;
                    }
                    return socketVector->allowAccept;
                });
            }

        protected:
            virtual bool                                                                            PushToServer(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<unsigned char>& packet, size_t counts) {
                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<Server> servero = GetServerObject(platform, serverNo);
                if (!servero) {
                    return false;
                }

                std::shared_ptr<Server::SocketVectorObject> socketVector = servero->FindSocketVector(linkType, true);
                if (!socketVector) {
                    return false;
                }
                return socketVector->Send(packet, counts);
            }
            virtual int                                                                             PushToServer(const std::string& platform, int serverNo, Byte linkType, Transitroute& transitroute) {
                unsigned int counts = 0;
                std::shared_ptr<unsigned char> packet = transitroute.ToPacket(StageSocket::ServerKf, model::Commands_Transitroute, Message::NewId(), counts);
                if (!packet || 0 == counts) {
                    return false;
                }
                return PushToServer(platform, serverNo, linkType, packet, counts);
            }
            virtual int                                                                             PredicateAllServer(
                const std::function<bool(std::shared_ptr<StageSocketCommunication::Server>& server, 
                    std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject>& socketVector)>& predicate) {

                typedef StageSocketCommunication::Server Server;

                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return ~0;
                }

                if (communication->IsDisposed()) {
                    return ~1;
                }

                if (!predicate) {
                    return ~2;
                }

                std::vector<int> serverNoVector;
                std::vector<std::string> platformVector;
                std::vector<std::shared_ptr<Server::SocketVectorObject>/**/> socketVectorVector;

                int events = 0;
                int platformVectorCount = communication->GetAllPlatformNames(platformVector);
                for (int platformVectorIndex = 0; platformVectorIndex < platformVectorCount; platformVectorIndex++) {
                    std::string& platform = Object::constcast(platformVector[platformVectorIndex]);

                    int serverNoVectorCount = communication->GetAllServerNo(platform, serverNoVector);
                    for (int serverNoVectorIndex = 0; serverNoVectorIndex < serverNoVectorCount; serverNoVectorIndex++) {
                        int serverNo = serverNoVector[serverNoVectorIndex];

                        std::shared_ptr<Server> server = GetServerObject(platform, serverNo);
                        if (!server) {
                            continue;
                        }

                        int socketVectorVectorCount = server->GetAllSocketVector(socketVectorVector, true);
                        for (int socketVectorVectorIndex = 0; socketVectorVectorIndex < socketVectorVectorCount; socketVectorVectorIndex++) {
                            std::shared_ptr<Server::SocketVectorObject> socketVector = socketVectorVector[socketVectorVectorIndex];
                            if (!socketVector) {
                                continue;
                            }

                            if (predicate(server, socketVector)) {
                                events++;
                            }
                        }
                    }
                }
                return events;
            }
            virtual int                                                                             PushToAllServer(const std::shared_ptr<unsigned char>& packet, size_t counts) {
                typedef StageSocketCommunication::Server Server;
                typedef StageSocketCommunication::Server::SocketVectorObject SocketVectorObject;

                return PredicateAllServer([&](std::shared_ptr<Server>& server, std::shared_ptr<SocketVectorObject>& socketVector) {
                    return socketVector->Send(packet, counts);
                });
            }
            virtual int                                                                             PushToAllServer(Transitroute& transitroute) {
                unsigned int counts = 0;
                std::shared_ptr<unsigned char> packet = transitroute.ToPacket(StageSocket::ServerKf, model::Commands_Transitroute, Message::NewId(), counts);
                if (!packet || 0 == counts) {
                    return ~0;
                }
                return PushToAllServer(packet, counts);
            }
            virtual bool                                                                            CreateAcceptServer(Transitroute& transitroute, std::shared_ptr<StageSocket>& socket) {
                std::shared_ptr<StageSocketCommunication::Server> server;
                std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject> socketVector;
                return CreateAcceptServer(transitroute, socket, server, socketVector, 0);
            }
            virtual bool                                                                            CreateAcceptServer(
                Transitroute&                                                                       transitroute,
                std::shared_ptr<StageSocketCommunication::Server>&                                  server,
                std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject>&              socketVector) {
                std::shared_ptr<StageSocket> socket;
                return CreateAcceptServer(transitroute, socket, server, socketVector, 1);
            }
            inline Error                                                                            PushAllAcceptServer(std::shared_ptr<StageSocket>& socket) {
                typedef StageSocketCommunication::Server Server;
                typedef StageSocketCommunication::Server::SocketVectorObject SocketVectorObject;

                int events = 0;
                if (!socket) {
                    return model::Error_NotAllowedToProvideSocketNullReferences;
                }

                std::shared_ptr<StageSocketCommunication> communication = GetSocketCommunication();
                if (!communication) {
                    return model::Error_UnableToGetSocketCommunication;
                }

                // ROOT
                do {
                    Transitroute transitroute;
                    if (!CreateAcceptServer(transitroute, socket)) {
                        return model::Error_UnableToCreateAcceptServerIL;
                    }

                    events += PushToAllServer(transitroute);
                    if (events < 0) {
                        return model::Error_UnableToPushToAllServers;
                    }

#ifdef EPDEBUG
                    printf("PushToAllServer: CommandId=%d, Platform=%s, ServerNo=%d, LinkType=%d\n",
                        transitroute.CommandId,
                        transitroute.Platform.data(),
                        transitroute.ServerNo,
                        transitroute.LinkType);
#endif
                } while (0, 0);

                // PUSH ALL
                events += PredicateAllServer([&](std::shared_ptr<Server>& server, std::shared_ptr<SocketVectorObject>& socketVector) {
                    Transitroute transitroute;
                    if (!CreateAcceptServer(transitroute, server, socketVector)) {
                        return false;
                    }

                    unsigned int counts = 0;
                    std::shared_ptr<unsigned char> packet = transitroute.ToPacket(StageSocket::ServerKf, model::Commands_Transitroute, Message::NewId(), counts);
                    if (!packet || 0 == counts) {
                        return false;
                    }

#ifdef EPDEBUG
                    printf("PredicateAllServer: CommandId=%d, Platform=%s, ServerNo=%d, LinkType=%d\n",
                        transitroute.CommandId,
                        transitroute.Platform.data(),
                        transitroute.ServerNo,
                        transitroute.LinkType);
#endif
                    return socket->Send(packet, counts, NULL);
                });
                if (events < 0) {
                    return model::Error_UnableToPushToAllServers;
                }

                // SEND BACK
                std::shared_ptr<ServerSendbackController> sendbackController = communication->GetSendbackController();
                if (!sendbackController) {
                    return model::Error_UnableToGetSendbackController;
                }
                else {
                    events += sendbackController->On(ServerSendbackController::kOnline, socket->PlatformName, socket->ServerNo, socket->LinkType,
                        [&](std::shared_ptr<unsigned char>& packet, size_t packetSize) {
                        return socket->Send(packet, packetSize, NULL);
                    });
                }

                return model::Error_Success;
            }

        protected:
            virtual bool                                                                            LoadBulkMany(BulkManyMessage& bulk, const std::shared_ptr<Message>& message) {
                return StageSocketCommunication::LoadBulkMany(bulk, message);
            }
            inline std::shared_ptr<StageSocket>                                                     PredicateBulkMany(
                std::shared_ptr<StageSocketCommunication>&                                          communication,
                const std::string&                                                                  platform,
                int                                                                                 serverNo,
                const std::function<bool(BulkManyMessage&, std::shared_ptr<StageSocket>&)>&         predicate,
                const std::shared_ptr<Message>&                                                     message) {

                BulkManyMessage bulk;
                if (!LoadBulkMany(bulk, message)) {
                    return false;
                }

                return PredicateBulkMany(bulk, communication, platform, serverNo, bulk.MemberType, predicate);
            }
            inline std::shared_ptr<StageSocket>                                                     PredicateBulkMany(
                std::shared_ptr<StageSocketCommunication>&                                          communication,
                const std::string&                                                                  platform,
                int                                                                                 serverNo,
                Byte                                                                                linkType,
                const std::function<bool(BulkManyMessage&, std::shared_ptr<StageSocket>&)>&         predicate,
                const std::shared_ptr<Message>&                                                     message) {

                BulkManyMessage bulk;
                if (!LoadBulkMany(bulk, message)) {
                    return false;
                }

                return PredicateBulkMany(bulk, communication, platform, serverNo, linkType, predicate);
            }
            virtual std::shared_ptr<StageSocket>                                                    PredicateBulkMany(
                BulkManyMessage&                                                                    bulk, 
                std::shared_ptr<StageSocketCommunication>&                                          communication, 
                const std::string&                                                                  platform,
                int                                                                                 serverNo,
                Byte                                                                                linkType,
                const std::function<bool(BulkManyMessage&, std::shared_ptr<StageSocket>&)>&         predicate) {
                typedef StageSocketCommunication::Server Server;

                if (!predicate) {
                    return make_shared_null<StageSocket>();
                }

                if (bulk.MemberCount != 0 && NULL == bulk.Member) {
                    return make_shared_null<StageSocket>();
                }

                if (bulk.MemberCount > 0) {
                    std::shared_ptr<Server> server;
                    auto socketVector = communication->GetSocketClientVector(platform, serverNo, linkType, server);
                    if (!socketVector || !server) {
                        return make_shared_null<StageSocket>();
                    }

                    for (int i = 0; i < bulk.MemberCount; i++) {
                        unsigned int linkNo = bulk.Member[i];

                        auto sockets = socketVector->FindSocket(linkNo);
                        if (!sockets) {
                            continue;
                        }

                        if (predicate(bulk, sockets)) {
                            return sockets;
                        }
                    }
                }
                return make_shared_null<StageSocket>();
            }

        protected:
            virtual bool                                                                            ProcessAbortClient(std::shared_ptr<StageSocketCommunication>& communication, std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message) {
                PredicateBulkMany(communication, socket->PlatformName, socket->ServerNo, socket->LinkType, [&](BulkManyMessage& bulk, std::shared_ptr<StageSocket>& client) {
                    client->Close();
                    client->Dispose();
                    return false;
                }, message);
                return true;
            }
            virtual bool                                                                            ProcessRoutingTransparent(
                std::shared_ptr<StageSocketCommunication>&                                          communication, 
                std::shared_ptr<StageSocket>&                                                       socket,
                std::shared_ptr<Message>&                                                           message) {
                PredicateBulkMany(communication, socket->PlatformName, socket->ServerNo, [&](BulkManyMessage& bulk, std::shared_ptr<StageSocket>& client) {
                    size_t packet_size = 0;
                    std::shared_ptr<unsigned char> packet_data = client->ToPacket(bulk.CommandId, bulk.SequenceNo, bulk.PayloadPtr, bulk.PayloadSize, packet_size);
                    if (!packet_data || 0 == packet_data) {
                        return false;
                    }

                    client->Send(packet_data, packet_size, NULL); // 每次都发送一个完整的数据报文（含对报文的各种处理，例如压缩加密等）
                    return false;
                }, message);
                return true;
            }
            virtual bool                                                                            ProcessTransitroute(
                std::shared_ptr<StageSocketCommunication>&                                          communication,
                std::shared_ptr<StageSocket>&                                                       socket,
                std::shared_ptr<Message>&                                                           message) {
                typedef StageSocketCommunication::Server Server;

                if (!communication) {
                    return false;
                }

                Transitroute transitroute;
                if (!transitroute.Load(message.get())) {
                    return false;
                }

                if (transitroute.CommandId == model::Commands_AcceptClient ||
                    transitroute.CommandId == model::Commands_AbortClient ||
                    transitroute.CommandId == model::Commands_Transitroute) {
                    return false;
                }

                unsigned int counts = 0;
                std::shared_ptr<unsigned char> packet = make_shared_null<unsigned char>();
                int rawSuccess = [&]() { // 今时不同往日 务农也讲知识
                    Transitroute exchange; // 虚拟交换机（HUB）
                    exchange.Payload = transitroute.Payload;
                    exchange.CommandId = transitroute.CommandId;
                    exchange.SequenceNo = transitroute.SequenceNo;
                    exchange.ServerNo = socket->ServerNo; // transitroute.ServerNo;
                    exchange.LinkType = socket->LinkType; // transitroute.LinkType;
                    exchange.Platform = socket->PlatformName; // transitroute.Platform;

                    packet = exchange.ToPacket(StageSocket::ServerKf, message->CommandId, message->SequenceNo, counts);
                    if (!packet || counts == 0) {
                        return -1;
                    }

                    std::shared_ptr<Server> server;
                    std::shared_ptr<Server::SocketVectorObject> serverVector =
                        communication->GetServerSocketVector(transitroute.Platform, transitroute.ServerNo, transitroute.LinkType, server);
                    if (!server) {
                        return 1;
                    }

                    if (!serverVector) {
                        return 2;
                    }

                    return serverVector->Send(packet, counts) ? 0 : 3;
                }();
                if (rawSuccess == 0) {
                    return true;
                }
                else if (rawSuccess < 0) {
                    return false;
                }
                else {
                    std::shared_ptr<ServerSendbackController> sendbackController = communication->GetSendbackController();
                    if (sendbackController) {
                        sendbackController->AddSendback(transitroute.Platform, transitroute.ServerNo, transitroute.LinkType, packet, counts);
                    }
                    return true;
                }
            }
        };

        bool StageSocketCommunication::Server::RemoveByLinkNo(Byte chType, unsigned int linkNo, bool server, std::shared_ptr<SocketVectorObject>& socketVector) {
            socketVector = make_shared_null<SocketVectorObject>();
            if (!this) {
                return false;
            }

            auto pl = Object::addressof(oSocketClient);
            if (server) {
                pl = Object::addressof(oServerSocket);
            }

            if (!Object::synchronize<bool>(syncobj, [&] { // anti-deadline-0
                auto il = pl->find(chType);
                if (il == pl->end()) {
                    return false;
                }

                socketVector = il->second;
                return NULL != socketVector.get();
            })) {
                return false;
            }

            auto socket = make_shared_null<StageSocket>();
            if (!socketVector->RemoveByLinkNo(linkNo, socket)) {
                return false;
            }

            return Object::synchronize<bool>(syncobj, [&] {
                if (socketVector->sockets.empty()) {
                    auto il = pl->find(chType);
                    if (il != pl->end()) {
                        pl->erase(il);
                    }
                }
                return true;
            });
        }

        bool StageSocketCommunication::Server::SocketVectorObject::RemoveByLinkNo(unsigned int linkNo, std::shared_ptr<StageSocket>& socket) {
            socket = make_shared_null<StageSocket>();
            if (!this) {
                return false;
            }
            return Object::synchronize<bool>(syncobj, [&] {
                Server::SocketVector::iterator& rl = current;
                Server::SocketVector::iterator el = sockets.end(); // 抗当前迭代器被破坏

                if (sockets.empty()) {
                    rl = el;
                    return false;
                }

                auto il = sockets.find(linkNo);
                if (il == sockets.end()) {
                    return false;
                }

                unsigned int dwCurrentKey = 0;
                if (rl != el) { // 当前迭代器等于要删除的成员时向后移动迭代器
                    if (il->first == rl->first) {
                        ++rl;
                        if (rl != el) {
                            dwCurrentKey = rl->first;
                        }
                    }
                }

                socket = il->second;
                sockets.erase(il);

                if (0 != dwCurrentKey) {
                    rl = el;
                }
                else {
                    rl = sockets.find(dwCurrentKey);
                }
                return true;
            });
        }

        StageSocketCommunication::StageSocketCommunication(const std::shared_ptr<Hosting>& hosting, int backlog, int clientPort, int serverPort, bool onlyRoutingTransparent, bool allowInternalHostAbortCloseAllClient)
            : _disposed(false)
            , _onlyRoutingTransparent(onlyRoutingTransparent)
            , _allowInternalHostAbortCloseAllClient(allowInternalHostAbortCloseAllClient)
            , _hosting(hosting)
            , _backlog(backlog)
            , _clientPort(clientPort)
            , _serverPort(serverPort)
            , _linkNoAtomic(0) {
            if (clientPort <= 0 || clientPort > 65535) {
                throw std::runtime_error("The listening clientPort must not be less than or equal to 0 and greater than 65535");
            }

            if (serverPort <= 0 || serverPort > 65535) {
                throw std::runtime_error("The listening clientPort must not be less than or equal to 0 and greater than 65535");
            }

            if (!hosting) {
                throw std::runtime_error("The Hosting does not allow references to null address identifiers");
            }

            if (backlog <= 0) {
                backlog = 1 << 10; // 1024
            }

            // 最大接受队列数量
            _backlog = backlog;
        }

        StageSocketCommunication::~StageSocketCommunication() {
            Dispose();
        }

        void StageSocketCommunication::Dispose() {
            StageSocketCommunication* self = this;
            if (!self) {
                return;
            }
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (!_disposed) {
                _disposed = true;
                if (_clientListener) {
                    _clientListener->Dispose();
                    _clientListener.reset();
                }
                if (_serverListener) {
                    _serverListener->Dispose();
                    _serverListener.reset();
                }
                if (_hosting) {
                    _hosting.reset();
                }
            }
        }

        bool StageSocketCommunication::IsDisposed() {
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            return _disposed;
        }

        std::shared_ptr<StageSocketCommunication> StageSocketCommunication::GetPtr() {
            StageSocketCommunication* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<StageSocketCommunication>();
        }

        bool StageSocketCommunication::Listen() {
            std::shared_ptr<StageSocketCommunication> self = GetPtr();
            if (!self) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_disposed) {
                return false;
            }

            if (!_sendbackController) {
                _sendbackController = CreateSendbackController();
            }

            if (!_sendbackController) {
                throw std::runtime_error("Unable to create sendback controller instance");
            }

            if (!_clientListener) {
                bool transparent = false;
                bool needToVerifyMaskOff = true;
                bool onlyRoutingTransparent = IsOnlyRoutingTransparent();
                _clientListener = make_shared_object<StageSocketListener2>(
                    _hosting,
                    StageSocket::SessionKf, 
                    _backlog, 
                    _clientPort,
                    transparent,
                    needToVerifyMaskOff,
                    onlyRoutingTransparent,
                    [self](std::shared_ptr<StageSocketListener>& listener) {
                    return self->CreateClientSocketHandler(listener);
                });
            }

            if (!_clientListener) {
                throw std::runtime_error("Unable to create a client socket listener instance");
            }

            if (!_serverListener) {
                bool transparent = true;
                bool needToVerifyMaskOff = false;
                bool onlyRoutingTransparent = false;
                _serverListener = make_shared_object<StageSocketListener2>(
                    _hosting,
                    StageSocket::ServerKf, 
                    _backlog, 
                    _serverPort, 
                    transparent,
                    needToVerifyMaskOff,
                    onlyRoutingTransparent,
                    [self](std::shared_ptr<StageSocketListener>& listener) {
                    return self->CreateServerSocketHandler(listener);
                });
            }

            if (!_serverListener) {
                throw std::runtime_error("Unable to create a server socket listener instance");
            }

            if (!_sendbackController->Run()) {
                throw std::runtime_error("Unable to run a sendback controller instance");
            }

            if (!_clientListener->Listen()) {
                throw std::runtime_error("Unable to listen a server socket listener instance");
            }
            
            if (!_serverListener->Listen()) {
                throw std::runtime_error("Unable to listen a server socket listener instance");
            }
            return true;
        }

        SyncObjectBlock& StageSocketCommunication::GetSynchronizingObject() {
            return _syncobj;
        }

        std::shared_ptr<StageSocketListener> StageSocketCommunication::GetClientSocketListener() {
            return _clientListener;
        }

        std::shared_ptr<StageSocketListener> StageSocketCommunication::GetServerSocketListener() {
            return _serverListener;
        }

        bool StageSocketCommunication::IsOnlyRoutingTransparent()
        {
            if (!this) {
                return false;
            }
            return _onlyRoutingTransparent;
        }

        bool StageSocketCommunication::IsAllowInternalHostAbortCloseAllClient()
        {
            if (!this) {
                return false;
            }
            return _allowInternalHostAbortCloseAllClient;
        }

        bool StageSocketCommunication::IsEnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            bool success = false;
            if (_serverListener) {
                success |= _serverListener->IsEnabledCompressedMessages();
            }
            if (_clientListener) {
                success |= _serverListener->IsEnabledCompressedMessages();
            }
            return success;
        }

        bool StageSocketCommunication::EnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            bool success = false;
            if (_serverListener) {
                success |= _serverListener->EnabledCompressedMessages();
            }
            if (_clientListener) {
                success |= _serverListener->EnabledCompressedMessages();
            }
            return success;
        }

        bool StageSocketCommunication::DisabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            bool success = false;
            if (_serverListener) {
                success |= _serverListener->DisabledCompressedMessages();
            }
            if (_clientListener) {
                success |= _serverListener->DisabledCompressedMessages();
            }
            return success;
        }

        bool StageSocketCommunication::LoadBulkMany(BulkManyMessage& bulk, const std::shared_ptr<Message>& message)
        {
            if (!message) {
                return false;
            }

            std::shared_ptr<BufferSegment> rawMessage = message->Payload;
            if (!rawMessage) {
                return false;
            }

            if (rawMessage->Length <= 0) {
                return false;
            }

            // 路由服务器提供消息实例
            if (!bulk.LoadFrom(*message.get())) {
                return false;
            }

            if (NULL == bulk.Member || !(bulk.MemberCount > 0)) {
                return false;
            }

            return true;
        }

        std::shared_ptr<ServerSendbackController> StageSocketCommunication::GetSendbackController() {
            return _sendbackController;
        }

        std::shared_ptr<IStageSocketHandler> StageSocketCommunication::CreateServerSocketHandler(std::shared_ptr<StageSocketListener>& listener) {
            if (!listener) {
                return make_shared_null<IStageSocketHandler>();
            }
            auto self = GetPtr();
            return make_shared_object<ServerStageSocketHandler>(self, listener);
        }

        std::shared_ptr<IStageSocketHandler> StageSocketCommunication::CreateClientSocketHandler(std::shared_ptr<StageSocketListener>& listener) {
            if (!listener) {
                return make_shared_null<IStageSocketHandler>();
            }
            auto self = GetPtr();
            return make_shared_object<ClientStageSocketHandler>(self, listener);
        }

        int StageSocketCommunication::GetAllPlatformNames(std::vector<std::string>& s)
        {
            unsigned int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            do {
                if (_disposed) {
                    break;
                }

                unsigned int counts = s.size();
                for (auto il : _platformServerTable) {
                    if (events < counts) {
                        s[events++] = il.first;
                    }
                    else {
                        events++;
                        s.push_back(il.first);
                    }
                }
            } while (0, 0);
            return events;
        }

        int StageSocketCommunication::GetAllServerNo(const std::string& platform, std::vector<int>& s)
        {
            unsigned int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            do {
                if (_disposed) {
                    break;
                }

                auto il = _platformServerTable.find(platform);
                if (il == _platformServerTable.end()) {
                    break;
                }

                auto& collections = il->second;
                if (collections.empty()) {
                    break;
                }

                unsigned int counts = s.size();
                for (auto sl : collections) {
                    if (events < counts) {
                        s[events++] = sl.first;
                    }
                    else {
                        events++;
                        s.push_back(sl.first);
                    }
                }
            } while (0, 0);
            return events;
        }

        std::shared_ptr<StageSocketCommunication::Server> StageSocketCommunication::AddOrFindServerObject(const std::string& platform, int serverNo, bool adding, std::function<bool(std::shared_ptr<Server>&)> predicate) {
            typedef StageSocketCommunication::Server Server;

            std::shared_ptr<Server> server = make_shared_null<Server>();
            do {
                StageSocketCommunication* self = this;
                if (!self) {
                    break;
                }

                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                do {
                    if (_disposed) {
                        break;
                    }

                    if (_platformServerTable.empty()) {
                        break;
                    }

                    PlatformServerTable::iterator iterTail = _platformServerTable.find(platform);
                    PlatformServerTable::iterator iterEndl = _platformServerTable.end();
                    if (iterEndl == iterTail) {
                        break;
                    }

                    ServerServerTable& oTable = Object::constcast(iterTail->second);
                    ServerServerTable::iterator iterValue = oTable.find(serverNo);
                    if (iterValue == oTable.end()) {
                        break;
                    }

                    server = iterValue->second;
                } while (0, 0);
                bool newing = false;
                if (adding && !server) {
                    newing = true;
                    server = make_shared_object<Server>();
                }

                if (predicate) {
                    if (!predicate(server)) {
                        server = make_shared_null<Server>();
                    }
                }

                if (newing && server) {
                    _platformServerTable[platform][serverNo] = server;
                }
            } while (0, 0);
            return server;
        }

        std::shared_ptr<StageSocketCommunication::Server> StageSocketCommunication::GetServerObject(const std::string& platform, int serverNo) {
            std::function<bool(std::shared_ptr<Server>&)> predicate = NULL;
            return AddOrFindServerObject(platform, serverNo, false, predicate);
        }

        std::shared_ptr<StageSocketCommunication::Server> StageSocketCommunication::AddServerObject(const std::string& platform, int serverNo, const std::function<bool(std::shared_ptr<Server>&)> predicate) {
            return AddOrFindServerObject(platform, serverNo, true, Object::constcast(predicate));
        }
        
        std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject> StageSocketCommunication::GetUnknowSocketVector(bool serverMode, const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server) {
            typedef StageSocketCommunication::Server Server;

            std::shared_ptr<Server>& serverw = Object::constcast(server);

            serverw = make_shared_null<Server>();
            if (!this) {
                return make_shared_null<Server::SocketVectorObject>();
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            do {
                serverw = GetServerObject(platform, serverNo);
                if (!serverw) {
                    return make_shared_null<Server::SocketVectorObject>();
                }

                auto po = Object::addressof(serverw->oSocketClient);
                if (serverMode) {
                    po = Object::addressof(serverw->oServerSocket);
                }

                auto tail = po->find(linkType);
                if (tail == po->end()) {
                    return make_shared_null<Server::SocketVectorObject>();
                }

                std::shared_ptr<Server::SocketVectorObject> socketVector = tail->second;
                return socketVector;
            } while (0, 0);
            return make_shared_null<Server::SocketVectorObject>();
        }

        std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject> StageSocketCommunication::GetServerSocketVector(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server) {
            return GetUnknowSocketVector(true, platform, serverNo, linkType, server);
        }

        std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject> StageSocketCommunication::GetSocketClientVector(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<Server>& server) {
            return GetUnknowSocketVector(false, platform, serverNo, linkType, server);
        }

        unsigned short StageSocketCommunication::NewLinkNo(const std::string& platform, int serverNo, Byte linkType, bool server) {
            typedef StageSocketCommunication::Server Server;

            unsigned short linkNo = 0;
            if (!this) {
                return linkNo;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_disposed) {
                return linkNo;
            }

            auto servero = GetServerObject(platform, serverNo);
            Server::SocketVector* sockets = NULL;
            std::shared_ptr<Server::SocketVectorObject> socketVector;
            if (servero) {
                auto socketTable = &servero->oSocketClient;
                if (server) {
                    socketTable = &servero->oServerSocket;
                }
                auto tail = socketTable->find(linkType);
                if (tail != socketTable->end()) {
                    socketVector = tail->second;
                    if (socketVector) {
                        sockets = &socketVector->sockets;
                    }
                }
                if (NULL != sockets) {
                    if (sockets->empty()) {
                        sockets = NULL;
                    }
                }
            }

            std::function<unsigned short()> fNewLinkNoImpl = [&] {
                for (int c = 0; c < 0xffff; c++) {
                    do {
                        linkNo = ++_linkNoAtomic;
                    } while (0 == linkNo);
                    if (NULL == servero || NULL == sockets) {
                        break;
                    }
                    auto tail = sockets->find(linkNo);
                    if (tail == sockets->end()) {
                        break;
                    }
                }
                return linkNo;
            };
            if (!socketVector) {
                return fNewLinkNoImpl();
            }
            return Object::synchronize(socketVector->syncobj, fNewLinkNoImpl);
        }

        std::shared_ptr<ServerSendbackController> StageSocketCommunication::CreateSendbackController() {
            return make_shared_object<ServerSendbackController>();
        }

        StageSocketCommunication::Server::Server() {
            Clear();
        }

        StageSocketCommunication::Server::~Server() {
            Dispose();
        }

        void StageSocketCommunication::Server::Clear() {
            Object::synchronize(syncobj, [&] {
                wSvrAreaNo = 0;
                wServerNo = 0;
                wMaxSocketCount = 0;
                szPlatformName = "";
                oServerSocket.clear();
                oSocketClient.clear();
            });
        }

        SyncObjectBlock& StageSocketCommunication::Server::GetSynchronizingObject() {
            return syncobj;
        }

        void StageSocketCommunication::Server::Dispose() {
            Clear();
        }

        int StageSocketCommunication::Server::GetAllLinkType(std::vector<Byte>& s, bool server)
        {
            unsigned int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(syncobj);
            do {
                SocketTable* po = Object::addressof(oSocketClient);
                if (server) {
                    po = Object::addressof(oServerSocket);
                }

                unsigned int counts = s.size();
                for (auto il : *po) {
                    if (events < counts) {
                        s[events++] = il.first;
                    }
                    else {
                        events++;
                        s.push_back(il.first);
                    }
                }
            } while (0, 0);
            return events;
        }

        int StageSocketCommunication::Server::GetAllSocketVector(std::vector<std::shared_ptr<SocketVectorObject>/**/>& s, bool server)
        {
            unsigned int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(syncobj);
            do {
                SocketTable* po = Object::addressof(oSocketClient);
                if (server) {
                    po = Object::addressof(oServerSocket);
                }

                unsigned int counts = s.size();
                for (auto il : *po) {
                    if (events < counts) {
                        s[events++] = il.second;
                    }
                    else {
                        events++;
                        s.push_back(il.second);
                    }
                }
            } while (0, 0);
            return events;
        }

        std::shared_ptr<StageSocketCommunication::Server::SocketVectorObject> StageSocketCommunication::Server::FindSocketVector(Byte chType, bool server)
        {
            std::shared_ptr<SocketVectorObject> result = make_shared_null<SocketVectorObject>();
            if (!this) {
                return result;
            }

            Object::synchronize(syncobj, [&] {
                auto pl = Object::addressof(oSocketClient);
                if (server) {
                    pl = Object::addressof(oServerSocket);
                }
                auto il = pl->find(chType);
                if (il != pl->end()) {
                    result = il->second;
                }
            });
            return result;
        }

        StageSocketCommunication::Server::SocketVectorObject::SocketVectorObject()
            : linkType(0) {
            Clear(false);
        }

        StageSocketCommunication::Server::SocketVectorObject::~SocketVectorObject() {
            Dispose();
        }

        void StageSocketCommunication::Server::SocketVectorObject::Clear(bool closing) {
            std::vector<UInt64> handles;
            do {
                std::unique_lock<SyncObjectBlock> score(syncobj);
                if (!closing) {
                    sockets.clear();
                    current = sockets.end();
                }
                else {
                    auto il = sockets.begin();
                    auto el = sockets.end();
                    for (; il != el; ++il) {
                        handles.push_back(il->first);
                    }
                    current = sockets.end();
                }
                allowAccept = false;
            } while (0, 0);
            size_t i = 0;
            size_t l = handles.size();
            for (; i < l; ++i) {
                std::shared_ptr<StageSocket> socket = make_shared_null<StageSocket>();
                if (RemoveByLinkNo(handles[i], socket)) {
                    if (socket) {
                        socket->Close();
                        socket->Dispose();
                    }
                }
            }
            if (closing) {
                Clear(false);
            }
        }

        bool StageSocketCommunication::Server::SocketVectorObject::Send(const std::shared_ptr<unsigned char>& packet, size_t counts) {
            StageSocketCommunication::Server::SocketObject socket = NextSocket();
            if (!socket) {
                return false;
            }

            std::shared_ptr<Socket> socketObj = Object::as<Socket>(socket);
            if (!socketObj) {
                return false;
            }
            return socketObj->Send(packet, counts, NULL);
        }

        SyncObjectBlock& StageSocketCommunication::Server::SocketVectorObject::GetSynchronizingObject() {
            return syncobj;
        }

        StageSocketCommunication::Server::SocketObject StageSocketCommunication::Server::SocketVectorObject::NextSocket() {
            SocketObject socket = make_shared_null<StageSocket>();
            do {
                std::unique_lock<SyncObjectBlock>(syncobj);
                if (sockets.empty()) {
                    current = sockets.end();
                }
                else {
                    if (current != sockets.end()) {
                        ++current;
                    }
                    if (current == sockets.end()) {
                        current = sockets.begin();
                    }
                    if (current != sockets.end()) {
                        socket = current->second;
                    }
                }
            } while (0, 0);
            return socket;
        }

        boost::asio::ip::tcp::endpoint StageSocketCommunication::Server::SocketVectorObject::GetRemoteEndPoint()
        {
            StageSocketCommunication::Server::SocketObject socket = NextSocket();
            if (socket) {
                return socket->GetRemoteEndPoint();
            }
            return SocketListener::AnyAddress(0);
        }

        StageSocketCommunication::Server::SocketObject StageSocketCommunication::Server::SocketVectorObject::FindSocket(unsigned int linkNo) {
            SocketObject socket = make_shared_null<StageSocket>();
            do {
                std::unique_lock<SyncObjectBlock> score(syncobj);
                if (!sockets.empty()) {
                    auto tail = sockets.find(linkNo);
                    if (tail != sockets.end()) {
                        socket = tail->second;
                    }
                }
            } while (0, 0);
            return socket;
        }

        void StageSocketCommunication::Server::SocketVectorObject::Dispose() {
            Clear(true);
        }
    }
}