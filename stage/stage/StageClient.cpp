#include "StageClient.h"
#include "StageServerSocket.h"
#include "StageVirtualSocket.h"
#include "StageSocketCommunication.h"

namespace ep
{
    namespace stage
    {
        const int StageClient::MAX_CONCURRENT;
        const int StageClient::MIN_CONCURRENT;

        StageClient::StageClient(const std::shared_ptr<Hosting>& hosting, const std::string& hostName, int port, Byte linkType, const std::string& platform, int svrAreaNo, int serverNo, int maxFiledescriptor, int maxConcurrent)
            : _hosting(hosting)
            , _chLinkType(linkType)
            , _maxConcurrent(maxConcurrent)
            , _port(port)
            , _hostName(hostName)
            , _disposed(false)
            , _listening(false)
            , _absOpened(false)
            , _allowAccept(false)
            , _firstTimeAcceptClient(false)
            , _platform(platform)
            , _svrAreaNo(svrAreaNo)
            , _serverNo(serverNo)
            , _maxFiledescriptor(maxFiledescriptor)
            , _lastRecoverAllChannelTime(0)
        {
            if (port <= 0 || port > 65535) {
                throw std::runtime_error("Ports linked to remote hosts must not be less than or equal to 0 or greater than 65535");
            }

            if (!hosting) {
                throw std::runtime_error("A local host instance hosting the current running instance is not allowed to be a NullReferences");
            }

            if (hostName.empty()) {
                throw std::runtime_error("The remote host name of the link is not allowed to be NullReferences");
            }

            if (maxFiledescriptor < StageSocketBuilder::MIN_FILEDESCRIPTOR) {
                maxFiledescriptor = StageSocketBuilder::MIN_FILEDESCRIPTOR;
                _maxFiledescriptor = maxFiledescriptor;
            }
            else if (maxFiledescriptor > StageSocketBuilder::MAX_FILEDESCRIPTOR) {
                maxFiledescriptor = StageSocketBuilder::MAX_FILEDESCRIPTOR;
                _maxFiledescriptor = maxFiledescriptor;
            }

            _availableChannelsList.Clear();
            if (maxConcurrent <= 0) {
                maxConcurrent = Hosting::GetProcesserCount();
                if (maxConcurrent < MIN_CONCURRENT) {
                    maxConcurrent = MIN_CONCURRENT;
                }
            }

            if (maxConcurrent > MAX_CONCURRENT) { // 通常服务器与服务器之间最大数量普遍建议为32个指标(具体参考成熟的中间件或者基础架构)
                maxConcurrent = MAX_CONCURRENT;
            }

            // 配置适配器投递默认堆积基础选项
            _sendbackController.settings.
                SetThoroughlyAbandonTime(1 * 60 * 1000).    // 失去链路超出一分钟彻底放弃堆积在适配器的待发送的内容（缺省为一分钟，如果链路无法重新建立或积极拒接则彻底放弃）
                SetMaxSendQueueSize(10000).                 // 最大发送队列块数量（缺省为一万个）
                SetMaxSendBufferSize(10 * 1024 * 1024);     // 最大发送队列缓存器大小（缺省为十兆字节）

            for (int i = 0; i < maxConcurrent; i++) {
                auto node = make_shared_object<SocketLinkedListNode>();
                if (!node) {
                    throw std::runtime_error("The socket linkedlist node object cannot be assigned");
                }

                _freeChannelNode.push_back(node);
            }
        }

        StageClient::~StageClient()
        {
            Dispose();
        }

        bool StageClient::IsDisposed()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _disposed;
        }

        int StageClient::GetMaxConcurrent()
        {
            return _maxConcurrent;
        }

