#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/LinkedList.h"
#include "util/stage/threading/TimerScheduler.h"

namespace ep
{
    namespace threading
    {
        class Timer : public std::enable_shared_from_this<Timer>
        {
        private:
            friend class TimerScheduler;

        private:
            typedef std::shared_ptr<Timer>                                                                  TimerObject;
            typedef std::shared_ptr<TimerScheduler>                                                         SchedulerObject;

        public:
            class TickEventArgs : public EventArgs
            {
            public:
                TickEventArgs();
                TickEventArgs(UInt64 elapsedMilliseconds);

            public:
                const UInt64                                                                                ElapsedMilliseconds;
            };
            typedef std::function<void(std::shared_ptr<Timer>& sender, TickEventArgs& e)>                   TickEventHandler;
            enum DurationType
            {
                kHours,                                                                                     //  ±
                kMinutes,                                                                                   // ∑÷
                kSeconds,                                                                                   // √Î
                kMilliseconds,                                                                              // ∫¡√Î
            };
            static boost::asio::deadline_timer::duration_type                                               DurationTime(long long int value, DurationType durationType = kMilliseconds);

        public:
            Timer(const std::shared_ptr<TimerScheduler>& scheduler);
            virtual ~Timer();

        public:
            TickEventHandler                                                                                TickEvent;

        protected:
            virtual void                                                                                    OnTick(TickEventArgs& e);

        public:
            virtual std::shared_ptr<Timer>                                                                  GetPtr();
            virtual void                                                                                    Dispose();
            virtual bool                                                                                    SetInterval(int milliseconds);
            template<typename TTimer>
            inline static std::shared_ptr<TTimer>                                                           Create() {
                return Create<TTimer>(TimerScheduler::DefaultScheduler);
            }
            template<typename TTimer>
            inline static std::shared_ptr<TTimer>                                                           Create(const std::shared_ptr<TimerScheduler>& scheduler) {
                if (!scheduler) {
                    throw std::runtime_error("An NullReferences form of the TimerScheduler is not allowed");
                }
                return make_shared_object<TTimer>(scheduler);
            }

        public:
            virtual bool                                                                                    Start();
            virtual bool                                                                                    Stop();
            virtual SyncObjectBlock&                                                                        GetSynchronizingObject();

        public:
            inline std::shared_ptr<TimerScheduler>                                                          GetScheduler() {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                return _scheduler;
            }
            inline bool                                                                                     IsEnabled() {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                return _enabled;
            }
            inline bool                                                                                     SetEnabled(bool value) {
                return value ? Start() : Stop();
            }
            inline int                                                                                      GetInterval() { return _interval; }

        private:
            bool                                                                                            DoTick(UInt64 elapsedMilliseconds);

        private:
            std::shared_ptr<TimerScheduler>                                                                 _scheduler;
            bool                                                                                            _enabled : 1;
            bool                                                                                            _disposed : 7;
            UInt64                                                                                          _last;
            int                                                                                             _interval;
            SyncObjectBlock                                                                                 _syncobj;
            LinkedListNode<TimerObject>                                                                     _nodeblock;
        };
    }
}