#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/net/Socket.h"
#include "util/stage/net/Message.h"
#include "util/stage/threading/Timer.h"

#include <list>
#include <vector>

namespace ep
{
    namespace stage
    {
        using ep::threading::Timer;

        class ServerSendbackSettings;
        class ServerSendbackController;

        class ServerSendbackSettings // 单个内部服务器实例托管的最大堆积的发回选项
        {
        private:
            mutable int                                                                                             _maxSendQueueSize;
            mutable Int64                                                                                           _maxSendBufferSize;
            mutable int                                                                                             _thoroughlyAbandonTime;

        public:
            ServerSendbackSettings() {
                Clear();
            }
            virtual ~ServerSendbackSettings() {
                Clear();
            }

        public:
            virtual void                                                                                            Clear() {
                _maxSendQueueSize = 0;
                _maxSendBufferSize = 0;
                _thoroughlyAbandonTime = ~0;
            }

        public:
            virtual ServerSendbackSettings&                                                                         SetMaxSendQueueSize(int value) {
                if (value < 0) {
                    value = 0;
                }
                _maxSendQueueSize = value;
                return *this;
            }
            virtual ServerSendbackSettings&                                                                         SetMaxSendBufferSize(Int64 value) {
                if (value < 0) {
                    value = 0;
                }
                _maxSendBufferSize = value;
                return *this;
            }
            virtual ServerSendbackSettings&                                                                         SetThoroughlyAbandonTime(int value) {
                if (value < 0) {
                    value = ~0;
                }
                _thoroughlyAbandonTime = value;
                return *this;
            }

        public:
            virtual const int&                                                                                      GetMaxSendQueueSize() { return _maxSendQueueSize; }
            virtual const Int64&                                                                                    GetMaxSendBufferSize() { return _maxSendBufferSize; }
            virtual const int&                                                                                      GetThoroughlyAbandonTime() { return _thoroughlyAbandonTime; }
        };

        class ServerSendbackController : public std::enable_shared_from_this<ServerSendbackController>              // 控制器
        {
        private:
            class ServerSendbackContext
            {
            public:
                std::shared_ptr<unsigned char>                                                          packet;
                int                                                                                     packetSize;
            };
            class ServerObjectController
            {
            public:
                int                                                                                     serverNo;
                Byte                                                                                    linkType;
                std::string                                                                             platform;
                UInt64                                                                                  cachedAbortingTime;
                Int64                                                                                   cachedPacketSize;
                Int32                                                                                   cachedQueueCount;
                std::list<ServerSendbackContext>                                                        cachedContexts;

            public:
                inline ServerObjectController() {
                    Clear(true);
                }
                inline void                                                                             Clear(bool resetAbortingTime = false) {
                    if (this) {
                        serverNo = 0;
                        linkType = 0;
                        platform = "";
                        if (resetAbortingTime) {
                            cachedAbortingTime = 0;
                        }
                        cachedPacketSize = 0;
                        cachedQueueCount = 0;
                        cachedContexts.clear();
                    }
                }
            };

        private:
            typedef std::shared_ptr<ServerObjectController>                                                         ServerObjectControllerObject;
            typedef HASHMAPDEFINE(std::string, ServerObjectControllerObject)                                        ServerObjectControllerTable;

        public:
            class UnderloadEventArgs : public EventArgs
            {
            public:
                std::string                                                                                         Platform;
                int                                                                                                 ServerNo;
                Byte                                                                                                LinkType;

            public:
                inline UnderloadEventArgs()
                    : EventArgs()
                    , ServerNo(0)
                    , LinkType(0)
                {

                }
            };
            typedef std::function<void(std::shared_ptr<ServerSendbackController>& sender, UnderloadEventArgs& e)>   UnderloadEventHandler;

        public:
            ServerSendbackController();
            virtual ~ServerSendbackController();

        public:
            UnderloadEventHandler                                                                                   UnderloadEvent;

        private:
            int                                                                                                     RawAddSendback(
                const std::string&                                                                                  platform,
                int                                                                                                 serverNo,
                Byte                                                                                                linkType,
                const std::shared_ptr<unsigned char>&                                                               packet,
                size_t                                                                                              packetSize);

        public:
            virtual SyncObjectBlock&                                                                                GetSynchronizingObject();
            virtual std::string                                                                                     GetControllerKey(const std::string& platform, int serverNo, Byte linkType);
            virtual int                                                                                             DoEvents(const ServerObjectControllerObject& controller);
            virtual ServerSendbackSettings*                                                                         GetSettings();
            virtual int                                                                                             Clear(const std::string& platform, int serverNo, Byte linkType);

        public:
            virtual bool                                                                                            AddSendback(
                const std::string&                                                                                  platform,
                int                                                                                                 serverNo,
                Byte                                                                                                linkType,
                const std::shared_ptr<unsigned char>&                                                               packet,
                size_t                                                                                              packetSize);
            virtual void                                                                                            OnUnderload(UnderloadEventArgs& e);

        public:
            enum Events
            {
                kLeftShiftCritical,
                kOnline,
                kOffline,
                kRightShiftCritical,
            };
            virtual int                                                                                             On(
                Events                                                                                              events,
                const std::string&                                                                                  platform,
                int                                                                                                 serverNo,
                Byte                                                                                                linkType,
                const std::function<bool(std::shared_ptr<unsigned char>& packet, size_t packetSize)>&               sendback);

        public:
            virtual bool                                                                                            Run();
            virtual std::shared_ptr<ServerSendbackController>                                                       GetPointer();
            virtual bool                                                                                            IsDisposed();
            virtual void                                                                                            Dispose();

        protected:
            virtual int                                                                                             ProcessTickAlways();
            virtual bool                                                                                            ResetSendback(const ServerObjectControllerObject& controller);

        private:
            SyncObjectBlock                                                                                         _syncobj;
            ServerSendbackSettings                                                                                  _settings;
            bool                                                                                                    _disposed;
            ServerObjectControllerTable                                                                             _controllerTable;
            std::shared_ptr<Timer>                                                                                  _tickAlwaysTimer;
        };
    }
}