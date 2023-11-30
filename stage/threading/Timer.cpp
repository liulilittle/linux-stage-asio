#include "Timer.h"
#include "TimerScheduler.h"

namespace ep
{
    namespace threading
    {
        Timer::Timer(const std::shared_ptr<TimerScheduler>& scheduler)
            : _scheduler(scheduler)
            , _enabled(false)
            , _disposed(false)
            , _last(0)
            , _interval(0) {
            if (!scheduler) {
                throw std::runtime_error("An NullReferences form of the TimerScheduler is not allowed");
            }
            TickEvent = NULL;
            _nodeblock.LinkedList_ = NULL;
            _nodeblock.Next = NULL;
            _nodeblock.Previous = NULL;
            _nodeblock.Value = make_shared_null<Timer>();
        }

        Timer::~Timer() {
            Dispose();
        }

        void Timer::OnTick(TickEventArgs& e) {
            if (this) {
                TickEventHandler events = TickEvent;
                if (events) {
                    std::shared_ptr<Timer> self = GetPtr();
                    events(self, e);
                }
            }
        }

        bool Timer::DoTick(UInt64 elapsedMilliseconds)
        {
            if (!IsEnabled()) {
                return false;
            }
            else {
                TickEventArgs e(elapsedMilliseconds);
                OnTick(e);
            }
            return true;
        }

        bool Timer::Start() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            if (_enabled) {
                return true;
            }
            else if (_scheduler) {
                std::shared_ptr<Timer> self = GetPtr();
                if (!self) {
                    return false;
                }
                if (_scheduler->Start(self)) {
                    _enabled = true;
                    _last = Hosting::GetTickCount();
                }
            }
            return _enabled;
        }

        bool Timer::Stop() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (_disposed) {
                return false;
            }
            if (_enabled) {
                if (!_scheduler) {
                    _last = 0;
                    _enabled = false;
                }
                else {
                    std::shared_ptr<Timer> self = GetPtr();
                    if (!self) {
                        return false;
                    }
                    if (_scheduler->Stop(self)) {
                        _enabled = false;
                    }
                }
            }
            if (_enabled) {
                return false;
            }
            else {
                _last = 0;
            }
            return true;
        }

        SyncObjectBlock& Timer::GetSynchronizingObject() {
            return _syncobj;
        }

        std::shared_ptr<Timer> Timer::GetPtr() {
            const Timer* self = this;
            if (self) {
                return shared_from_this();
            }
            return std::shared_ptr<Timer>();
        }

        void Timer::Dispose() {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (!_disposed) {
                if (_enabled) {
                    SetEnabled(false);
                }
                _nodeblock.LinkedList_ = NULL;
                _nodeblock.Next = NULL;
                _nodeblock.Previous = NULL;
                _nodeblock.Value = make_shared_null<Timer>();
                if (TickEvent) {
                    TickEvent = NULL;
                }
                if (_scheduler) {
                    _scheduler.reset();
                }
            }
            _disposed = true;
        }

        bool Timer::SetInterval(int milliseconds) {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (milliseconds <= 0) {
                milliseconds = 0;
            }
            if (milliseconds <= 0) {
                if (_enabled) {
                    bool success = Stop();
                    if (!success) {
                        return false;
                    }
                }
            }
            _interval = milliseconds;
            return _interval;
        }

        Timer::TickEventArgs::TickEventArgs(UInt64 elapsedMilliseconds)
            : ElapsedMilliseconds(elapsedMilliseconds) {

        }

        Timer::TickEventArgs::TickEventArgs()
            : ElapsedMilliseconds(0) {

        }

        boost::asio::deadline_timer::duration_type Timer::DurationTime(long long int value, DurationType durationType) {
            switch (durationType)
            {
            case kHours:
                return boost::posix_time::hours(value);
            case kMinutes:
                return boost::posix_time::minutes(value);
            case kSeconds:
                return boost::posix_time::seconds(value);
            case kMilliseconds:
                return boost::posix_time::milliseconds(value);
            default:
                return boost::posix_time::milliseconds(value);
            };
        }
    }
}