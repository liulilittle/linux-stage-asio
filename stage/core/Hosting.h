#pragma once

#include "Object.h"

namespace ep
{
    class Hosting : public std::enable_shared_from_this<Hosting>
    {
    public:
        /// 中间件驱动器
        typedef std::function<void(Reference& key, std::shared_ptr<Hosting>& hosting)>                                                          Middlewaredriver;

    public:
        Hosting(int concurrent = 0);
        virtual ~Hosting();

    public:
        virtual int                                                                                                                             GetMaxConcurrent();
        virtual bool                                                                                                                            Run();
        virtual void                                                                                                                            Dispose();
        void                                                                                                                                    Close();
        virtual bool                                                                                                                            IsDisposed();
        virtual bool                                                                                                                            IsWorking();

    public:
        virtual io_context&                                                                                                                     GetContextObject();
        virtual io_context::work&                                                                                                               GetContextWorkObject();
        virtual boost::asio::strand&                                                                                                            GetStrandObject();
        virtual SyncObjectBlock&                                                                                                                GetSynchronizingObject();
        virtual std::shared_ptr<Hosting>                                                                                                        GetPtr();

    public:
        virtual Middlewaredriver                                                                                                                UnuseMiddlewaredriver(const Reference& key);
        virtual bool                                                                                                                            UseMiddlewaredriver(const Reference& key, const Middlewaredriver& middlewaredriver);
        virtual Middlewaredriver                                                                                                                GetMiddlewaredriver(const Reference& key);

    protected:
        virtual void                                                                                                                            RunAllMiddlewaredriver();
        virtual bool                                                                                                                            IsAllowMiddlewareToRunVariableSpeeds();

    public:
        static int                                                                                                                              GetProcesserCount();
        static unsigned long long                                                                                                               GetTickCount(bool microseconds = false);
        static void                                                                                                                             Sleep(int milliseconds);

    private:
        bool                                                                                                                                    _disposed;
        int                                                                                                                                     _concurrent;
        bool                                                                                                                                    _runtiming;
        io_context                                                                                                                              _context;
        io_context::work                                                                                                                        _work;
        boost::asio::strand                                                                                                                     _strand;
        std::vector<std::thread>                                                                                                                _working;
        SyncObjectBlock                                                                                                                         _syncobj;
        HASHMAPDEFINE(Reference, Middlewaredriver)                                                                                              _middlewaredrivers;
    };
}