#pragma once

#include <boost/noncopyable.hpp> 
#include <boost/shared_ptr.hpp> 
#include <boost/bind.hpp> 
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <iostream> 
#include <atomic>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <stdexcept>
#include <exception>
#include <functional>

#ifndef WIN32
#include "util/Util.h"
#include "util/memory/MemDefine.h"
#endif

#ifndef unchecked
#define unchecked
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#if defined(DEBUG) || defined(_DEBUG)
    #ifndef DEBUG
        #define DEBUG
    #endif
    #ifndef _DEBUG
        #define _DEBUG
    #endif
    #ifndef EPDEBUG
        #define EPDEBUG
    #endif
#endif

#ifndef new_constructor
	#ifdef WIN32
		#define new_constructor(T)					new T
		#define new_constructors(T, SIZE)			new T[SIZE]
	#else
		#define new_constructor(T)					MemNewAlloc2(T)
		#define new_constructors(T, SIZE)			MemNewArray(T, SIZE)
	#endif
#endif

#ifndef delete_constructor
	#ifdef WIN32
		#define delete_constructor(p)				if (NULL != p) { delete p; }
		#define delete_constructors(p)				if (NULL != p) { delete[] p; }
	#else
		#define delete_constructor(p)				MemDeleteAlloc2(p)
		#define delete_constructors(p)				MemDeleteArray(p)
	#endif
#endif

#ifndef offsetof
    #define offsetof(type, member) ((size_t)&reinterpret_cast<char const volatile&>((((type*)0)->member)))
#endif
#ifndef container_of
    #define container_of(ptr, type, member) ((type*)((char*)static_cast<const decltype(((type*)0)->member)*>(ptr) - offsetof(type,member)))
#endif

namespace ep
{
    template<class T>
    struct delete_constructor_t // default_delete
    {
    public:
        inline void operator() (const T* x) const
        {
            if (x != NULL)
            {
#if __cplusplus >= 201103L
                static_assert(0 < sizeof(T),
                    "can't delete an incomplete type");
#endif
                T* v = (T*)x;
                delete_constructor(v);
            }
        }
    };

    template<class T>
    struct delete_constructor_t<T[]> // default_delete
    {
    public:
        inline void operator() (const T* x) const
        {
            if (x != NULL)
            {
#if __cplusplus >= 201103L
                static_assert(0 < sizeof(T),
                    "can't delete an incomplete type");
#endif
                T* v = (T*)x;
                delete_constructors(v);
            }
        }
    };

    template<class T>
    struct no_delete_constructor_t
    {
    public:
        inline void operator() (const T* x) const
        {
#if __cplusplus >= 201103L
            static_assert(0 < sizeof(T),
                "can't delete an incomplete type");
#endif
        }
    };

    template<typename T>
    inline std::shared_ptr<T> make_shared_null()
    {
        return std::shared_ptr<T>();
    }

    template<typename T>
    inline std::shared_ptr<T> warp_shared_object(const T* p)
    {
        T* x = (T*)p;
        if (!x)
        {
            return make_shared_null<T>();
        }
        return std::shared_ptr<T>(x, no_delete_constructor_t<T>());
    }

    template<typename T, typename ...A>
    inline std::shared_ptr<T> make_shared_object(A&&... s)
    {
        T* v = new_constructor(T)(std::forward<A>(s)...);
        return std::shared_ptr<T>(v, delete_constructor_t<T>());
    }

    template<typename T>
    inline std::shared_ptr<T> make_shared_array(size_t size)
    {
        if (0 == size)
        {
            return std::shared_ptr<T>();
        }

        T* v = new_constructors(T, size);
        return std::shared_ptr<T>(v, delete_constructor_t<T[]>());
    }

    typedef boost::asio::io_service                                                                     io_context;
    typedef std::recursive_mutex                                                                        SyncObjectBlock;
    typedef long long int                                                                               Int64;
    typedef unsigned long long int                                                                      UInt64;
    typedef signed int                                                                                  Int32;
    typedef unsigned int                                                                                UInt32;
    typedef signed char                                                                                 SByte;
    typedef unsigned char                                                                               Byte;
    typedef signed short                                                                                Short;
    typedef unsigned short                                                                              UShort;
    typedef float                                                                                       Single;
    typedef double                                                                                      Double;
    typedef long double                                                                                 Decimal;

