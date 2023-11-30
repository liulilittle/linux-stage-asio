#pragma once

#include "util/Util.h"
#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/LinkedList.h"
#include "util/stage/net/Message.h"
#include "StageSocket.h"
#include "IStageSocketHandler.h"
#include "StageSocketListener.h"

#include <list>
#include <vector>

namespace ep
{
    namespace stage
    {
        class StageClient;
        class StageServerSocket;
        class StageVirtualSocket;

        class StageClient : public std::enable_shared_from_this<StageClient> // STAGE�ͻ���������
        {
            friend class StageServerSocket;
            friend class StageVirtualSocket;

        public:
            typedef std::function<void(std::shared_ptr<StageClient>& sender, EventArgs& e)>                                             AbortEventHandler;
            typedef std::function<void(std::shared_ptr<StageClient>& sender, EventArgs& e)>                                             OpenEventHandler;
            typedef std::function<void(std::shared_ptr<StageClient>& sender, std::shared_ptr<Message>& e)>                              MessageEventHandler;
            typedef ISocket::SendAsyncCallback                                                                                          SendAsyncCallback;

        public:
            typedef std::function<void(std::shared_ptr<StageServerSocket>& sender, EventArgs& e)>                                       ServerSocketAbortEventHandler;
            typedef std::function<void(std::shared_ptr<StageServerSocket>& sender, EventArgs& e)>                                       ServerSocketOpenEventHandler;
            typedef std::function<void(std::shared_ptr<StageServerSocket>& sender, std::shared_ptr<Message>& e)>                        ServerSocketMessageEventHandler;

        public:
            typedef std::function<void(std::shared_ptr<StageVirtualSocket>& sender, EventArgs& e)>                                      VirtualSocketAbortEventHandler;
            typedef std::function<void(std::shared_ptr<StageVirtualSocket>& sender, EventArgs& e)>                                      VirtualSocketOpenEventHandler;
            typedef std::function<void(std::shared_ptr<StageVirtualSocket>& sender, std::shared_ptr<Message>& e)>                       VirtualSocketMessageEventHandler;

        public:
            class ServerSocketTable : public std::enable_shared_from_this<ServerSocketTable>
            {
                friend class StageClient;

            public:
                ServerSocketTable(const std::string& platform, Byte linkType);
                virtual ~ServerSocketTable();

            public:
                virtual void                                                                                                            RemoveAllSockets();
                virtual SyncObjectBlock&                                                                                                GetSynchronizingObject();
                virtual std::shared_ptr<ServerSocketTable>                                                                              GetPtr();
                virtual void                                                                                                            Dispose();

            public:
                virtual int                                                                                                             GetCount();
                virtual bool                                                                                                            IsEmpty();
                virtual bool                                                                                                            IsDisposed();
                virtual const std::string&                                                                                              GetPlatform();
                virtual Byte                                                                                                            GetLinkType();

            public:
                virtual std::shared_ptr<StageServerSocket>                                                                              GetSocket(int serverNo);
                virtual bool                                                                                                            ContainsSocket(int serverNo);
                virtual std::shared_ptr<StageServerSocket>                                                                              RemoveSocket(int serverNo);
                virtual std::shared_ptr<StageServerSocket>                                                                              AddSocket(
                    int                                                                                                                 serverNo,
                    const std::function<std::shared_ptr<StageServerSocket>(const std::string& platform, Byte linkType, int serverNo)>&  addValueFactory);
                virtual int                                                                                                             GetAllServers(std::vector<int>& servers);
                virtual int                                                                                                             GetAllSockets(std::vector<std::shared_ptr<StageServerSocket>/**/>& sockets);

            private:
                bool                                                                                                                    _disposed;
                Byte                                                                                                                    _linkType;
                std::string                                                                                                             _platform;
                SyncObjectBlock                                                                                                         _syncobj;
                HASHMAPDEFINE(int, std::shared_ptr<StageServerSocket>/**/)                                                              _sockets;
            };

