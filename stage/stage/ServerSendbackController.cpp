#include "ServerSendbackController.h"

namespace ep
{
    namespace stage
    {
        ServerSendbackController::ServerSendbackController() 
            : _disposed(false) {
            _settings.
                SetThoroughlyAbandonTime(1 * 60 * 1000).
                SetMaxSendQueueSize(10000).
                SetMaxSendBufferSize(10 * 1024 * 1024);
        }

        ServerSendbackController::~ServerSendbackController() {
            Dispose();
        }

        SyncObjectBlock& ServerSendbackController::GetSynchronizingObject() {
            return _syncobj;
        }

        std::string ServerSendbackController::GetControllerKey(const std::string& platform, int serverNo, Byte linkType) {
            char sz[1000];
            sprintf(sz, "%s.%d.%d", platform.data(), serverNo, linkType);
            return sz;
        }

        int ServerSendbackController::RawAddSendback(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<unsigned char>& packet, size_t packetSize) {
            if (!packet || 0 == packetSize) {
                return -1;
            }

            std::string controllerKey = GetControllerKey(platform, serverNo, linkType);
            if (controllerKey.empty()) {
                return -2;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_disposed) {
                return -3;
            }

            // 检查重新提交内核PCB队列限定设置
            ServerSendbackSettings* settings = GetSettings();
            if (NULL == settings) {
                return -4;
            }

            int thoroughlyAbandonTime = settings->GetThoroughlyAbandonTime();
            if (0 == thoroughlyAbandonTime) {
                return -5;
            }

            ServerObjectControllerObject controller = make_shared_null<ServerObjectController>();
            auto il = _controllerTable.find(controllerKey);
            auto el = _controllerTable.end();
            if (il != el) {
                controller = il->second;
                if (!controller) {
                    return -6;
                }
            }
            else {
                controller = make_shared_object<ServerObjectController>();
                if (!controller) {
                    return -7;
                }
                else {
                    controller->platform = platform;
                    controller->serverNo = serverNo;
                    controller->linkType = linkType;
                    controller->cachedAbortingTime = Hosting::GetTickCount();
                }
                _controllerTable[controllerKey] = controller;
            }

            Int64 cachedPacketSize = controller->cachedPacketSize + packetSize;
            Int32 cachedQueueCount = controller->cachedQueueCount + 1;

            if (thoroughlyAbandonTime > 0) {
                Int64 abortingTime = controller->cachedAbortingTime;
                if (0 != abortingTime) {
                    Int64 currentTime = Hosting::GetTickCount();
                    if (abortingTime > currentTime || (currentTime - abortingTime) >= thoroughlyAbandonTime) {
                        return -8;
                    }
                }
            }

            if (cachedQueueCount > settings->GetMaxSendQueueSize()) {
                return 1;
            }

            if (cachedPacketSize > settings->GetMaxSendBufferSize()) {
                return 2;
            }

            ServerSendbackContext context;
            context.packet = packet;
            context.packetSize = packetSize;
            controller->cachedContexts.push_back(context);

            controller->cachedPacketSize = cachedPacketSize;
            controller->cachedQueueCount = cachedQueueCount;
            return 0;
        }

        int ServerSendbackController::DoEvents(const ServerObjectControllerObject& controller) {
            int events = 0;
            if (!controller) {
                return events;
            }

            UInt64 ullCurrentTime = Hosting::GetTickCount();
            ServerSendbackSettings* settings = GetSettings();
            if (NULL == settings) {
                return events;
            }

            int iThoroughlyAbandonTime = settings->GetThoroughlyAbandonTime();
            if (iThoroughlyAbandonTime < 0) {
                return events;
            }

            Int64 llAbortingTime = controller->cachedAbortingTime;
            if (0 != llAbortingTime) {
                if (llAbortingTime < 0 || (UInt64)llAbortingTime >= ullCurrentTime || (ullCurrentTime - (UInt64)llAbortingTime) >= (UInt64)iThoroughlyAbandonTime) {
                    int cachedQueueCount = controller->cachedQueueCount;
                    if (cachedQueueCount) {
                        if (cachedQueueCount > 0) {
                            events += cachedQueueCount;
                        }
                        ResetSendback(controller);
                    }
                }
            }

            return events;
        }

        ServerSendbackSettings* ServerSendbackController::GetSettings() {
            if (!this) {
                return NULL;
            }
            return Object::addressof(_settings);
        }

        int ServerSendbackController::ProcessTickAlways() {
            int events = 0;
            std::vector<ServerObjectControllerObject> controllers;
            do {
                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                for (auto il : _controllerTable) {
                    controllers.push_back(il.second);
                }
            } while (0, 0);
            for (size_t i = 0, l = controllers.size(); i < l; i++) {
                ServerObjectControllerObject& il = controllers[i];
                events += DoEvents(il);
            }
            return events;
        }

        bool ServerSendbackController::ResetSendback(const ServerObjectControllerObject& controller) {
            if (!controller) {
                return false;
            }
            else {
                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                controller->cachedPacketSize = 0;
                controller->cachedQueueCount = 0;
                controller->cachedContexts.clear();
            }
            return true;
        }

