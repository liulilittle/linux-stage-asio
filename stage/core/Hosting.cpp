#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "Hosting.h"

namespace ep
{
    Hosting::Hosting(int concurrent)
        : _disposed(false)
        , _concurrent(concurrent)
        , _runtiming(false)
        , _context()
        , _work(_context)
        , _strand(_context) {
        if (concurrent <= 0) {
            concurrent = GetProcesserCount();
        }

        if (concurrent <= 0) {
            concurrent = 1;
        }

        _concurrent = concurrent;
    }

    Hosting::~Hosting() {
        Dispose();
    }

    int Hosting::GetProcesserCount() {
        return get_nprocs();
    }

    unsigned long long Hosting::GetTickCount(bool microseconds) {
#ifdef WIN32
        static LARGE_INTEGER ticksPerSecond; // (unsigned long long)GetTickCount64();
        LARGE_INTEGER ticks;
        if (!ticksPerSecond.QuadPart) {
            QueryPerformanceFrequency(&ticksPerSecond);
        }

        QueryPerformanceCounter(&ticks);
        if (microseconds) {
            double cpufreq = (double)(ticksPerSecond.QuadPart / 1000000);
            unsigned long long nowtick = (unsigned long long)(ticks.QuadPart / cpufreq);
            return nowtick;
        }
        else {
            unsigned long long seconds = ticks.QuadPart / ticksPerSecond.QuadPart;
            unsigned long long milliseconds = 1000 * (ticks.QuadPart - (ticksPerSecond.QuadPart * seconds)) / ticksPerSecond.QuadPart;
            unsigned long long nowtick = seconds * 1000 + milliseconds;
            return (unsigned long long)nowtick;
        }
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);

        if (microseconds) {
            unsigned long long nowtick = (unsigned long long)tv.tv_sec * 1000000;
            nowtick += tv.tv_usec;
            return nowtick;
        }

        return ((unsigned long long)tv.tv_usec / 1000) + ((unsigned long long)tv.tv_sec * 1000);
#endif
    }

    void Hosting::Sleep(int milliseconds)
    {
        if (milliseconds > 0) {
#ifdef WIN32
            timeBeginPeriod(1);
            ::Sleep(1);
            timeEndPeriod(1);
#else
            int seconds = milliseconds / 1000;
            milliseconds = milliseconds % 1000;
            if (seconds > 0) {
                sleep(seconds);
            }

            int microseconds = milliseconds * 1000;
            if (microseconds > 0) {
                usleep(microseconds);
            }
#endif
        }
    }

    bool Hosting::IsDisposed() {
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        return _disposed;
    }

    bool Hosting::IsWorking() {
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        return _runtiming;
    }

    int Hosting::GetMaxConcurrent() {
        return _concurrent;
    }

    bool Hosting::Run() {
        std::shared_ptr<Hosting> self = GetPtr();
        if (!self) {
            return false;
        }

        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (_runtiming || _disposed) {
            return false;
        }

        // 设置为运行时状态
        _runtiming = true;

        // 运行服务器工作线程
        for (int i = 0; i < _concurrent; i++) {
            _working.push_back(
                std::thread([self] {
                if (self || !self->IsDisposed()) {
                    boost::system::error_code ec;
                    while (self->_context.run_one(ec)) {
                        self->RunAllMiddlewaredriver();
                        if (!self->_runtiming || self->_disposed) {
                            break;
                        }
                    }
                }
            }));
        }

        _working.push_back(std::thread([self] {
            if (self) {
                while (!self->IsDisposed()) {
                    if (!self->IsAllowMiddlewareToRunVariableSpeeds()) {
                        self->RunAllMiddlewaredriver();
                        Sleep(1);
                    }
                    else {
                        UInt64 enterMicroseconds = Hosting::GetTickCount(true);
                        do {
                            self->RunAllMiddlewaredriver();
                        } while (0, 0);
                        UInt64 exitMicrosenconds = Hosting::GetTickCount(true);
                        UInt64 elapsedMicrosenconds = exitMicrosenconds - enterMicroseconds;
                        if (elapsedMicrosenconds < 1000) {
#ifdef WIN32
                            Sleep(1);
#else
                            UInt64 sleepMicrosenconds = 1000 - elapsedMicrosenconds;
                            usleep(sleepMicrosenconds);
#endif
                        }
                    }
                }
            }
        }));
        return true;
    }

    void Hosting::Dispose() {
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (_disposed) {
            return;
        }

        // 设置为未运行状态
        _disposed = true;
        _runtiming = false;

        // 停止并且重置I/O工作上下文队列
        _context.stop();
        _context.reset();

        // 等待I/O上下文堆列工作线程释放
        for (int count = _working.size(), index = 0; index < count; ++index) {
            Object::constcast(_working[index]).detach();
        }

        // 清楚包含I/O上下文堆列工作线程的管理容器
        _working.clear();
    }

    void Hosting::Close() {
        Dispose();
    }

    io_context& Hosting::GetContextObject() {
        return _context;
    }

    io_context::work& Hosting::GetContextWorkObject() {
        return _work;
    }

    boost::asio::strand& Hosting::GetStrandObject() {
        return _strand;
    }

    SyncObjectBlock& Hosting::GetSynchronizingObject() {
        return _syncobj;
    }

    std::shared_ptr<Hosting> Hosting::GetPtr() {
        Hosting* self = this;
        if (self) {
            return shared_from_this();
        }
        return Object::Nulll<Hosting>();
    }

    Hosting::Middlewaredriver Hosting::UnuseMiddlewaredriver(const Reference& key)
    {
        if (!key) {
            return NULL;
        }
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (_disposed) {
            return NULL;
        }
        if (_middlewaredrivers.empty()) {
            return NULL;
        }
        auto il = _middlewaredrivers.find(key);
        auto el = _middlewaredrivers.end();
        if (il == el) {
            return NULL;
        }
        Middlewaredriver middlewaredriver = il->second;
        _middlewaredrivers.erase(il);
        return middlewaredriver;
    }

    bool Hosting::UseMiddlewaredriver(const Reference& key, const Hosting::Middlewaredriver& middlewaredriver)
    {
        if (!key) {
            return false;
        }
        if (!middlewaredriver) {
            return false;
        }
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (_disposed) {
            return NULL;
        }
        _middlewaredrivers[key] = middlewaredriver;
        return true;
    }

    Hosting::Middlewaredriver Hosting::GetMiddlewaredriver(const Reference& key) {
        if (!key) {
            return NULL;
        }
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (_disposed) {
            return NULL;
        }
        if (_middlewaredrivers.empty()) {
            return NULL;
        }
        auto il = _middlewaredrivers.find(key);
        auto el = _middlewaredrivers.end();
        if (il == el) {
            return NULL;
        }
        return il->second;
    }

    void Hosting::RunAllMiddlewaredriver()
    {
        std::unique_lock<SyncObjectBlock> scoped(_syncobj);
        if (!_disposed) {
            std::shared_ptr<Hosting> hosting = GetPtr();
            if (hosting) {
                auto il = _middlewaredrivers.begin();
                auto el = _middlewaredrivers.end();
                for (; il != el; ) {
                    if (_middlewaredrivers.empty()) {
                        break;
                    }

                    Middlewaredriver& middlewaredriver = const_cast<Middlewaredriver&>(il++->second);
                    if (!middlewaredriver) {
                        continue;
                    }

                    Reference& key = const_cast<Reference&>(il->first);
                    middlewaredriver(key, hosting);
                }
            }
        }
    }

    bool Hosting::IsAllowMiddlewareToRunVariableSpeeds()
    {
        return false;
    }
}