        public:
#pragma pack(push, 1)
            struct AcceptClient
            {
            public:
                unsigned short                                                                                                          LinkNo;
                unsigned short                                                                                                          Port;
                unsigned int                                                                                                            Address;
            };
#pragma pack(pop)
            typedef std::shared_ptr<StageSocket>                                                                                        SocketObject;
            typedef std::shared_ptr<StageVirtualSocket>                                                                                 VirtualSocket;
            typedef LinkedList<SocketObject>                                                                                            SocketLinkedList;
            typedef LinkedListNode<SocketObject>                                                                                        SocketLinkedListNode;
            typedef HASHMAPDEFINE(StageSocket*, std::shared_ptr<SocketLinkedListNode>/**/)                                              SocketObjectTable;
            typedef HASHMAPDEFINE(UInt32, VirtualSocket)                                                                                VirtualSocketTable;
            typedef HASHMAPDEFINE(StageVirtualSocket*, VirtualSocket)                                                                   VirtualSocketTable2;

        public:
            class SendbackSettings
            {
            private:
                mutable int                                                                                                             _maxSendQueueSize;
                mutable Int64                                                                                                           _maxSendBufferSize;
                mutable int                                                                                                             _thoroughlyAbandonTime;

            public:
                SendbackSettings();
                virtual ~SendbackSettings();

            public:
                virtual void                                                                                                            Clear();

            public:
                virtual SendbackSettings&                                                                                               SetMaxSendQueueSize(int value) {
                    if (value < 0) {
                        value = 0;
                    }
                    _maxSendQueueSize = value;
                    return *this;
                }
                virtual SendbackSettings&                                                                                               SetMaxSendBufferSize(Int64 value) {
                    if (value < 0) {
                        value = 0;
                    }
                    _maxSendBufferSize = value;
                    return *this;
                }
                virtual SendbackSettings&                                                                                               SetThoroughlyAbandonTime(int value) {
                    if (value < 0) {
                        value = ~0;
                    }
                    _thoroughlyAbandonTime = value;
                    return *this;
                }

            public:
                virtual const int&                                                                                                      GetMaxSendQueueSize() { return _maxSendQueueSize; }
                virtual const Int64&                                                                                                    GetMaxSendBufferSize() { return _maxSendBufferSize; }
                virtual const int&                                                                                                      GetThoroughlyAbandonTime() { return _thoroughlyAbandonTime; }
            };

        private:
            struct SendbackContext
            {
            public:
                std::shared_ptr<unsigned char>                                                                                          packet;
                size_t                                                                                                                  counts;
                SendAsyncCallback                                                                                                       callback;

            public:
                inline SendbackContext()
                {
                    Clear();
                }

            public:
                inline void                                                                                                             Clear()
                {
                    counts = 0;
                    callback = NULL;
                    packet = make_shared_null<unsigned char>();
                }
            };
            typedef std::list<SendbackContext>                                                                                          SendbackContextVector;
            struct SendbackController
            {
            public:
                Int64                                                                                                                   heapupSize;
                int                                                                                                                     contextNum;
                SendbackContextVector                                                                                                   contexts;
                SyncObjectBlock                                                                                                         syncobj;
                SendbackSettings                                                                                                        settings;
                Int64                                                                                                                   abortingTime;

            public:
                inline SendbackController() {
                    Clear(true);
                }

            public:
                inline void                                                                                                             Clear(bool resetAbortingTime = false) {
                    std::unique_lock<SyncObjectBlock> scoped(syncobj);
                    heapupSize = 0;
                    contextNum = 0;
                    if (resetAbortingTime) {
                        abortingTime = 0;
                    }
                    contexts.clear();
                }
            };
            typedef HASHMAPDEFINE(Byte, std::shared_ptr<ServerSocketTable>/**/)                                                         ServerSocketLinkTypeTable;
            typedef HASHMAPDEFINE(std::string, ServerSocketLinkTypeTable)                                                               ServerSocketPlatformTable;