    class Object
    {
    public:
        template<typename TMonitor>
        inline static void                                                                              synchronize(const TMonitor& o, const std::function<void()>& criticalSection)
        {
            TMonitor& syncobj = const_cast<TMonitor&>(o);
            std::unique_lock<TMonitor> scope(syncobj);
            criticalSection();
        }
        template<typename TResult, typename TMonitor>
        inline static TResult                                                                           synchronize(const TMonitor& o, const std::function<TResult()>& criticalSection)
        {
            TMonitor& syncobj = const_cast<TMonitor&>(o);
            std::unique_lock<TMonitor> scope(syncobj);
            return criticalSection();
        }
        template<typename TResult, typename TMonitor>
        inline static TResult                                                                           synchronize2(const TMonitor& o, const boost::function<TResult()>& criticalSection)
        {
            TMonitor& syncobj = const_cast<TMonitor&>(o);
            std::unique_lock<TMonitor> scope(syncobj);
            return criticalSection();
        }
        template<typename TMonitor>
        inline static void                                                                              synchronize2(const TMonitor& o, const boost::function<void()>& criticalSection)
        {
            TMonitor& syncobj = const_cast<TMonitor&>(o);
            std::unique_lock<TMonitor> scope(syncobj);
            criticalSection();
        }

    public:
        template<typename T>
        inline static T*                                                                                addressof(const T& v)
        {
            return (T*)&reinterpret_cast<const char&>(const_cast<T&>(v));
        }
        template<typename T>
        inline static T*                                                                                addressof(const std::shared_ptr<T>& v)
        {
            if (NULL == &reinterpret_cast<const char&>(v))
            {
                return NULL;
            }
            return (T*)v.get();
        }
        template<typename T>
        inline static T*                                                                                addressof(const boost::shared_ptr<T>& v)
        {
            if (NULL == &reinterpret_cast<const char&>(v))
            {
                return NULL;
            }
            return (T*)v.get();
        }
        template<typename T>
        inline static T*                                                                                constcast(const T* v)
        {
            return const_cast<T*>(v);
        }
        template<typename T>
        inline static T&                                                                                constcast(const T& v)
        {
            return const_cast<T&>(v);
        }
        template<typename T>
        inline static T*                                                                                constcastof(const T* v)
        {
            return const_cast<T*>(v);
        }
        template<typename T>
        inline static T&                                                                                constcastof(const T& v)
        {
            return const_cast<T&>(v);
        }
        template<typename T>
        inline static T&                                                                                as(const T& v)
        {
            return constcast(v);
        }
        template<typename T>
        inline static T*                                                                                as(const T* v)
        {
            return constcast(v);
        }
        template<typename R, typename T>
        inline static R&                                                                                as(const T& v)
        {
            T& r = const_cast<T&>(v);
            return dynamic_cast<R&>(r);
        }
        template<typename R, typename T>
        inline static R*                                                                                as(const T* v)
        {
            T* r = const_cast<T*>(v);
            return dynamic_cast<R*>(r);
        }
        template<typename R, typename T>
        inline static std::shared_ptr<R>                                                                as(const std::shared_ptr<T>& v)
        {
            if (IsNulll(v))
            {
                return Nulll<R>();
            }
            return std::dynamic_pointer_cast<R>(v);
        }
        template<typename R, typename T>
        inline static boost::shared_ptr<R>                                                              as(const boost::shared_ptr<T>& v)
        {
            if (IsNulll(v))
            {
                return Nulll2<R>();
            }
            return boost::dynamic_pointer_cast<R>(v);
        }
        template<typename T>
        inline static std::shared_ptr<T>                                                                Nulll()
        {
            return std::shared_ptr<T>();
        }
        template<typename T>
        inline static std::shared_ptr<T>                                                                Null()
        {
            return std::shared_ptr<T>();
        }
        template<typename T>
        inline static boost::shared_ptr<T>                                                              Nulll2()
        {
            return boost::shared_ptr<T>();
        }
        template<typename T>
        inline static boost::shared_ptr<T>                                                              Null2()
        {
            return boost::shared_ptr<T>();
        }
        template<typename T>
        inline static bool                                                                              IsNulll(const T& v)
        {
            return v ? true : false;
        }
        template<typename T>
        inline static bool                                                                              IsNulll(const std::shared_ptr<T>& v)
        {
            return NULL == addressof(v);
        }
        template<typename T>
        inline static bool                                                                              IsNulll(const boost::shared_ptr<T>& v)
        {
            return NULL == addressof(v);
        }
        template<typename T>
        inline static bool                                                                              IsNotNulll(const T& v)
        {
            return v ? false : true;
        }
        template<typename T>
        inline static bool                                                                              IsNotNulll(const std::shared_ptr<T>& v)
        {
            return NULL != addressof(v);
        }
        template<typename T>
        inline static bool                                                                              IsNotNulll(const boost::shared_ptr<T>& v)
        {
            return NULL != addressof(v);
        }
    };

