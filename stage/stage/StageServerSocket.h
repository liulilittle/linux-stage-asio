#pragma once

#include "util/Util.h"
#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/LinkedList.h"
#include "util/stage/net/Message.h"
#include "StageSocket.h"
#include "StageVirtualSocket.h"
#include "IStageSocketHandler.h"
#include "StageSocketListener.h"

namespace ep
{
    namespace stage
    {
        class StageClient;
        class StageServerSocket;

        class StageServerSocket : public StageVirtualSocket
        {
            friend class StageClient;

        public:
            StageServerSocket(
                const std::shared_ptr<StageClient>&             transmission,
                const std::string&                              platform,
                int                                             serverNo,
                Byte                                            linkType,
                UInt32                                          address,
                int                                             port);
            virtual ~StageServerSocket();

        public:
            virtual Byte                                        GetLinkType();
            virtual const std::string&                          GetPlatform();
            virtual int                                         GetServerNo();
            virtual std::shared_ptr<StageServerSocket>          GetPointer();
            virtual bool                                        Send(const Message& message, const SendAsyncCallback& callback);
            virtual void                                        Dispose();

        protected:
            virtual bool                                        DestroyObject(std::shared_ptr<StageClient>& transmission);
            virtual bool                                        PostLastAbortIL();

        protected:
            virtual void                                        OnOpen(EventArgs& e);
            virtual void                                        OnAbort(EventArgs& e);
            virtual void                                        OnClose(EventArgs& e);
            virtual void                                        OnMessage(std::shared_ptr<Message>& e);

        private:
            std::string                                         _platform;
            int                                                 _serverNo;
            Byte                                                _linkType;
        };
    }
}