        public:
            //************************************
            // Method:    StageClient
            // FullName:  ::global::ep::stage::StageClient::StageClient
            // Access:    public / internal
            // Returns:   
            // Qualifier:
            // Parameter: const std::shared_ptr<Hosting> & hosting                                                                      ���б��ͻ����������ı�������ʵ��
            // Parameter: const std::string & hostName                                                                                  ����Զ��������NS/AAAA����������̫��IPv4��ʽ���ֵ�ַ
            // Parameter: int port                                                                                                      ����Զ�������Ķ˿ڵ�ַ������1 ~ 65535֮�䣩
            // Parameter: Byte linkType                                                                                                 ָ����ǰ����������������(ͨ��Լ��Ϊ0)
            // Parameter: const std::string & platform                                                                                  ����ƽ̨
            // Parameter: int svrAreaNo                                                                                                 ����������������
            // Parameter: int serverNo                                                                                                  ����ƽ̨���������
            // Parameter: int maxFiledescriptor                                                                                         ָʾ�����ļ������������������ڼ�����(backlog)������Ŀ
            // Parameter: int maxConcurrent                                                                                             ͬʱ�������ָ����ͬһ��ʱ���������ִ�е�ָ��
            //************************************
            StageClient(
                const std::shared_ptr<Hosting>&                                                                                         hosting,
                const std::string&                                                                                                      hostName,
                int                                                                                                                     port,
                Byte                                                                                                                    linkType,
                const std::string&                                                                                                      platform,
                int                                                                                                                     svrAreaNo,
                int                                                                                                                     serverNo,
                int                                                                                                                     maxFiledescriptor,
                int                                                                                                                     maxConcurrent);
            virtual ~StageClient();

        public:
            Reference                                                                                                                   Tag;
            Reference                                                                                                                   UserToken;

        public:
            virtual SyncObjectBlock&                                                                                                    GetSynchronizingObject();
            virtual int                                                                                                                 GetAvailableChannels();
            virtual int                                                                                                                 GetServerNo();
            virtual int                                                                                                                 GetSvrAreaNo();
            virtual const std::string&                                                                                                  GetPlatform();
            virtual int                                                                                                                 GetMaxFiledescriptor();
            virtual std::shared_ptr<StageClient>                                                                                        GetPtr();
            virtual Byte                                                                                                                GetLinkType();
            virtual Byte                                                                                                                GetKf();
            virtual SendbackSettings*                                                                                                   GetSendbackSettings();

        public:
            virtual bool                                                                                                                Send(BulkManyMessage& message, const SendAsyncCallback& callback);
            virtual bool                                                                                                                Send(
                Byte                                                                                                                    handleType,
                std::vector<unsigned short>&                                                                                            handles, 
                Commands                                                                                                                commandId, 
                UInt32                                                                                                                  ackNo,
                const unsigned char*                                                                                                    payload, 
                UInt32                                                                                                                  length, 
                const SendAsyncCallback&                                                                                                callback);
            inline bool                                                                                                                 Send( // �Բ������Ǹ����ˣ�
                std::vector<unsigned short>&                                                                                            handles,
                Commands                                                                                                                commandId,
                UInt32                                                                                                                  ackNo,
                const SendAsyncCallback&                                                                                                callback)
            {
                return Send(handles, commandId, ackNo, NULL, 0, callback);
            }
            inline bool                                                                                                                 Send( // �Բ������Ǹ����ˣ�
                std::vector<unsigned short>&                                                                                            handles,
                Commands                                                                                                                commandId,
                const SendAsyncCallback&                                                                                                callback)
            {
                return Send(handles, commandId, Message::NewId(), callback);
            }
            inline bool                                                                                                                 Send(std::vector<unsigned short>& handles, Message& message, const SendAsyncCallback& callback)
            {
                if (!this) {
                    return false;
                }

                unsigned char* payload = NULL;
                unsigned int counts = 0;

                std::shared_ptr<BufferSegment> segment = message.Payload;
                if (segment) {
                    if (segment->Length < 0 || segment->Offset < 0) {
                        return false;
                    }

                    counts = segment->Length;
                    payload = segment->UnsafeAddrOfPinnedArray();
                }
                return Send(handles, (Commands)message.CommandId, message.SequenceNo, payload, counts, callback);
            }
            inline bool                                                                                                                 Send(
                std::vector<unsigned short>&                                                                                            handles,
                Commands                                                                                                                commandId,
                UInt32                                                                                                                  ackNo,
                const unsigned char*                                                                                                    payload,
                UInt32                                                                                                                  length,
                const SendAsyncCallback&                                                                                                callback)
            {
                return Send(GetLinkType(), handles, commandId, ackNo, payload, length, callback);
            }
            virtual bool                                                                                                                Send(Transitroute& transitroute, const SendAsyncCallback& callback);

        protected:
            virtual bool                                                                                                                Send(std::shared_ptr<unsigned char>& packet, size_t counts, const SendAsyncCallback& callback);
            virtual bool                                                                                                                RawSend(Message& message, const SendAsyncCallback& callback);

