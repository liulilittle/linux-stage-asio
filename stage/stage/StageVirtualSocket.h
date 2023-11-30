#pragma once

#include "util/Util.h"
#include "util/collections/linkedlist.h"
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
        class StageVirtualSocket;

        class StageVirtualSocket : public ISocket // 虚拟链路套接字
        {
            friend class StageClient;

        public:
            //************************************
            // Method:    StageVirtualSocket
            // FullName:  ::global::ep::stage::StageVirtualSocket::StageVirtualSocket
            // Access:    public / internal
            // Returns:   
            // Qualifier:
            // Parameter: const std::shared_ptr<StageClient> & transmission                         传输控制链路层(通常为承载虚拟套接字数据交换分层，介于标准模型的会话分层)
            // Parameter: unsigned short handle                                                     当前套接字实例的对象句柄
            //************************************
            StageVirtualSocket(const std::shared_ptr<StageClient>& transmission, UInt32 handle, UInt32 address, int port);
            virtual ~StageVirtualSocket();

        public:
            Reference                                           Tag;
            Reference                                           UserToken;

        public:
            virtual bool                                        Available();
            virtual void                                        Abort();
            virtual void                                        Close();
            virtual void                                        Dispose();
            virtual boost::asio::ip::tcp::endpoint              GetLocalEndPoint();
            virtual boost::asio::ip::tcp::endpoint              GetRemoteEndPoint();
            virtual Byte                                        GetLinkType();

        private:
            void                                                AbortINT3(bool abortingMode = true);
            void                                                ProcessInput(std::shared_ptr<Message>& message);

        protected:
            virtual void                                        OnOpen(EventArgs& e);
            virtual void                                        OnAbort(EventArgs& e);
            virtual void                                        OnClose(EventArgs& e);
            virtual void                                        OnMessage(std::shared_ptr<Message>& e);

        protected:
            virtual bool                                        GetTransmissionObject(std::shared_ptr<StageClient>& transmission);
            virtual bool                                        DestroyObject(std::shared_ptr<StageClient>& transmission);
            virtual bool                                        RebindEndPoint(UInt32 address, int port);
            virtual bool                                        PreventAbort();
            virtual bool                                        PostLastAbortIL();

        public:
            inline int                                          GetHandle()
            {
                return _handle;
            }
            virtual std::shared_ptr<StageClient>                GetTransmissionObject();
            virtual std::shared_ptr<Hosting>                    GetHosting();
            virtual SyncObjectBlock&                            GetSynchronizingObject();
            virtual bool                                        IsDisposed();
            virtual std::shared_ptr<StageVirtualSocket>         GetPtr();
            virtual bool                                        Send(const Message& message, const SendAsyncCallback& callback);
            virtual bool                                        Send(Commands commandId, const SendAsyncCallback& callback);
            virtual bool                                        Send(const unsigned char* packet, int counts, const SendAsyncCallback& callback);
            virtual bool                                        Send(const std::shared_ptr<unsigned char> packet, int counts, const SendAsyncCallback& callback);

        private:
            UInt32                                              _handle;
            bool                                                _disposed : 1;
            bool                                                _preventAbort : 7;
            UInt32                                              _address;
            int                                                 _port;
            SyncObjectBlock                                     _syncobj;
            std::shared_ptr<StageClient>                        _transmission;
        };
    }
}