    template<typename T1, typename T2>
    struct Tuple
    {
    public:
        T1                      Item1;
        T2                      Item2;

    public:
        Tuple(const T1& a1, const T2& a2) : Item1(a1), Item2(a2) {}
    };

    template<typename T1, typename T2, typename T3>
    struct Tuple3
    {
    public:
        T1                      Item1;
        T2                      Item2;
        T3                      Item3;

    public:
        Tuple3(const T1& a1, const T2& a2, const T3& a3) : Item1(a1), Item2(a2), Item3(a3) {}
    };

    template<typename T1, typename T2, typename T3, typename T4>
    struct Tuple4
    {
    public:
        T1                      Item1;
        T2                      Item2;
        T3                      Item3;
        T4                      Item4;

    public:
        Tuple4(const T1& a1, const T2& a2, const T3& a3, const T4& a4) : Item1(a1), Item2(a2), Item3(a3), Item4(a4) {}
    };

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    struct Tuple5
    {
    public:
        T1                      Item1;
        T2                      Item2;
        T3                      Item3;
        T4                      Item4;
        T5                      Item5;

    public:
        Tuple5(const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5) : Item1(a1), Item2(a2), Item3(a3), Item4(a4), Item5(a5) {}
    };

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct Tuple6
    {
    public:
        T1                      Item1;
        T2                      Item2;
        T3                      Item3;
        T4                      Item4;
        T5                      Item5;
        T6                      Item6;

    public:
        Tuple6(const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5, const T6& a6) : Item1(a1), Item2(a2), Item3(a3), Item4(a4), Item5(a5), Item6(a6) {}
    };

    class Reference // RTTI(/GR)
    {
    public:
        Reference()
        {
            _managedPointer = make_shared_null<void*>();
        }
        Reference(const void* value)
        {
            if (!value) {
                _managedPointer = make_shared_null<void*>();
            }
            else {
                _managedPointer = warp_shared_object<void*>((void**)value);
            }
        }
        template<typename T>
        Reference(const std::shared_ptr<T>& value)
        {
            T* pointer = value.get();
            if (!pointer) {
                _managedPointer = make_shared_null<void*>();
            }
            else {
                _managedPointer = std::shared_ptr<void*>((void**)pointer, [value](void** p) {
                    std::shared_ptr<T>& inl = Object::constcast(value);
                    if (inl) {
                        inl.reset();
                    }
                });
            }
        }
        ~Reference()
        {
            Clear();
        }