        public:
            virtual bool                                                                                                                PreventAcceptClient();      // ������ֹ���ܿͻ���
            virtual bool                                                                                                                PermittedAcceptClient();    // ����������ܿͻ���
            virtual bool                                                                                                                IsAllowAcceptClient();      // �Ƿ�������ܿͻ���

        public:
            virtual boost::asio::ip::tcp::endpoint                                                                                      GetLocalEndPoint();
            virtual boost::asio::ip::tcp::endpoint                                                                                      GetRemoteEndPoint();

        public:
            template<typename TCollection>
            inline static int                                                                                                           GetAllHandles(TCollection& s, std::vector<unsigned short>& handles) 
            {
                typename TCollection::iterator il = s.begin();
                typename TCollection::iterator el = s.end();

                int events = 0;
                for (; il != el; ++il) {
                    auto socket = *il;
                    if (!socket) {
                        continue;
                    }

                    events++;
                    handles.push_back(socket->GetHandle());
                }
                return events;
            }
            template<typename TVector>
            inline int                                                                                                                  GetAllHandles(TVector& s)
            {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

                VirtualSocketTable::iterator il = _virtualSocketTable.begin();
                VirtualSocketTable::iterator el = _virtualSocketTable.end();

                int events = 0;
                for (; il != el; ++il) {
                    events++;
                    s.push_back(il->first);
                }
                return events;
            }
            template<typename TSet>
            inline int                                                                                                                  GetAllHandles2(TSet& s)
            {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

                VirtualSocketTable::iterator il = _virtualSocketTable.begin();
                VirtualSocketTable::iterator el = _virtualSocketTable.end();

                int events = 0;
                for (; il != el; ++il) {
                    events++;
                    s.insert(il->first);
                }
                return events;
            }

        public:
            virtual int                                                                                                                 AbortAllVirtualSockets();
            virtual VirtualSocket                                                                                                       AbortVirtualSocket(UInt64 handle);
            virtual VirtualSocket                                                                                                       GetVirtualSocket(UInt64 handle);
            virtual VirtualSocket                                                                                                       GetVirtualSocket(const StageVirtualSocket* socket);
            virtual bool                                                                                                                ContainsVirtualSocket(UInt64 handle);
            virtual bool                                                                                                                ContainsVirtualSocket(const StageVirtualSocket* socket);
            virtual VirtualSocket                                                                                                       CreateVirtualSocket(UInt64 handle, UInt32 address, int port);

        protected:
            virtual VirtualSocket                                                                                                       AddVirtualSocket(UInt64 handle, std::function<VirtualSocket(UInt64 handle)> addValueFactory);
            template<typename TSet>
            inline int                                                                                                                  AbortAllOutsideVirtualSockets(TSet& s)
            {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

                int events = 0;
                auto il = _virtualSocketTable.begin();
                auto el = _virtualSocketTable.end();
                auto tl = s.end();

                std::vector<UInt64> handles;
                for (; il != el; ++il) {
                    UInt64 linkNo = il->first;
                    auto it = s.find(linkNo);
                    if (it != tl) {
                        continue;
                    }
                    handles.push_back(linkNo);
                }

                for (unsigned int i = 0, counts = handles.size(); i < counts; i++) {
                    UInt64 linkNo = handles[i];
                    VirtualSocket socket = AbortVirtualSocket(linkNo);
                    if (socket) {
                        events++;
                    }
                }
                return events;
            }
            virtual int                                                                                                                 PushAllSendback();
            virtual void                                                                                                                ResetAllSendback();
            virtual VirtualSocket                                                                                                       AbortVirtualSocket(UInt64 handle, bool preventAbort);

        public:
            void                                                                                                                        Close();
            virtual void                                                                                                                Dispose();
            virtual int                                                                                                                 Listen();
            virtual int                                                                                                                 GetAvailableChannels(std::set<SocketObject>& s);
            virtual bool                                                                                                                IsSecurityObject(ISocket* socket);
            virtual bool                                                                                                                IsDisposed();
            virtual int                                                                                                                 GetMaxConcurrent();
            virtual const std::string&                                                                                                  GetHostName();
            virtual int                                                                                                                 GetPort();
            virtual std::shared_ptr<Hosting>                                                                                            GetHosting();
            virtual bool                                                                                                                IsListening();
            virtual bool                                                                                                                IsOnlyRoutingTransparent();

