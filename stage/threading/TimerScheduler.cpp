#include "Timer.h"
#include "TimerScheduler.h"

namespace ep
{
    namespace threading
    {
        const std::shared_ptr<TimerScheduler> TimerScheduler::DefaultScheduler;

        TimerScheduler::TimerScheduler(const std::shared_ptr<Hosting>& hosting)
            : _hosting(hosting)
            , _disposed(false)
            , _lastTime(0)
            , _minPeriod(1)
            , _currentNode(NULL)
        {
            if (!hosting) {
                throw std::runtime_error("An NullReferences form of the Hosting is not allowed");
            }
            else {
                _linkedList.Clear(false);
            }
        }

        TimerScheduler::~TimerScheduler()
        {
            Dispose();
        }

        void TimerScheduler::PreparedGlobalScheduler(const std::shared_ptr<Hosting>& hosting)
        {
            if (!hosting) {
                throw std::runtime_error("An NullReferences form of the Hosting is not allowed");
            }
            std::shared_ptr<TimerScheduler>& scheduler = Object::constcast(DefaultScheduler);
            scheduler = make_shared_object<TimerScheduler>(hosting);
            if (!scheduler) {
                throw std::runtime_error("Unable to create an instance of a valid timer scheduler");
            }
            scheduler->Run();
        }

        int TimerScheduler::DoEvents()
        {
            int events = 0;
            UInt64 ullCurrentTime = Hosting::GetTickCount();
            if (0 == _lastTime) {
                _lastTime = ullCurrentTime;
            }
            else {
                int iMaxNextTimerCount = GetMaxNextTimerCount();
                if (iMaxNextTimerCount > 0) {
                    int iTickAlwaysIntervalTime = GetTickAlwaysIntervalTime();
                    if (iTickAlwaysIntervalTime <= 0) {
                        iTickAlwaysIntervalTime = 1;
                    }
                    if (_lastTime > ullCurrentTime || (ullCurrentTime - _lastTime) >= (UInt64)iTickAlwaysIntervalTime) {
                        _lastTime = ullCurrentTime;
                        events += ProcessTickAlways(ullCurrentTime, iMaxNextTimerCount);
                    }
                }
            }
            return events;
        }