    public:
        inline Reference&                                                                   operator=(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            if (this) {
                left._managedPointer = right._managedPointer;
            }
            return left;
        }
        inline bool                                                                         operator==(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            void* x = Object::addressof(left);
            void* y = Object::addressof(right);
            if (x == y) {
                return true;
            }
            if (x == NULL && !right._managedPointer) {
                return true;
            }
            if (!left._managedPointer && y == NULL) {
                return true;
            }
            return left._managedPointer == right._managedPointer;
        }
        inline bool                                                                         operator!=(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            return !(left == right);
        }
        inline bool                                                                         operator<(const Reference& right) const 
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer() < const_cast<Reference&>(right).GetNativePointer();
        }
        inline bool                                                                         operator>(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer() > const_cast<Reference&>(right).GetNativePointer();
        }
        inline bool                                                                         operator>=(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer() >= const_cast<Reference&>(right).GetNativePointer();
        }
        inline bool                                                                         operator<=(const Reference& right) const
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer() >= const_cast<Reference&>(right).GetNativePointer();
        }
        inline unsigned char*                                                               operator&() const
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer();
        }
        inline unsigned char&                                                               operator*() const
        {
            Reference& left = const_cast<Reference&>(*this);
            return *(unsigned char*)(left.GetNativePointer());
        }
        inline bool                                                                         operator!() const
        {
            Reference& left = const_cast<Reference&>(*this);
            return NULL == left.GetNativePointer();
        }

    public:
        inline operator bool() const
        {
            Reference& left = const_cast<Reference&>(*this);
            return NULL != left.GetNativePointer();
        }
        inline operator void*() const
        {
            Reference& left = const_cast<Reference&>(*this);
            return left.GetNativePointer();
        }

    public:
        inline unsigned char*                                                               GetNativePointer() const
        {
            if (!this) {
                return NULL;
            }
            Reference& left = const_cast<Reference&>(*this);
            return (unsigned char*)left._managedPointer.get();
        }
        template<typename T>
        inline T*                                                                           GetNativePointer() const
        {
            void* p = GetNativePointer();
            if (!p) {
                return NULL;
            }
            return (T*)(p);
        }

    public:
        template<typename T>
        inline std::shared_ptr<T>                                                           GetPointer() const
        {
            T* po = GetNativePointer<T>();
            if (!po) {
                return make_shared_null<T>();
            }

            std::shared_ptr<void*> mp = _managedPointer;
            if (!mp) {
                return make_shared_null<T>();
            }

            return std::shared_ptr<T>(po, [mp](T* p) {
                auto& inl = Object::constcast(mp);
                if (inl) {
                    inl.reset();
                }
            });
        }

    public:
        inline bool                                                                         NotValue() const {
            return !this || !GetNativePointer();
        }
        inline bool                                                                         HasValue() const {
            return !NotValue();
        }
        inline void                                                                         Clear() const {
            if (this) {
                if (_managedPointer) {
                    _managedPointer.reset();
                }
            }
        }

    private:
        mutable std::shared_ptr<void*>                                                      _managedPointer;
    };

    class Try
    {
    public:
        inline Try(const std::function<void()>& block)
            : ___try(block) {
            if (!block) {
                throw std::runtime_error("The block part of the try is not allowed to be Null References");
            }
        }
        inline ~Try() {
            ___try = NULL;
            ___catch = NULL;
            ___finally = NULL;
        }

    public:
        template<typename TException>
        inline Try&                                             Catch() const {
            return Catch<TException>([](TException& e) {});
        }
        template<typename TException>
        inline Try&                                             Catch(const std::function<void(TException& e)>& block) const {
            return Catch<TException>(block, NULL);
        }
        template<typename TException>
        inline Try&                                             Catch(const std::function<void(TException& e)>& block, const std::function<bool(TException& e)>& when) const {
            if (!block) {
                throw std::runtime_error("The block part of the try-catch is not allowed to be Null References");
            }

            if (this) {
                auto previous = ___catch;
                ___catch = [block, previous, when](std::exception* e) {
                    if (previous) {
                        if (previous(e)) {
                            return true;
                        }
                    }
                    TException* exception = dynamic_cast<TException*>(e);
                    if (!exception) {
                        return false;
                    }
                    if (when) {
                        if (!when(*exception)) {
                            return false;
                        }
                    }
                    if (block) {
                        block(*exception);
                    }
                    return true;
                };
            }
            return const_cast<Try&>(*this);
        }
        inline Try&                                             Finally(const std::function<void()>& block) const {
            if (this) {
                ___finally = block;
            }
            return const_cast<Try&>(*this);
        }
        inline int                                              Invoke() const {
            if (!this) {
                return -1;
            }
            if (!___try) {
                return -2;
            }
            int ec = 0;
            std::exception* exception = NULL; // Œ¥¥¶¿Ì“Ï≥£
            try {
                ___try();
            }
            catch (std::exception& e) {
                ec = 1;
                exception = (std::exception*)&reinterpret_cast<const char&>(e);
                if (___catch) {
                    if (___catch(exception)) {
                        exception = NULL;
                    }
                }
            }
            if (___finally) {
                ___finally();
            }
            if (exception) {
                throw *exception;
            }
            return ec;
        }

    private:
        mutable std::function<void()>                           ___try;
        mutable std::function<bool(std::exception*)>            ___catch;
        mutable std::function<void()>                           ___finally;
    };
}

namespace std
{
    template <>
    struct hash<ep::Reference>
    {
        int operator()(const ep::Reference& reference) const
        {
            ep::Reference& r = const_cast<ep::Reference&>(reference);
            return std::hash<void*>{}(r.GetNativePointer());
        }
    };

    namespace tr1
    {
        template <>
        struct hash<ep::Reference>
        {
            int operator()(const ep::Reference& reference) const
            {
                ep::Reference& r = const_cast<ep::Reference&>(reference);
                return std::hash<void*>{}(r.GetNativePointer());
            }
        };
    }
}