        public:
            virtual int                                                                                                                 GetAllPlatformName(std::vector<std::string>& s);
            virtual int                                                                                                                 GetAllLinkType(const std::string& platform, std::vector<Byte>& s);
            virtual int                                                                                                                 GetAllServerSocketTable(const std::string& platform, std::vector<std::shared_ptr<ServerSocketTable>/**/>& s);
            virtual std::shared_ptr<ServerSocketTable>                                                                                  GetServerSocketTable(const std::string& platform, Byte linkType);
            virtual std::shared_ptr<StageServerSocket>                                                                                  GetServerSocket(const std::string& platform, Byte linkType, int serverNo);
            virtual std::shared_ptr<StageServerSocket>                                                                                  AbortServerSocket(const std::string& platform, Byte linkType, int serverNo);
            virtual int                                                                                                                 AbortAllServerSockets();

        protected:
            virtual int                                                                                                                 ProcessTransitroute(Transitroute& e, std::shared_ptr<Message>& message);
            virtual bool                                                                                                                ReshiftMessage(Transitroute& e, std::shared_ptr<Message>& message);
            virtual int                                                                                                                 ProcessAccept(Transitroute& e);
            virtual int                                                                                                                 ProcessAbort(Transitroute& e);
            virtual int                                                                                                                 ProcessMessage(Transitroute& e, std::shared_ptr<Message>& message);
            virtual std::shared_ptr<StageServerSocket>                                                                                  AddServerSocket(
                const std::string&                                                                                                      platform,
                Byte                                                                                                                    linkType,
                int                                                                                                                     serverNo,
                std::shared_ptr<ServerSocketTable>&                                                                                     socketTable,
                const std::function<std::shared_ptr<StageServerSocket>(const std::string& platform, Byte linkType, int serverNo)>&      addValueFactory);
            virtual bool                                                                                                                IsAllowServerOfflinePackingOnly();
            virtual std::shared_ptr<ServerSocketTable>                                                                                  CreateServerSocketTable(const std::string& platform, Byte linkType);
            virtual std::shared_ptr<StageServerSocket>                                                                                  CreateServerSocket(const std::string& platform, Byte linkType, int serverNo, UInt32 address, int port);

        protected:
            virtual int                                                                                                                 GetTickAlwaysIntervalTime();
            virtual bool                                                                                                                ProcessTickAlways();
            virtual int                                                                                                                 GetRecoverAllChannelsTickAlwaysTime();
            virtual bool                                                                                                                RecoverAllChannels();
            virtual bool                                                                                                                IsLinkAbortCloseAllVirtualSockets();
            virtual bool                                                                                                                IsLinkAbortCloseAllServerSockets();

        private:
            int                                                                                                                         AddAndOpenReleaseAllChannels(int mode);
            bool                                                                                                                        RawSend(std::shared_ptr<unsigned char>& packet, size_t counts, const SendAsyncCallback& callback);
            std::shared_ptr<StageServerSocket>                                                                                          FindOrAddServerSocket(
                const std::string&                                                                                                      platform,
                Byte                                                                                                                    linkType,
                int                                                                                                                     serverNo,
                std::shared_ptr<ServerSocketTable>&                                                                                     socketTable,
                bool                                                                                                                    allowAdding,
                AcceptClient*                                                                                                           pac);

        protected:
            virtual SocketObject                                                                                                        NextAvailableChannel();
            virtual SocketObject                                                                                                        CreateSocket();
            virtual std::shared_ptr<IStageSocketHandler>                                                                                GetSocketHandler();
            virtual bool                                                                                                                AddChannel(SocketObject& socket);
            virtual bool                                                                                                                ReleaseChannel(SocketObject& socket);
            virtual bool                                                                                                                MarkAvailableChannel(SocketObject& socket);

        protected:
            virtual void                                                                                                                ProcessAbort(std::shared_ptr<StageSocket>& socket);
            virtual bool                                                                                                                ProcessOpen(std::shared_ptr<StageSocket>& socket);