        int TimerScheduler::GetCount()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed) {
                return 0;
            }
            return std::min<int>(_linkedList.Count(), _objectTable.size());
        }

        SyncObjectBlock& TimerScheduler::GetSynchronizingObject()
        {
            return _syncobj;
        }

        void TimerScheduler::Dispose()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            _currentNode = NULL;
            if (!_disposed) {
                TimerLikedListNode* node = _linkedList.First();
                while (node) {
                    TimerObject to = node->Value;
                    node = node->Next;
                    if (to) {
                        Stop(to);
                    }
                }
            }
            _linkedList.Clear(false);
            _objectTable.clear();
            if (_hosting) {
                _hosting->UnuseMiddlewaredriver(GetPtr());
                _hosting.reset();
            }
            _disposed = true;
        }

        bool TimerScheduler::IsDisposed()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            return _disposed;
        }

        bool TimerScheduler::IsWatch(Timer* timer)
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed) {
                return false;
            }
            auto il = _objectTable.find(timer);
            auto el = _objectTable.end();
            if (il == el) {
                return false;
            }
            return true;
        }

        Reference TimerScheduler::GetReference()
        {
            return GetPtr();
        }

        std::shared_ptr<TimerScheduler> TimerScheduler::GetPtr()
        {
            TimerScheduler* self = this;
            if (self) {
                return shared_from_this();
            }
            return Object::Nulll<TimerScheduler>();
        }

        std::shared_ptr<Hosting> TimerScheduler::GetHosting()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            return _hosting;
        }

        bool TimerScheduler::Run()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed) {
                return false;
            }
            std::shared_ptr<TimerScheduler> scheduler = GetPtr();
            if (!scheduler) {
                return false;
            }
            std::shared_ptr<Hosting> hosting = GetHosting();
            if (!hosting) {
                return false;
            }
            if (hosting->GetMiddlewaredriver(scheduler)) {
                return true;
            }
            return hosting->UseMiddlewaredriver(scheduler, [this](Reference& key, std::shared_ptr<Hosting>& hosting) {
                DoEvents();
            });
        }

        int TimerScheduler::GetTickAlwaysIntervalTime()
        {
            int period = _minPeriod;
            return period <= 0 ? 1000 : period;
        }

        int TimerScheduler::GetMaxNextTimerCount()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            return std::min<int>(1000, _linkedList.Count());
        }

        std::shared_ptr<Timer> TimerScheduler::NextTimerObject()
        {
            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed || _linkedList.Count() <= 0 || _objectTable.empty()) {
                _currentNode = NULL;
            }
            else {
                if (_currentNode) {
                    _currentNode = _currentNode->Next;
                }
                if (!_currentNode) {
                    _currentNode = _linkedList.First();
                }
                if (_currentNode) {
                    return _currentNode->Value;
                }
            }
            return make_shared_null<Timer>();
        }

        int TimerScheduler::ProcessTickAlways(UInt64 ticks, int counts)
        {
            int events = 0;
            for (int i = 0; i < counts; i++) {
                if (_disposed) {
                    break;
                }
                std::shared_ptr<Timer> timer = NextTimerObject();
                if (!timer) {
                    continue;
                }
                UInt64& last = timer->_last;
                if (0 == last) {
                    last = ticks;
                    continue;
                }
                bool stoped = false;
                int interval = timer->_interval;
                if (interval <= 0 || timer->_disposed || !timer->_enabled) {
                    stoped = true;
                }
                else {
                    UInt64 deltaTime = 0;
                    if (last > ticks || (deltaTime = (ticks - last)) >= (UInt64)interval) {
                        last = ticks;
                        if (timer->DoTick(deltaTime)) {
                            events++;
                        }
                        else {
                            stoped = true;
                        }
                    }
                }
                if (stoped) {
                    Stop(timer);
                }
            }
            return events;
        }

        bool TimerScheduler::Start(std::shared_ptr<Timer>& timer)
        {
            if (!timer) {
                return false;
            }

            Timer* po = timer.get();
            if (!po) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scope_timer(po->_syncobj);
            if (po->_disposed) {
                return false;
            }

            LinkedListNode<TimerObject>* nodeblock = Object::addressof(po->_nodeblock);
            assert(NULL != nodeblock);

            auto il = _objectTable.find(po);
            auto el = _objectTable.end();
            if (il != el) {
                return true;
            }
            else {
                if (po->_interval <= 0) {
                    return false;
                }

                po->_enabled = true;
                po->_last = Hosting::GetTickCount();

                if (nodeblock) {
                    nodeblock->LinkedList_ = NULL;
                    nodeblock->Next = NULL;
                    nodeblock->Previous = NULL;
                    nodeblock->Value = timer;
                }
            }

            if (0 == _minPeriod || _linkedList.Count() <= 0 || _objectTable.empty()) {
                _currentNode = NULL;
                _minPeriod = po->_interval;
            }
            else if (_minPeriod > po->_interval) {
                _minPeriod = po->_interval;
            }

            _objectTable[po] = timer;
            _linkedList.AddLast(nodeblock);
            return true;
        }

        bool TimerScheduler::Stop(std::shared_ptr<Timer>& timer)
        {
            if (!timer) {
                return false;
            }

            Timer* po = timer.get();
            if (!po) {
                return false;
            }

            std::unique_lock<SyncObjectBlock> scope(_syncobj);
            if (_disposed) {
                return false;
            }

            LinkedListNode<TimerObject>* nodeblock = Object::addressof(po->_nodeblock);
            assert(NULL != nodeblock);

            std::unique_lock<SyncObjectBlock> scope_timer(po->_syncobj);
            if (nodeblock == _currentNode) { // 若删除当前节点的成员
                _currentNode = _currentNode->Next; // 则向后移动地址标识符
                if (!_currentNode) { // 若地址标识符指向空地址则指向头部地址
                    _currentNode = _linkedList.First();
                }
            }
            _linkedList.Remove(nodeblock);

            auto il = _objectTable.find(timer.get());
            auto el = _objectTable.end();
            if (il != el) {
                _objectTable.erase(il);
                il = el;
            }

            po->_last = 0;
            po->_enabled = false;

            if (nodeblock) {
                nodeblock->LinkedList_ = NULL;
                nodeblock->Next = NULL;
                nodeblock->Previous = NULL;
                nodeblock->Value = make_shared_null<Timer>();
            }

            if (_linkedList.Count() <= 0 || _objectTable.empty()) {
                _minPeriod = 0;
                _currentNode = NULL;
            }
            return true;
        }
    }
}