        const std::string& StageClient::GetHostName()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _hostName;
        }

        int StageClient::GetPort()
        {
            return _port;
        }

        std::shared_ptr<Hosting> StageClient::GetHosting()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _hosting;
        }

        bool StageClient::IsListening()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            return _listening;
        }

        bool StageClient::IsOnlyRoutingTransparent()
        {
            return false;
        }

        int StageClient::GetAllPlatformName(std::vector<std::string>& s)
        {
            int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_disposed) {
                auto il = _serverSocketTable.begin();
                auto el = _serverSocketTable.end();
                for (; il != el; il++) {
                    events++;
                    s.push_back(il->first);
                }
            }
            return events;
        }

        int StageClient::GetAllLinkType(const std::string& platform, std::vector<Byte>& s)
        {
            int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_disposed) {
                auto il = _serverSocketTable.find(platform);
                auto el = _serverSocketTable.end();
                if (il != el) {
                    auto& collections = il->second;
                    if (!collections.empty()) {
                        auto collil = collections.begin();
                        auto collel = collections.end();
                        for (; collil != collel; collil++) {
                            events++;
                            s.push_back(collil->first);
                        }
                    }
                }
            }
            return events;
        }

        int StageClient::GetAllServerSocketTable(const std::string& platform, std::vector<std::shared_ptr<ServerSocketTable>/**/>& s)
        {
            int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_disposed) {
                auto il = _serverSocketTable.find(platform);
                auto el = _serverSocketTable.end();
                if (il != el) {
                    auto& collections = il->second;
                    if (!collections.empty()) {
                        auto collil = collections.begin();
                        auto collel = collections.end();
                        for (; collil != collel; collil++) {
                            events++;
                            s.push_back(collil->second);
                        }
                    }
                }
            }
            return events;
        }

        std::shared_ptr<StageClient::ServerSocketTable> StageClient::GetServerSocketTable(const std::string& platform, Byte linkType)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            do {
                if (_disposed) {
                    break;
                }

                auto il = _serverSocketTable.find(platform);
                auto el = _serverSocketTable.end();
                if (il == el) {
                    break;
                }

                auto& collections = il->second;
                if (collections.empty()) {
                    _serverSocketTable.erase(il);
                    break;
                }

                auto collil = collections.find(linkType);
                auto collel = collections.end();
                if (collil == collel) {
                    break;
                }

                return collil->second;
            } while (0, 0);
            return make_shared_null<ServerSocketTable>();
        }

        std::shared_ptr<StageServerSocket> StageClient::GetServerSocket(const std::string& platform, Byte linkType, int serverNo)
        {
            std::shared_ptr<ServerSocketTable> socketTable;
            return FindOrAddServerSocket(platform, linkType, serverNo, socketTable, false, NULL);
        }

        int StageClient::GetTickAlwaysIntervalTime()
        {
            return 500;
        }

        bool StageClient::ProcessTickAlways()
        {
            UInt64 ullCurrentTime = Hosting::GetTickCount();
            if (0 == _lastRecoverAllChannelTime) {
                _lastRecoverAllChannelTime = ullCurrentTime;
            }
            else if (_lastRecoverAllChannelTime > ullCurrentTime || 
                (ullCurrentTime - _lastRecoverAllChannelTime) >= (UInt64)GetRecoverAllChannelsTickAlwaysTime()) {
                _lastRecoverAllChannelTime = ullCurrentTime;
                RecoverAllChannels();
            }
            int iThoroughlyAbandonTime = _sendbackController.settings.GetThoroughlyAbandonTime();
            if (iThoroughlyAbandonTime >= 0) {
                Int64 llAbortingTime = _sendbackController.abortingTime;
                if (0 != llAbortingTime) {
                    if (llAbortingTime < 0 || (UInt64)llAbortingTime >= ullCurrentTime || (ullCurrentTime - (UInt64)llAbortingTime) >= (UInt64)iThoroughlyAbandonTime) {
                        ResetAllSendback();
                    }
                }
            }
            return true;
        }

        int StageClient::GetRecoverAllChannelsTickAlwaysTime()
        {
            return 1000;
        }

        bool StageClient::RecoverAllChannels()
        {
            return AddAndOpenReleaseAllChannels(1) >= 0;
        }

        bool StageClient::IsLinkAbortCloseAllVirtualSockets()
        {
            return true;
        }

        bool StageClient::IsLinkAbortCloseAllServerSockets()
        {
            return true;
        }

        int StageClient::AddAndOpenReleaseAllChannels(int mode)
        {
            std::shared_ptr<StageClient> self = GetPtr();
            if (!self) {
                return -1;
            }
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return -2;
            }
            if (mode == 0) {
                if (_listening) {
                    return -3;
                }
                if (!_tickAlways) {
                    std::shared_ptr<Hosting> hosting = GetHosting();
                    if (!hosting) {
                        return -4;
                    }
                    _tickAlways = Timer::Create<Timer>();
                    if (!_tickAlways) {
                        return -5;
                    }
                    _tickAlways->SetInterval(GetTickAlwaysIntervalTime());
                    _tickAlways->TickEvent = [self](std::shared_ptr<Timer>& sender, Timer::TickEventArgs& e) {
                        if (self) {
                            std::unique_lock<SyncObjectBlock> scoped(self->GetSynchronizingObject());
                            if (!self->IsDisposed()) {
                                self->ProcessTickAlways();
                            }
                        }
                    };
                    _lastRecoverAllChannelTime = Hosting::GetTickCount();
                    _tickAlways->Start();
                }
                _listening = true;
            }

            int events = 0;
            unsigned int counts = _freeChannelNode.size();
            for (unsigned int i = 0; i < counts; i++) {
                SocketObject socket = CreateSocket();
                if (!socket) {
                    continue;
                }
                if (!AddChannel(socket)) {
                    continue;
                }
                socket->Open(); // 打开套接字实例
                if (socket->IsDisposed()) {
                    continue;
                }
                else {
                    events++;
                }
                _remoteEP = socket->GetRemoteEndPoint();
            }

            return events;
        }

        SyncObjectBlock& StageClient::GetSynchronizingObject()
        {
            return _syncobj;
        }

        int StageClient::GetAvailableChannels()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return 0;
            }
            return _availableChannelsList.Count();
        }

        int StageClient::GetAvailableChannels(std::set<SocketObject>& s)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

            int events = 0;
            int counts = _availableChannelsList.Count();
            if (counts > 0) {
                auto node = _availableChannelsList.First();
                while (node) {
                    auto socket = node->Value;
                    if (socket) {
                        events++;
                        s.insert(socket);
                    }
                    node = node->Next;
                }
            }
            return events;
        }

        void StageClient::Close()
        {
            Dispose();
        }

        bool StageClient::IsSecurityObject(ISocket* socket)
        {
            if (!this) {
                return false;
            }

            if (!socket) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            else {
                StageSocket* poss = dynamic_cast<StageSocket*>(socket);
                if (NULL != poss) {
                    auto il = _workingChannelsTable.find(poss);
                    auto el = _workingChannelsTable.end();
                    if (il == el) {
                        return false;
                    }

                    std::shared_ptr<SocketLinkedListNode> node = il->second;
                    if (!node) {
                        return false;
                    }

                    SocketObject po = node->Value;
                    if (!po) {
                        return false;
                    }

                    return po.get() == poss;
                }
            }

            if (_disposed) {
                return false;
            }
            else {
                StageVirtualSocket* povs = dynamic_cast<StageVirtualSocket*>(socket);
                if (NULL != povs) {
                    auto il = _virtualSocketTable2.find(povs);
                    auto el = _virtualSocketTable2.end();
                    if (il == el) {
                        return false;
                    }

                    VirtualSocket po = il->second;
                    if (!po) {
                        return false;
                    }

                    return po.get() == povs;
                }
            }

            return false;
        }

        StageClient::SocketObject StageClient::NextAvailableChannel()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

            int counts = _availableChannelsList.Count();
            if (counts <= 0) {
                _currentAvailableChannels = NULL;
            }
            else {
                if (!_currentAvailableChannels) {
                    _currentAvailableChannels = _availableChannelsList.First();
                }
                while (_currentAvailableChannels) {
                    auto socket = _currentAvailableChannels->Value;
                    if (socket) {
                        return socket;
                    }
                    _currentAvailableChannels = _currentAvailableChannels->Next;
                }
            }
            return make_shared_null<StageSocket>();
        }

        StageClient::SocketObject StageClient::CreateSocket()
        {
            StageSocketBuilder builder;
            return make_shared_object<StageSocket>(builder
                .SetKf(GetKf())
                .SetLinkType(GetLinkType())
                .SetPlatform(GetPlatform())
                .SetPort(GetPort())
                .SetHosting(GetHosting())
                .SetHostName(GetHostName())
                .SetServerNo(GetServerNo())
                .SetTransparent(true)
                .SetNeedToVerifyMaskOff(false)
                .SetMaxFiledescriptor(GetMaxFiledescriptor())
                .SetSvrAreaNo(GetSvrAreaNo())
                .SetSocketHandler(GetSocketHandler())
                .SetOnlyRoutingTransparent(IsOnlyRoutingTransparent()));
        }

        std::shared_ptr<IStageSocketHandler> StageClient::GetSocketHandler()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_socketHandler) {
                if (!_disposed) {
                    _socketHandler = make_shared_object<IStageSocketHandler>();
                }
            }
            return _socketHandler;
        }

        bool StageClient::AddChannel(SocketObject& socket)
        {
            if (!this) {
                return false;
            }

            if (!socket) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }

            auto po = socket.get();
            auto il = _workingChannelsTable.find(po);
            auto el = _workingChannelsTable.end();
            if (il != el) {
                return true;
            }

            if (_freeChannelNode.empty()) {
                return false;
            }

            std::shared_ptr<SocketLinkedListNode> node = _freeChannelNode.back();
            assert(NULL != node);

            // 从释放通道节点容器之中弹出当前使用的节点
            _freeChannelNode.pop_back();

            // 写入新构建的套接字对象实例的引用到节点上
            node->Value = socket;

            auto self = GetPtr();
            auto fAbort = [this, self](std::shared_ptr<ISocket>& sender, EventArgs& e) {
                if (!sender) {
                    return;
                }
                std::shared_ptr<StageSocket> raw = Object::as<StageSocket>(sender);
                do {
                    if (!raw) {
                        break;
                        ProcessAbort(raw);
                    }

                    if (IsDisposed()) {
                        break;
                    }

                    ProcessAbort(raw);
                } while (0, 0);
                if (sender) {
                    sender->Close();
                    sender->Dispose();
                }
            };
            socket->CloseEvent = fAbort;
            socket->AbortEvent = fAbort;
            socket->OpenEvent = [this, self](std::shared_ptr<ISocket>& sender, EventArgs& e) {
                if (!sender) {
                    return;
                }
                bool closing = false;
                std::shared_ptr<StageSocket> raw = Object::as<StageSocket>(sender);
                do {
                    if (!raw) {
                        closing = true;
                        break;
                    }

                    std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                    if (IsDisposed()) {
                        closing = true;
                        break;
                    }

                    _localEP = raw->GetLocalEndPoint();
                    closing = !ProcessOpen(raw);
                } while (0, 0);
                if (closing) {
                    if (sender) {
                        sender->Close();
                        sender->Dispose();
                    }
                }
            };
            socket->MessageEvent = [this, self](std::shared_ptr<ISocket>& sender, std::shared_ptr<Message>& e) {
                if (!sender) {
                    return;
                }
                bool closing = false;
                std::shared_ptr<StageSocket> raw = Object::as<StageSocket>(sender);
                do {
                    if (!raw) {
                        closing = true;
                        break;
                    }
                    else {
                        std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                        if (IsDisposed()) {
                            closing = true;
                            break;
                        }
                    }
                    if (!ProcessExchange(raw, e)) {
                        closing = true;
                    }
                } while (0, 0);
                if (closing) {
                    if (sender) {
                        sender->Close();
                        sender->Dispose();
                    }
                }
            };

            _workingChannelsTable[po] = node;
            return true;
        }

        bool StageClient::ReleaseChannel(SocketObject& socket)
        {
            if (!this) {
                return false;
            }

            if (!socket) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }

            auto po = socket.get();
            auto il = _workingChannelsTable.find(po);
            auto el = _workingChannelsTable.end();
            if (il == el) {
                return true;
            }

            std::shared_ptr<SocketLinkedListNode> node = il->second;
            _workingChannelsTable.erase(il);

            if (!node) {
                assert(false);
                return false;
            }

            auto no = node.get();
            if (_currentAvailableChannels) {
                if (no == _currentAvailableChannels) {
                    _currentAvailableChannels = _currentAvailableChannels->Next;
                }
            }

            if (no->LinkedList_) { // 已加入可用链表则从链表之中删除这个节点
                _availableChannelsList.Remove(no);
            }

            auto so = node->Value;
            if (so) {
                so->Close();
                so->Dispose();
            }

            _freeChannelNode.push_back(node);
            return true;
        }

        bool StageClient::MarkAvailableChannel(SocketObject& socket)
        {
            if (!this) {
                return false;
            }

            if (!socket) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }

            if(_workingChannelsTable.empty()) {
                return false;
            }

            auto po = socket.get();
            auto il = _workingChannelsTable.find(po);
            auto el = _workingChannelsTable.end();
            if (il == el) {
                return false;
            }

            std::shared_ptr<SocketLinkedListNode> node = il->second;
            if (!node) {
                return false;
            }

            _availableChannelsList.AddLast(node.get());
            return true;
        }

        void StageClient::ProcessAbort(std::shared_ptr<StageSocket>& socket)
        {
            bool events = false;
            bool closeAll = false;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (ReleaseChannel(socket)) {
                    if (_availableChannelsList.Count() <= 0) {
                        closeAll = true;
                        if (_absOpened) {
                            events = true;
                        }
                        _absOpened = false;
                        _allowAccept = false;
                        _firstTimeAcceptClient = false;
                    }
                }
            } while (0, 0);
            if (closeAll) {
                _sendbackController.abortingTime = Hosting::GetTickCount();
                if (IsLinkAbortCloseAllVirtualSockets()) {
                    AbortAllVirtualSockets();
                }
                if (IsLinkAbortCloseAllServerSockets()) {
                    AbortAllServerSockets();
                }
            }
            if (socket) {
                socket->Close();
                socket->Dispose();
            }
            if (events) {
                OnAbort(EventArgs::Empty);
            }
        }

        int StageClient::ProcessAbort(Transitroute& e)
        {
            if (!Object::synchronize<bool>(GetSynchronizingObject(), [this] {
                if (IsDisposed()) {
                    return false;
                }
                if (_disposed) {
                    return false;
                }
                return true;
            })) {
                return -1;
            }
            int events = 0;
            std::shared_ptr<StageServerSocket> socket = AbortServerSocket(e.Platform, e.LinkType, e.ServerNo);
            if (socket) {
                events++;
            }
            return events;
        }

        int StageClient::ProcessAccept(Transitroute& e)
        {
            if (!Object::synchronize<bool>(GetSynchronizingObject(), [this] {
                if (IsDisposed()) {
                    return false;
                }
                if (_disposed) {
                    return false;
                }
                return true;
            })) {
                return -1;
            }
            AcceptClient ac;
            memset(&ac, 0, sizeof(ac));
            do {
                std::shared_ptr<BufferSegment> payload = e.Payload;
                if (!payload) {
                    break;
                }

                if ((Int64)payload->Length < (Int64)sizeof(ac)) {
                    break;
                }

                AcceptClient* pac = (AcceptClient*)payload->UnsafeAddrOfPinnedArray();
                if (NULL == pac) {
                    break;
                }

                memcpy(&ac, pac, sizeof(ac));
            } while (0, 0);
            int events = 0;
            std::shared_ptr<ServerSocketTable> socketTable;
            std::shared_ptr<StageServerSocket> socket = FindOrAddServerSocket(e.Platform, e.LinkType, e.ServerNo, socketTable, true, &ac);
            if (socket) {
                events++;
            }
            return events;
        }

        int StageClient::ProcessMessage(Transitroute& e, std::shared_ptr<Message>& message)
        {
            if (!Object::synchronize<bool>(GetSynchronizingObject(), [this] {
                if (IsDisposed()) {
                    return false;
                }
                if (_disposed) {
                    return false;
                }
                return true;
            })) {
                return -1;
            }
            int events = 0;
            std::shared_ptr<ServerSocketTable> socketTable;
            std::shared_ptr<StageServerSocket> socket = FindOrAddServerSocket(e.Platform, e.LinkType, e.ServerNo, socketTable, true, NULL);
            if (socket) {
                events++;
                socket->ProcessInput(message);
            }
            return events;
        }

        std::shared_ptr<StageServerSocket> StageClient::AddServerSocket(
            const std::string&                                                                                                  platform,
            Byte                                                                                                                linkType,
            int                                                                                                                 serverNo,
            std::shared_ptr<ServerSocketTable>&                                                                                 socketTable,
            const std::function<std::shared_ptr<StageServerSocket>(const std::string& platform, Byte linkType, int serverNo)>&  addValueFactory)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageServerSocket>();
            }
            std::shared_ptr<StageServerSocket> socket = make_shared_null<StageServerSocket>();
            std::shared_ptr<ServerSocketTable> stable = make_shared_null<ServerSocketTable>();
            do {
                auto ilpl = _serverSocketTable.find(platform);
                auto elpl = _serverSocketTable.end();
                if (ilpl == elpl) {
                    break;
                }

                auto& cllk = ilpl->second;
                if (cllk.empty()) {
                    break;
                }

                auto illk = cllk.find(linkType);
                auto ellk = cllk.end();
                if (illk == ellk) {
                    break;
                }

                stable = illk->second;
                if (!stable) {
                    break;
                }

                socket = stable->GetSocket(serverNo);
            } while (0, 0);
            do {
                if (socket) {
                    break;
                }

                if (!addValueFactory) {
                    break;
                }

                bool notable = stable ? false : true;
                if (notable) {
                    stable = CreateServerSocketTable(platform, linkType);
                    if (!stable) {
                        break;
                    }
                }

                socket = stable->AddSocket(serverNo, addValueFactory);
                if (!socket) {
                    stable = make_shared_null<ServerSocketTable>();
                    break;
                }

                if (notable) {
                    _serverSocketTable[platform][linkType] = stable;
                }
            } while (0, 0);
            socketTable = stable;
            return socket;
        }

        std::shared_ptr<StageServerSocket> StageClient::AbortServerSocket(const std::string& platform, Byte linkType, int serverNo)
        {
            std::shared_ptr<ServerSocketTable> stable = make_shared_null<ServerSocketTable>();
            std::shared_ptr<StageServerSocket> socket = make_shared_null<StageServerSocket>();
            do {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    return false;
                }
                auto ilpl = _serverSocketTable.find(platform);
                auto elpl = _serverSocketTable.end();
                if (ilpl == elpl) {
                    break;
                }

                auto& cllk = ilpl->second;
                if (cllk.empty()) {
                    _serverSocketTable.erase(ilpl);
                    break;
                }

                auto illk = cllk.find(linkType);
                auto ellk = cllk.end();
                if (illk == ellk) {
                    break;
                }
                else {
                    stable = illk->second;
                }

                bool destroyTable = false;
                if (!stable) {
                    destroyTable = true;
                }
                else {
                    Object::synchronize(stable->GetSynchronizingObject(), [&socket, &stable, &serverNo, &destroyTable] {
                        socket = stable->RemoveSocket(serverNo);
                        if (stable->GetCount() <= 0) {
                            destroyTable = true;
                        }
                    });
                }

                if (destroyTable) {
                    cllk.erase(illk);
                    if (cllk.empty()) {
                        _serverSocketTable.erase(ilpl);
                    }
                }
            } while (0, 0);
            
            if (socket) {
                socket->Close();
                socket->Dispose();
            }

            return socket;
        }

        int StageClient::AbortAllServerSockets()
        {
            int events = 0;
            ServerSocketPlatformTable platformTable;
            do {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    return ~0;
                }

                platformTable = _serverSocketTable;
                _serverSocketTable.clear();

                if (platformTable.empty()) {
                    return ~0;
                }
            } while (0, 0);
            for (auto platform : platformTable) {
                for (auto linkType : platform.second) {
                    std::shared_ptr<ServerSocketTable> socketTable = linkType.second;
                    if (!socketTable) {
                        continue;
                    }

                    std::vector<int> servers;
                    socketTable->GetAllServers(servers);
                    for (int serverNo : servers) {
                        std::shared_ptr<StageServerSocket> socket = socketTable->GetSocket(serverNo);
                        if (!socket) {
                            continue;
                        }
                        else {
                            events++;
                        }
                        socket->Close();
                        socket->Dispose();
                    }
                }
            }
            return events;
        }

        std::shared_ptr<StageClient::ServerSocketTable> StageClient::CreateServerSocketTable(const std::string& platform, Byte linkType)
        {
            return make_shared_object<ServerSocketTable>(platform, linkType);
        }

        std::shared_ptr<StageServerSocket> StageClient::CreateServerSocket(const std::string& platform, Byte linkType, int serverNo, UInt32 address, int port)
        {
            std::shared_ptr<StageClient> transmission = GetPtr();
            if (!transmission) {
                return make_shared_null<StageServerSocket>();
            }
            return make_shared_object<StageServerSocket>(transmission, platform, serverNo, linkType, address, port);
        }

        bool StageClient::IsAllowServerOfflinePackingOnly()
        {
            return true;
        }

        int StageClient::ProcessTransitroute(Transitroute& e, std::shared_ptr<Message>& message)
        {
#ifdef EPDEBUG
            printf("ProcessTransitroute: CommandId=%d, Platform=%s, ServerNo=%d, LinkType=%d\n", e.CommandId, e.Platform.data(), e.ServerNo, e.LinkType);
#endif
            switch (message->CommandId)
            {
            case model::Commands_AcceptClient:
                return ProcessAccept(e);
            case model::Commands_AbortClient:
                return ProcessAbort(e);
            default:
                return ProcessMessage(e, message);
            }
        }

        bool StageClient::ReshiftMessage(Transitroute& e, std::shared_ptr<Message>& message)
        {
            if (!message || !e.Load(message.get())) {
                return false;
            }
            message->CommandId = e.CommandId;
            message->SequenceNo = e.SequenceNo;
            message->Payload = e.Payload;
            return true;
        }

        bool StageClient::ProcessOpen(std::shared_ptr<StageSocket>& socket)
        {
            if (!socket) {
                return false;
            }
            bool events = false;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (!MarkAvailableChannel(socket)) {
                    return false;
                }
                else {
                    _absOpened = true;
                    if (_availableChannelsList.Count() <= 1) {
                        events = true;
                    }
                }
            } while (0, 0);
            if (events) { // 驱动投递全部发回的上下文
                _sendbackController.abortingTime = 0;
                PushAllSendback();
                OnOpen(EventArgs::Empty); // 驱动适配器打开事件处理器
            }
            return true;
        }

        bool StageClient::ProcessExchange(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message)
        {
            if (!socket || !message) {
                return false;
            }
            if (message->CommandId == model::Commands_AcceptClient) {
                TBulkManyMessage<AcceptClient> bulk;
                if (bulk.LoadFrom(*message.get())) {
                    return ProcessAccept(bulk) >= 0;
                }
            }
            else if (message->CommandId == model::Commands_Transitroute) {
                Transitroute transitroute;
                if (ReshiftMessage(transitroute, message)) {
                    return ProcessTransitroute(transitroute, message) >= 0;
                }
            }
            else {
                BulkManyMessage bulk;
                if (ReshiftMessage(bulk, message)) {
                    switch (bulk.CommandId) // 驱动命令状态机
                    {
                    case model::Commands_AbortClient:
                        return ProcessAbort(bulk) >= 0;
                    default:
                        return ProcessMessage(bulk, message) >= 0;
                    };
                }
            }
            return true;
        }

        int StageClient::ProcessAccept(TBulkManyMessage<AcceptClient>& e)
        {
            int events = 0;
            bool firstTimeAcceptClient = false;
            if (!Object::synchronize<bool>(GetSynchronizingObject(), [this, &firstTimeAcceptClient] {
                if (IsDisposed()) {
                    return false;
                }
                if (_disposed) {
                    return false;
                }
                else if (!_firstTimeAcceptClient) {
                    firstTimeAcceptClient = true;
                    _firstTimeAcceptClient = true;
                }
                return true;
            })) {
                return -1;
            }
            std::set<UInt64> availableLinkNoSet;
            for (int i = 0; i < e.MemberCount; i++) {
                AcceptClient& ac = e.Member[i];
                VirtualSocket socket = AddVirtualSocket(ac.LinkNo, [this, &ac](UInt64 handle) {
                    VirtualSocket socket = CreateVirtualSocket(handle, ac.Address, ac.Port);
                    if (socket) {
                        socket->OnOpen(EventArgs::Empty);
                        if (socket->IsDisposed()) {
                            socket = make_shared_null<StageVirtualSocket>();
                        }
                    }
                    return socket;
                });
                if (socket) {
                    events++;
                    if (firstTimeAcceptClient) {
                        availableLinkNoSet.insert(ac.LinkNo);
                    }
                }
            }
            if (firstTimeAcceptClient) {
                AbortAllOutsideVirtualSockets(availableLinkNoSet);
            }
            return events;
        }

        int StageClient::ProcessAbort(BulkManyMessage& e)
        {
            int events = 0;
            if (NULL == e.Member || !(e.MemberCount > 0)) {
                return events;
            }
            for (int i = 0; i < e.MemberCount; i++) { // 强制关闭虚套接字
                if (AbortVirtualSocket(e.Member[i], true)) {
                    events++;
                }
            }
            return events;
        }

        int StageClient::ProcessMessage(BulkManyMessage& e, std::shared_ptr<Message>& message)
        {
            int events = 0;
            if (NULL == e.Member || !(e.MemberCount > 0)) {
                return events;
            }
            for (int i = 0; i < e.MemberCount; i++) { // 驱动虚拟套接字
                VirtualSocket socket = GetVirtualSocket(e.Member[i]);
                if (!socket) {
                    continue;
                }
                events++;
                socket->ProcessInput(message);
            }
            return events;
        }

        StageClient::VirtualSocket StageClient::AbortVirtualSocket(UInt64 handle)
        {
            return AbortVirtualSocket(handle, false);
        }

        StageClient::VirtualSocket StageClient::AbortVirtualSocket(UInt64 handle, bool preventAbort)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageVirtualSocket>();
            }

            if (_virtualSocketTable.empty()) {
                return make_shared_null<StageVirtualSocket>();
            }

            auto il = _virtualSocketTable.find(handle);
            if (_virtualSocketTable.end() == il) {
                return make_shared_null<StageVirtualSocket>();
            }

            VirtualSocket socket = il->second;
            if (socket) {
                auto tail = _virtualSocketTable2.find(socket.get());
                if (tail != _virtualSocketTable2.end()) {
                    _virtualSocketTable2.erase(tail);
                }
            }

            if (_virtualSocketTable.end() != il) {
                _virtualSocketTable.erase(il);
                il = _virtualSocketTable.end();
            }

            if (socket) {
                if (preventAbort) {
                    socket->PreventAbort();
                }
                socket->Close();
                socket->Dispose();
            }
            return socket;
        }

        StageClient::VirtualSocket StageClient::GetVirtualSocket(UInt64 handle)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageVirtualSocket>();
            }

            if (_virtualSocketTable.empty()) {
                return make_shared_null<StageVirtualSocket>();
            }

            auto il = _virtualSocketTable.find(handle);
            auto el = _virtualSocketTable.end();
            if (el == il) {
                return make_shared_null<StageVirtualSocket>();
            }

            return il->second;
        }

        StageClient::VirtualSocket StageClient::GetVirtualSocket(const StageVirtualSocket* socket)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageVirtualSocket>();
            }

            if (_virtualSocketTable2.empty()) {
                return make_shared_null<StageVirtualSocket>();
            }

            auto il = _virtualSocketTable2.find((StageVirtualSocket*)socket);
            auto el = _virtualSocketTable2.end();
            if (el == il) {
                return make_shared_null<StageVirtualSocket>();
            }

            return il->second;
        }

        StageClient::VirtualSocket StageClient::AddVirtualSocket(UInt64 handle, std::function<VirtualSocket(UInt64 handle)> addValueFactory)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageVirtualSocket>();
            }

            auto il = _virtualSocketTable.find(handle);
            auto el = _virtualSocketTable.end();
            if (il != el) {
                return il->second;
            }

            if (!addValueFactory) {
                return make_shared_null<StageVirtualSocket>();
            }

            VirtualSocket socket = addValueFactory(handle);
            if (socket) {
                _virtualSocketTable[handle] = socket;
                _virtualSocketTable2[socket.get()] = socket;
            }

            return socket;
        }

        int StageClient::PushAllSendback()
        {
            int events = 0;
            for (int i = 0; i < _sendbackController.contextNum; i++) {
                do {
                    std::unique_lock<SyncObjectBlock> scope(_sendbackController.syncobj);
                    if (_sendbackController.contextNum <= 0) {
                        break;
                    }

                    SendbackContext& context = _sendbackController.contexts.front();
                    if (!RawSend(context.packet, context.counts, context.callback)) {
                        break;
                    }

                    // 对反应事件计数器进行计数
                    events++;

                    // 从托管的sendback控制器之中移除当前发出的报文
                    _sendbackController.contexts.pop_front();
                    _sendbackController.contextNum--;
                    _sendbackController.heapupSize -= context.counts;

                    // 回退发送上文计数小于0出现了严重问题
                    if (_sendbackController.contextNum < 0) {
                        assert(false);
                        _sendbackController.contextNum = 0;
                    }

                    // 回退发送堆积大小小于0出现了严重问题
                    if (_sendbackController.heapupSize < 0) {
                        assert(false);
                        _sendbackController.heapupSize = 0;
                    }
                } while (0, 0);
            }
            return events;
        }

        void StageClient::ResetAllSendback()
        {
            _sendbackController.Clear();
        }

        int StageClient::AbortAllVirtualSockets()
        {
            int events = 0;
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            do {
                std::vector<UInt64> handles;
                int counts = GetAllHandles(handles);
                bool cleanAll = false;
                do {
                    Message il;
                    il.CommandId = model::Commands_AbortAllClient;
                    il.SequenceNo = Message::NewId();
                    cleanAll = RawSend(il, NULL);
                } while (0, 0);
                for (int i = 0; i < counts; i++) {
                    UInt64 handle = handles[i];
                    if (AbortVirtualSocket(handle, cleanAll)) {
                        events++;
                    }
                }
            } while (0, 0);
            return events;
        }

        bool StageClient::ContainsVirtualSocket(UInt64 handle)
        {
            VirtualSocket socket = GetVirtualSocket(handle);
            return NULL != socket.get();
        }

        bool StageClient::ContainsVirtualSocket(const StageVirtualSocket* socket)
        {
            VirtualSocket po = GetVirtualSocket(socket);
            return NULL != po.get();
        }

        StageClient::VirtualSocket StageClient::CreateVirtualSocket(UInt64 handle, UInt32 address, int port)
        {
            if (!this) {
                return make_shared_null<StageVirtualSocket>();
            }

            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return make_shared_null<StageVirtualSocket>();
            }

            std::shared_ptr<StageClient> transmission = GetPtr();
            if (!transmission) {
                return make_shared_null<StageVirtualSocket>();
            }

            return make_shared_object<StageVirtualSocket>(transmission, handle, address, port);
        }

        bool StageClient::ReshiftMessage(BulkManyMessage& bulk, std::shared_ptr<Message>& message)
        {
            if (!StageSocketCommunication::LoadBulkMany(bulk, message)) {
                return false;
            }
            std::shared_ptr<BufferSegment> segment = message->Payload;
            if (!segment) {
                return false;
            }
            if (!bulk.PayloadPtr || !bulk.PayloadSize) { // 重新修正投递给上层的消息流偏移量与载荷数量
                segment->Offset = 0;
                segment->Length = 0;
                segment->Buffer.reset();
            }
            else {
                Int64 offset = bulk.PayloadPtr - segment->Buffer.get();
                if (offset >= segment->Length) {
                    segment->Length = 0;
                    segment->Offset = 0;
                    segment->Buffer.reset();
                }
                else {
                    segment->Offset += offset;
                    segment->Length -= offset;
                }
            }
            return true;
        }

        void StageClient::OnOpen(EventArgs& e)
        {
            OpenEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = OpenEvent;
            } while (0, 0);
            if (events) {
                auto self = GetPtr();
                events(self, e);
            }
        }

        void StageClient::OnAbort(EventArgs& e)
        {
            AbortEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = AbortEvent;
            } while (0, 0);
            if (events) {
                auto self = GetPtr();
                events(self, e);
            }
        }

        void StageClient::OnMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message)
        {
            MessageEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = MessageEvent;
            } while (0, 0);
            if (events) {
                auto self = GetPtr();
                events(self, message);
            }
        }

        void StageClient::OnOpen(std::shared_ptr<StageVirtualSocket>& socket, EventArgs& e)
        {
            VirtualSocketOpenEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = VirtualSocketOpenEvent;
            } while (0, 0);
            if (events) {
                events(socket, e);
            }
        }

        void StageClient::OnAbort(std::shared_ptr<StageVirtualSocket>& socket, EventArgs& e)
        {
            VirtualSocketAbortEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = VirtualSocketAbortEvent;
            } while (0, 0);
            if (events) {
                events(socket, e);
            }
        }

        void StageClient::OnMessage(std::shared_ptr<StageVirtualSocket>& socket, std::shared_ptr<Message>& message)
        {
            VirtualSocketMessageEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = VirtualSocketMessageEvent;
            } while (0, 0);
            if (events) {
                events(socket, message);
            }
        }

        void StageClient::OnOpen(std::shared_ptr<StageServerSocket>& socket, EventArgs& e)
        {
            ServerSocketOpenEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = ServerSocketOpenEvent;
            } while (0, 0);
            if (events) {
                events(socket, e);
            }
        }

        void StageClient::OnAbort(std::shared_ptr<StageServerSocket>& socket, EventArgs& e)
        {
            ServerSocketAbortEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = ServerSocketAbortEvent;
            } while (0, 0);
            if (events) {
                events(socket, e);
            }
        }
        
        void StageClient::OnMessage(std::shared_ptr<StageServerSocket>& socket, std::shared_ptr<Message>& message)
        {
            ServerSocketMessageEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                events = ServerSocketMessageEvent;
            } while (0, 0);
            if (events) {
                events(socket, message);
            }
        }

        int StageClient::GetServerNo()
        {
            return _serverNo;
        }

        void StageClient::Dispose()
        {
            std::exception* exception = NULL;
            bool cleanUp = false;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (_tickAlways) {
                    _tickAlways->Stop();
                    _tickAlways->Dispose();
                }
                _tickAlways.reset();
                if (!_disposed) {
                    cleanUp = true;
                    auto shadowsChannelTable = _workingChannelsTable;
                    for (auto il : shadowsChannelTable) {
                        std::shared_ptr<SocketLinkedListNode> socketNode = il.second;
                        if (!socketNode) {
                            continue;
                        }
                        SocketObject socket = socketNode->Value;
                        if (socket) {
                            socket->Close();
                            socket->Dispose();
                        }
                    }
                    _freeChannelNode.clear();
                    _workingChannelsTable.clear();
                    if (!_virtualSocketTable.empty()) { // 关闭全部客户端套接字(虚链路)
                        auto shadowsVirtualSocketTable = _virtualSocketTable;
                        for (auto il : shadowsVirtualSocketTable) { 
                            AbortVirtualSocket(il.first, true); // nothing-call-AbortAllVirtualSockets();
                        }
                    }
                    _virtualSocketTable.clear();
                    _virtualSocketTable2.clear();
                    if (!_serverSocketTable.empty()) { // 关闭全部服务器套接字(虚链路)
                        AbortAllServerSockets();
                    }
                    _serverSocketTable.clear();
                    if (_hosting) {
                        _hosting.reset();
                    }
                    if (_socketHandler) {
                        _socketHandler.reset();
                    }
                    bool established = _availableChannelsList.Count() > 0;
                    if (_availableChannelsList.Count() > 0) { // 彻底关闭物理链路的通道组
                        auto node = _availableChannelsList.First();
                        while (node) {
                            SocketObject socket = node->Value;
                            node = node->Next;
                            if (socket) {
                                socket->Close();
                                socket->Dispose();
                            }
                        }
                    }
                    _availableChannelsList.Clear();
                    _currentAvailableChannels = NULL;
                    if (established) { // 设链路处理建立的情况则触发链路中断事件（只脉冲一次状态）
                        try {
                            OnAbort(EventArgs::Empty);
                        }
                        catch (std::exception& e) {
                            exception = Object::addressof(e);
                        }
                    }
                    AbortEvent = NULL;
                    OpenEvent = NULL;
                    MessageEvent = NULL;
                    ServerSocketOpenEvent = NULL;
                    ServerSocketAbortEvent = NULL;
                    ServerSocketMessageEvent = NULL;
                    VirtualSocketAbortEvent = NULL;
                    VirtualSocketOpenEvent = NULL;
                    VirtualSocketMessageEvent = NULL;
                }
                _disposed = true;
                _listening = false;
            } while (0, 0);
            if (cleanUp) {
                ResetAllSendback();
            }
            if (NULL != exception) {
                throw *exception;
            }
        }

        int StageClient::Listen()
        {
            return AddAndOpenReleaseAllChannels(0);
        }

        int StageClient::GetSvrAreaNo()
        {
            return _svrAreaNo;
        }

        const std::string& StageClient::GetPlatform()
        {
            return _platform;
        }

        int StageClient::GetMaxFiledescriptor()
        {
            return _maxFiledescriptor;
        }

        std::shared_ptr<StageClient> StageClient::GetPtr()
        {
            StageClient* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<StageClient>();
        }

        Byte StageClient::GetLinkType()
        {
            return _chLinkType;
        }

        Byte StageClient::GetKf()
        {
            return Socket::ServerKf;
        }

        StageClient::SendbackSettings* StageClient::GetSendbackSettings()
        {
            return Object::addressof(_sendbackController.settings);
        }

        bool StageClient::Send(BulkManyMessage& message, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            unsigned int counts = 0;
            std::shared_ptr<unsigned char> packet = message.ToPacket(counts);
            if (!packet || 0 == counts) {
                return false;
            }

            return Send(packet, counts, callback);
        }

        bool StageClient::Send(Byte handleType, std::vector<unsigned short>& handles, Commands commandId, UInt32 ackNo, const unsigned char* payload, UInt32 length, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            if (handles.empty()) {
                return false;
            }

            if (NULL == payload && length != 0) {
                return false;
            }

            BulkManyMessage message;
            message.CommandId = commandId;
            message.SequenceNo = ackNo;
            message.Kf = GetKf();
            message.MemberType = handleType;
            message.Member = &(handles[0]);
            message.MemberCount = handles.size();
            message.PayloadSize = length;
            message.PayloadPtr = (unsigned char*)payload;
            return Send(message, callback);
        }

        bool StageClient::Send(Transitroute& transitroute, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            unsigned int counts = 0;
            std::shared_ptr<unsigned char> packet = transitroute.ToPacket(GetKf(), model::Commands_Transitroute, Message::NewId(), counts);
            if (!packet || 0 == counts) {
                return false;
            }

            return Send(packet, counts, callback);
        }

        bool StageClient::Send(std::shared_ptr<unsigned char>& packet, size_t counts, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            if (RawSend(packet, counts, callback)) { // 已提交到内核PCB队列发送时返回真
                return true;
            }

            std::unique_lock<SyncObjectBlock> scope(_sendbackController.syncobj);

            // 检查重新提交内核PCB队列限定设置
            SendbackSettings* settings = GetSendbackSettings();
            if (NULL == settings) {
                return false;
            }

            int thoroughlyAbandonTime = settings->GetThoroughlyAbandonTime();
            if (0 == thoroughlyAbandonTime) {
                return false;
            }
            else if (thoroughlyAbandonTime > 0) {
                Int64 abortingTime = _sendbackController.abortingTime;
                Int64 currentTime = Hosting::GetTickCount();
                if (abortingTime > currentTime || (currentTime - abortingTime) >= thoroughlyAbandonTime) {
                    return false;
                }
            }

            int maxSendQueueSize = _sendbackController.contextNum + 1;
            if (maxSendQueueSize > settings->GetMaxSendQueueSize()) {
                return false;
            }

            Int64 maxSendBufferSize = _sendbackController.heapupSize + counts;
            if (maxSendBufferSize > settings->GetMaxSendBufferSize()) {
                return false;
            }

            // 提交到内核PCB队列过程中出现了问题，缓存发送内容到队列之中
            SendbackContext context;
            context.counts = counts;
            context.packet = packet;
            context.callback = callback;

            _sendbackController.contextNum = maxSendQueueSize;
            _sendbackController.heapupSize = maxSendBufferSize;
            _sendbackController.contexts.push_back(context);
            return true;
        }

        bool StageClient::RawSend(Message& message, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            int max = Object::synchronize<int>(GetSynchronizingObject(), [this] {
                int l = _availableChannelsList.Count();
                if (l <= 0) {
                    l = 0;
                }
                return l;
            });

            for (int i = 0; i < max; i++) {
                std::shared_ptr<StageSocket> socket = NextAvailableChannel();
                if (!socket) {
                    continue;
                }

                if (socket->Send(message, callback)) {
                    return true;
                }
            }
            return false;
        }

        bool StageClient::RawSend(std::shared_ptr<unsigned char>& packet, size_t counts, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            if (IsDisposed()) {
                return false;
            }

            int max = Object::synchronize<int>(GetSynchronizingObject(), [this] {
                int l = _availableChannelsList.Count();
                if (l <= 0) {
                    l = 0;
                }
                return l;
            });

            for (int i = 0; i < max; i++) {
                std::shared_ptr<StageSocket> socket = NextAvailableChannel();
                if (!socket) {
                    continue;
                }

                if (socket->Send(packet, counts, callback)) {
                    return true;
                }
            }
            return false;
        }

        std::shared_ptr<StageServerSocket> StageClient::FindOrAddServerSocket(const std::string& platform, Byte linkType, int serverNo, std::shared_ptr<ServerSocketTable>& socketTable, bool allowAdding, AcceptClient* pac)
        {
            std::shared_ptr<StageServerSocket> socket = make_shared_null<StageServerSocket>();
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return socket;
            }
            do {
                auto po = GetServerSocketTable(platform, linkType);
                if (po) {
                    socket = po->GetSocket(serverNo);
                }
            } while (0, 0);
            if (socket) {
                do {
                    if (NULL == pac) {
                        break;
                    }

                    if (0 == pac->Address) {
                        break;
                    }

                    if (0 == pac->Port) {
                        break;
                    }

                    socket->RebindEndPoint(pac->Address, pac->Port);
                } while (0, 0);
            }
            else {
                if (allowAdding || IsAllowServerOfflinePackingOnly()) {
                    AcceptClient ac;
                    if (NULL != pac) {
                        ac = *pac;
                    }
                    else {
                        memset(&ac, 0, sizeof(ac));
                    }
                    socket = AddServerSocket(platform, linkType, serverNo, socketTable, [&ac, this](const std::string& platform, Byte linkType, int serverNo) {
                        std::shared_ptr<StageServerSocket> socket = CreateServerSocket(platform, linkType, serverNo, ac.Address, ac.Port);
                        if (socket) {
                            socket->OnOpen(EventArgs::Empty);
                            if (socket->IsDisposed()) {
                                socket = make_shared_null<StageServerSocket>();
                            }
                        }
                        return socket;
                    });
                }
            }
            return socket;
        }

        bool StageClient::PreventAcceptClient()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            if (_allowAccept) {
                Message il;
                il.CommandId = model::Commands_PreventAccept;
                il.SequenceNo = Message::NewId();
                if (!RawSend(il, NULL)) {
                    return false;
                }
                _allowAccept = false;
            }
            return true;
        }

        bool StageClient::PermittedAcceptClient()
        {
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            if (!_allowAccept) {
                Message il;
                il.CommandId = model::Commands_PermittedAccept;
                il.SequenceNo = Message::NewId();
                if (!RawSend(il, NULL)) {
                    return false;
                }
                _allowAccept = true;
            }
            return true;
        }

        bool StageClient::IsAllowAcceptClient()
        {
            return _allowAccept;
        }

        boost::asio::ip::tcp::endpoint StageClient::GetLocalEndPoint()
        {
            return _localEP;
        }

        boost::asio::ip::tcp::endpoint StageClient::GetRemoteEndPoint()
        {
            return _remoteEP;
        }

        const std::string& StageClient::ServerSocketTable::GetPlatform()
        {
            return _platform;
        }

        Byte StageClient::ServerSocketTable::GetLinkType()
        {
            return _linkType;
        }

        int StageClient::ServerSocketTable::GetCount()
        {
            if (!this) {
                return 0;
            }
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (_disposed) {
                _sockets.clear();
                return 0;
            }
            return _sockets.size();
        }

        void StageClient::ServerSocketTable::RemoveAllSockets()
        {
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                _sockets.clear();
            }
        }

        int StageClient::ServerSocketTable::GetAllServers(std::vector<int>& servers)
        {
            int events = 0;
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    if (!_sockets.empty()) {
                        _sockets.clear();
                    }
                }
                else {
                    auto il = _sockets.begin();
                    auto el = _sockets.end();
                    for (; il != el; ++il) {
                        events++;
                        servers.push_back(il->first);
                    }
                }
            }
            return events;
        }

        int StageClient::ServerSocketTable::GetAllSockets(std::vector<std::shared_ptr<StageServerSocket>/**/>& sockets)
        {
            int events = 0;
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    if (!_sockets.empty()) {
                        _sockets.clear();
                    }
                }
                else {
                    auto il = _sockets.begin();
                    auto el = _sockets.end();
                    for (; il != el; ++il) {
                        events++;
                        sockets.push_back(il->second);
                    }
                }
            }
            return events;
        }

        SyncObjectBlock& StageClient::ServerSocketTable::GetSynchronizingObject()
        {
            return _syncobj;
        }

        std::shared_ptr<StageServerSocket> StageClient::ServerSocketTable::GetSocket(int serverNo)
        {
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    if (!_sockets.empty()) {
                        _sockets.clear();
                    }
                }
                else {
                    auto il = _sockets.find(serverNo);
                    auto el = _sockets.end();
                    if (il != el) {
                        return il->second;
                    }
                }
            }
            return make_shared_null<StageServerSocket>();
        }

        bool StageClient::ServerSocketTable::ContainsSocket(int serverNo)
        {
            if (!this) {
                return false;
            }
            std::shared_ptr<StageServerSocket> socket = GetSocket(serverNo);
            if (socket) {
                return true;
            }
            return false;
        }

        std::shared_ptr<StageServerSocket> StageClient::ServerSocketTable::RemoveSocket(int serverNo)
        {
            std::shared_ptr<StageServerSocket> po = make_shared_null<StageServerSocket>();
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    if (!_sockets.empty()) {
                        _sockets.clear();
                    }
                }
                else {
                    auto il = _sockets.find(serverNo);
                    auto el = _sockets.end();
                    if (il != el) {
                        po = il->second;
                        _sockets.erase(il);
                    }
                }
            }
            return po;
        }

        bool StageClient::ServerSocketTable::IsEmpty()
        {
            if (!this) {
                return true;
            }
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            if (_sockets.empty()) {
                return true;
            }
            if (_disposed) {
                _sockets.clear();
                return true;
            }
            return false;
        }

        bool StageClient::ServerSocketTable::IsDisposed()
        {
            if (!this) {
                return true;
            }
            std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
            return _disposed;
        }

        std::shared_ptr<StageClient::ServerSocketTable> StageClient::ServerSocketTable::GetPtr()
        {
            if (this) {
                return shared_from_this();
            }
            return make_shared_null<ServerSocketTable>();
        }

        void StageClient::ServerSocketTable::Dispose()
        {
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (!_disposed) {
                    RemoveAllSockets();
                }
                _disposed = true;
            }
        }

        std::shared_ptr<StageServerSocket> StageClient::ServerSocketTable::AddSocket(
            int                                                                                                                 serverNo,
            const std::function<std::shared_ptr<StageServerSocket>(const std::string& platform, Byte linkType, int serverNo)>&  addValueFactory)
        {
            if (this) {
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    if (!_sockets.empty()) {
                        _sockets.clear();
                    }
                }
                else {
                    auto il = _sockets.find(serverNo);
                    auto el = _sockets.end();
                    if (il != el) {
                        return il->second;
                    }

                    if (addValueFactory) {
                        std::shared_ptr<StageServerSocket> socket = addValueFactory(GetPlatform(), GetLinkType(), serverNo);
                        if (socket) {
                            _sockets[serverNo] = socket;
                            return socket;
                        }
                    }
                }
            }
            return make_shared_null<StageServerSocket>();
        }

        StageClient::ServerSocketTable::ServerSocketTable(const std::string& platform, Byte linkType)
            : _disposed(false)
            , _linkType(linkType)
            , _platform(platform)
        {

        }

        StageClient::ServerSocketTable::~ServerSocketTable()
        {
            if (this) {
                Dispose();
            }
        }

        StageClient::SendbackSettings::SendbackSettings()
        {
            Clear();
        }

        StageClient::SendbackSettings::~SendbackSettings()
        {
            Clear();
        }

        void StageClient::SendbackSettings::Clear()
        {
            _maxSendQueueSize = 0;
            _maxSendBufferSize = 0;
            _thoroughlyAbandonTime = ~0;
        }
    }
}