        protected:
            virtual bool                                                                                                                ProcessExchange(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message);
            virtual int                                                                                                                 ProcessAccept(TBulkManyMessage<AcceptClient>& e);
            virtual int                                                                                                                 ProcessAbort(BulkManyMessage& e);
            virtual int                                                                                                                 ProcessMessage(BulkManyMessage& e, std::shared_ptr<Message>& message);
            virtual bool                                                                                                                ReshiftMessage(BulkManyMessage& e, std::shared_ptr<Message>& message);

        protected:
            virtual void                                                                                                                OnOpen(EventArgs& e);
            virtual void                                                                                                                OnAbort(EventArgs& e);
            virtual void                                                                                                                OnMessage(std::shared_ptr<StageSocket>& socket, std::shared_ptr<Message>& message);

        protected:
            virtual void                                                                                                                OnOpen(std::shared_ptr<StageVirtualSocket>& socket, EventArgs& e);
            virtual void                                                                                                                OnAbort(std::shared_ptr<StageVirtualSocket>& socket, EventArgs& e);
            virtual void                                                                                                                OnMessage(std::shared_ptr<StageVirtualSocket>& socket, std::shared_ptr<Message>& message);

        protected:
            virtual void                                                                                                                OnOpen(std::shared_ptr<StageServerSocket>& socket, EventArgs& e);
            virtual void                                                                                                                OnAbort(std::shared_ptr<StageServerSocket>& socket, EventArgs& e);
            virtual void                                                                                                                OnMessage(std::shared_ptr<StageServerSocket>& socket, std::shared_ptr<Message>& message);

        public:
            AbortEventHandler                                                                                                           AbortEvent;
            OpenEventHandler                                                                                                            OpenEvent;
            MessageEventHandler                                                                                                         MessageEvent;

        public:
            ServerSocketAbortEventHandler                                                                                               ServerSocketAbortEvent;
            ServerSocketOpenEventHandler                                                                                                ServerSocketOpenEvent;
            ServerSocketMessageEventHandler                                                                                             ServerSocketMessageEvent;

        public:
            VirtualSocketAbortEventHandler                                                                                              VirtualSocketAbortEvent;
            VirtualSocketOpenEventHandler                                                                                               VirtualSocketOpenEvent;
            VirtualSocketMessageEventHandler                                                                                            VirtualSocketMessageEvent;

        public:
            static const int                                                                                                            MAX_CONCURRENT = 32;
            static const int                                                                                                            MIN_CONCURRENT = 1;

        private:
            SyncObjectBlock                                                                                                             _syncobj;
            std::shared_ptr<Hosting>                                                                                                    _hosting;
            Byte                                                                                                                        _chLinkType;
            int                                                                                                                         _maxConcurrent;
            int                                                                                                                         _port;
            std::string                                                                                                                 _hostName;
            bool                                                                                                                        _disposed : 1;
            bool                                                                                                                        _listening : 1;
            bool                                                                                                                        _absOpened : 1;
            bool                                                                                                                        _allowAccept : 1;
            bool                                                                                                                        _firstTimeAcceptClient : 4;
            SocketLinkedListNode*                                                                                                       _currentAvailableChannels;  // ��ǰ����ͨ��
            SocketLinkedList                                                                                                            _availableChannelsList;     // �ͨ������
            SocketObjectTable                                                                                                           _workingChannelsTable;      // ���ͨ����
            std::list<std::shared_ptr<SocketLinkedListNode>/**/>                                                                        _freeChannelNode;           // ����ͨ���ڵ�
            std::shared_ptr<IStageSocketHandler>                                                                                        _socketHandler;
            std::string                                                                                                                 _platform;
            int                                                                                                                         _svrAreaNo;
            int                                                                                                                         _serverNo;
            int                                                                                                                         _maxFiledescriptor;
            boost::asio::ip::tcp::endpoint                                                                                              _localEP;
            boost::asio::ip::tcp::endpoint                                                                                              _remoteEP;
            VirtualSocketTable                                                                                                          _virtualSocketTable;
            VirtualSocketTable2                                                                                                         _virtualSocketTable2;
            std::shared_ptr<Timer>                                                                                                      _tickAlways;
            UInt64                                                                                                                      _lastRecoverAllChannelTime;
            ServerSocketPlatformTable                                                                                                   _serverSocketTable;
            SendbackController                                                                                                          _sendbackController;
        };
    }
}