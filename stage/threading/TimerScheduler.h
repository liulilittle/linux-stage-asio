#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/LinkedList.h"

namespace ep
{
    namespace threading
    {
        class Timer;

        class TimerScheduler : public std::enable_shared_from_this<TimerScheduler>
        {
            typedef std::shared_ptr<Timer>                                  TimerObject;
            typedef LinkedList<TimerObject>                                 TimerLikedList;
            typedef LinkedListNode<TimerObject>                             TimerLikedListNode;
            typedef HASHMAPDEFINE(Timer*, TimerObject)                      TimerObjectTable;

            friend class Timer;
            friend class Object;

        public:
            TimerScheduler(const std::shared_ptr<Hosting>& hosting);
            virtual ~TimerScheduler();

        public:
            static const std::shared_ptr<TimerScheduler>                    DefaultScheduler;

        public:
            static void                                                     PreparedGlobalScheduler(const std::shared_ptr<Hosting>& hosting);

        public:
            virtual int                                                     DoEvents();
            virtual int                                                     GetCount();
            virtual SyncObjectBlock&                                        GetSynchronizingObject();
            virtual void                                                    Dispose();
            virtual bool                                                    IsDisposed();
            virtual bool                                                    IsWatch(Timer* timer);
            virtual Reference                                               GetReference();
            virtual std::shared_ptr<TimerScheduler>                         GetPtr();
            virtual std::shared_ptr<Hosting>                                GetHosting();
            virtual bool                                                    Run();

        protected:
            virtual int                                                     GetTickAlwaysIntervalTime();
            virtual int                                                     GetMaxNextTimerCount();
            virtual std::shared_ptr<Timer>                                  NextTimerObject();
            virtual int                                                     ProcessTickAlways(UInt64 ticks, int counts);

        protected:
            virtual bool                                                    Start(std::shared_ptr<Timer>& timer);
            virtual bool                                                    Stop(std::shared_ptr<Timer>& timer);

        private:
            SyncObjectBlock                                                 _syncobj;
            std::shared_ptr<Hosting>                                        _hosting;
            bool                                                            _disposed;
            UInt64                                                          _lastTime;
            int                                                             _minPeriod;
            TimerLikedListNode*                                             _currentNode;
            TimerLikedList                                                  _linkedList;
            TimerObjectTable                                                _objectTable;
        };
    }
}