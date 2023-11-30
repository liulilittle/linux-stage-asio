#pragma once 

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/Hosting.h"

#include <list>
#include <atomic>

namespace ep // HPP
{
    namespace threading
    {
        class Stranding : public std::enable_shared_from_this<Stranding> // STAGE搁浅（串行化事件驱动STA）
        {
        public:
            typedef std::function<bool(std::shared_ptr<Stranding>&)>            Handler;
            typedef std::function<bool()>                                       Handler2;

        public:
            Stranding()
                : _disposed(false)
                , _workingslen(0) {

            }
            virtual ~Stranding() {
                Dispose();
            }

        public:
            virtual void                                                        Dispose() {
                if (this) {
                    std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                    if (!_disposed) {
                        Clear();
                    }
                    _disposed = true;
                }
            }
            virtual bool                                                        IsDisposed() {
                if (!this) {
                    return false;
                }
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                return _disposed;
            }
            virtual void                                                        Clear() {
                if (this) {
                    std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                    _workingslen = 0;
                    _workings.clear();
                }
            }
            virtual int                                                         GetWorkingCount() {
                if (!this) {
                    return 0;
                }

                return _workingslen.load();
            }
            virtual std::shared_ptr<Stranding>                                  GetPtr() {
                if (!this) {
                    return Object::Nulll<Stranding>();
                }
                return shared_from_this();
            }
            virtual int                                                         DoEvents(int maxConcurrent) { // 驱动
                if (!this) {
                    return 0;
                }
                
                if (maxConcurrent <= 0) { // 至少驱动一次
                    maxConcurrent = 1;
                }

                maxConcurrent = std::min<int>(maxConcurrent, GetWorkingCount());

                int events = 0;
                if (maxConcurrent > 0) {
                    std::shared_ptr<Stranding> self = GetPtr();
                    for (int i = 0; i < maxConcurrent; i++) {
                        Handler working = PopWorkingObject();
                        if (!working) {
                            break;
                        }
                        if (working(self)) {
                            events++;
                        }
                    }
                }
                return events;
            }
            virtual bool                                                        Post(const Handler& working) { // 委托
                if (!this) {
                    return false;
                }
                
                if (!working) {
                    return false;
                }

                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    return false;
                }

                _workings.push_back(working);
                _workingslen++;
                return true;
            }
            inline bool                                                         Post(const Handler2& working) {
                if (!this) {
                    return false;
                }
                if (!working) {
                    return false;
                }
                return Post([working](std::shared_ptr<Stranding>&) {
                    return working();
                });
            }
            virtual SyncObjectBlock&                                            GetSynchronizingObject() {
                return _syncobj;
            }

        protected:
            virtual Handler                                                     PopWorkingObject() {
                if (!this) {
                    return NULL;
                }
                std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                if (_disposed) {
                    return NULL;
                }
                auto il = _workings.begin();
                auto el = _workings.end();
                if (il == el) {
                    return NULL;
                }
                Handler working = *il;
                if (il != el) {
                    _workings.erase(il);
                    --_workingslen;
                    if (_workingslen < 0) {
                        _workingslen = 0;
                    }
                    il = _workings.end();
                }
                return working;
            }

        private:
            bool                                                                _disposed;
            std::atomic<int>                                                    _workingslen;
            SyncObjectBlock                                                     _syncobj;
            std::list<Handler>                                                  _workings;
        };
    }
}