        int ServerSendbackController::On(Events events, const std::string& platform, int serverNo, Byte linkType, const std::function<bool(std::shared_ptr<unsigned char>& packet, size_t packetSize)>& sendback) {
            typedef ServerSendbackController eevents;

            if (events <= eevents::kLeftShiftCritical || events >= eevents::kRightShiftCritical) {
                return ~0;
            }
            
            if (events == eevents::kOnline) {
                if (!sendback) {
                    return Clear(platform, serverNo, linkType);
                }

                std::list<ServerSendbackContext> cachedContexts;
                do {
                    std::string controllerKey = GetControllerKey(platform, serverNo, linkType);
                    if (controllerKey.empty()) {
                        return 0;
                    }

                    std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                    auto il = _controllerTable.find(controllerKey);
                    auto el = _controllerTable.end();
                    if (il == el) {
                        return 0;
                    }

                    ServerObjectControllerObject controller = il->second;
                    _controllerTable.erase(il);

                    if (!controller) {
                        return 0;
                    }
                    else {
                        controller->cachedAbortingTime = 0;
                    }

                    cachedContexts = controller->cachedContexts;
                } while (0, 0);

                int events = 0;
                for (ServerSendbackContext& context : cachedContexts) {
                    if (sendback(context.packet, context.packetSize)) {
                        events++;
                    }
                }
                return events;
            }
            else if (events == eevents::kOffline) {
                std::list<ServerSendbackContext> cachedContexts;
                std::string controllerKey = GetControllerKey(platform, serverNo, linkType);
                if (controllerKey.empty()) {
                    return 0;
                }

                UInt64 cachedCurrentTime = Hosting::GetTickCount();
                ServerObjectControllerObject controller = make_shared_null<ServerObjectController>();

                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                auto il = _controllerTable.find(controllerKey);
                auto el = _controllerTable.end();
                if (il == el) {
                    controller = make_shared_object<ServerObjectController>();
                    if (!controller) {
                        return 0;
                    }
                    else {
                        controller->platform = platform;
                        controller->serverNo = serverNo;
                        controller->linkType = linkType;
                        controller->cachedAbortingTime = cachedCurrentTime;
                    }
                    il = _controllerTable.insert(std::make_pair(controllerKey, controller)).first;
                }
                else {
                    controller = il->second;
                    if (!controller) {
                        return 0;
                    }
                }
                controller->cachedAbortingTime = cachedCurrentTime;
                return 1;
            }
            else {
                return ~1;
            }
        }

        int ServerSendbackController::Clear(const std::string& platform, int serverNo, Byte linkType) {
            std::string controllerKey = GetControllerKey(platform, serverNo, linkType);
            if (controllerKey.empty()) {
                return 0;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            auto il = _controllerTable.find(controllerKey);
            auto el = _controllerTable.end();
            if (il == el) {
                return 0;
            }

            ServerObjectControllerObject controller = il->second;
            _controllerTable.erase(il);

            if (!controller) {
                return 0;
            }

            int events = controller->cachedQueueCount;
            controller->Clear(true);
            return events;
        }

        bool ServerSendbackController::Run() {
            std::shared_ptr<ServerSendbackController> self = GetPointer();
            if (!self) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            if (_disposed) {
                return false;
            }

            _tickAlwaysTimer = Timer::Create<Timer>();
            if (!_tickAlwaysTimer) {
                return false;
            }

            _tickAlwaysTimer->SetInterval(500);
            _tickAlwaysTimer->TickEvent = [self](std::shared_ptr<Timer>& sender, Timer::TickEventArgs& e) {
                if (self) {
                    std::unique_lock<SyncObjectBlock> scoped(self->GetSynchronizingObject());
                    if (!self->IsDisposed()) {
                        self->ProcessTickAlways();
                    }
                }
            };
            _tickAlwaysTimer->Start();
            return true;
        }

        std::shared_ptr<ServerSendbackController> ServerSendbackController::GetPointer() {
            if (this) {
                return shared_from_this();
            }
            return make_shared_null<ServerSendbackController>();
        }

        bool ServerSendbackController::IsDisposed() {
            if (!this) {
                return true;
            }
            std::unique_lock<SyncObjectBlock> scoped(_syncobj);
            return _disposed;
        }

        void ServerSendbackController::Dispose() {
            if (!this) {
                return;
            }
            do {
                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                _disposed = true;
                _controllerTable.clear();
                if (_tickAlwaysTimer) {
                    _tickAlwaysTimer->Dispose();
                }
                _tickAlwaysTimer.reset();
                if (UnderloadEvent) {
                    UnderloadEvent = NULL;
                }
            } while (0, 0);
        }

        void ServerSendbackController::OnUnderload(UnderloadEventArgs& e)
        {
            if (!this) {
                return;
            }
            UnderloadEventHandler events = NULL;
            do {
                std::unique_lock<SyncObjectBlock> scoped(_syncobj);
                if (_disposed) {
                    break;
                }
                events = UnderloadEvent;
            } while (0, 0);
            if (events) {
                std::shared_ptr<ServerSendbackController> controller = GetPointer();
                events(controller, e);
            }
        }

        bool ServerSendbackController::AddSendback(const std::string& platform, int serverNo, Byte linkType, const std::shared_ptr<unsigned char>& packet, size_t packetSize)
        {
            if (!this) {
                return false;
            }
            int rc = RawAddSendback(platform, serverNo, linkType, packet, packetSize);
            if (rc > 0) {
                UnderloadEventArgs e;
                e.LinkType = linkType;
                e.Platform = platform;
                e.ServerNo = serverNo;
                OnUnderload(e);
            }
            return 0 == rc;
        }
    }
}