#pragma once

#pragma pack(push, 1) // 结构的字节对齐为0x00000001（严格）

#ifdef WIN32
#pragma warning(disable: 4103)
#endif

#include <iostream>
#include <strstream>

#ifndef NULL
#define NULL 0
#endif

#ifndef unsafe
#define unsafe
#endif

#ifndef WIN32
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#else
#include <mutex>
#include <functional>
#endif

enum AddressFamily // == AF_* 
{
    AddressFamily_None = -1,
    AddressFamily_InterNetwork = 2,
    AddressFamily_InterNetworkV6 = 23,
};

class IPAddress
{
private:
    char                                                                    m_Address[16];
    int                                                                     m_AddressSize;
    AddressFamily                                                           m_AddressFamily;

public:
    IPAddress();
    IPAddress(int address);
    IPAddress(const char* address);

public:
    virtual AddressFamily                                                   GetAddressFamily();

public:
    virtual unsigned char*                                                  GetAddressBytes();
    virtual int                                                             GetAddressBytesLength();
    virtual long long int                                                   GetAddress();

public:
    virtual std::string                                                     ToString();
};

class IPEndPoint
{
private:
    int                                                                     m_Port;
    IPAddress                                                               m_Address;

public:
    IPEndPoint();
    IPEndPoint(const IPAddress& address, int port);

public:
    virtual IPAddress&                                                      GetAddress();
    virtual int                                                             GetPort();
    virtual AddressFamily                                                   GetAddressFamily();

public:
    static const int                                                        MaxPort = 65535;
    static const int                                                        MinPort = 0;
};

class SocketClient;

class EventArgs
{
private:
    static EventArgs                                                        m_Empty;

public:
    static EventArgs&                                                       GetEmpty() { return EventArgs::m_Empty; }
};

class SocketMessage : public EventArgs
{
private:
    SocketClient*                                                           m_Socket;
    unsigned char*                                                          m_Buffer;
    long long                                                               m_Offest;
    long long                                                               m_Length;
    unsigned short                                                          m_CommandId;
    long long                                                               m_Identifier;
    long long                                                               m_SequenceNo;

public:
    SocketMessage();
    virtual ~SocketMessage();

public:
    void                                                                    Clear();
    bool                                                                    IsFixedMode();

public:
    virtual unsigned short&                                                 GetCommandId() { return this->m_CommandId; }
    virtual long long&                                                      GetIdentifier() { return this->m_Identifier; }
    virtual long long&                                                      GetSequenceNo() { return this->m_SequenceNo; }

public:
    virtual long long&                                                      GetOffset() { return this->m_Offest; }
    virtual long long&                                                      GetLength() { return this->m_Length; }
    virtual unsigned char*&                                                 GetBuffer() { return this->m_Buffer; }

public:
    virtual SocketClient*&                                                  GetClient() { return this->m_Socket; }

public:
    static long long                                                        NewId();

public:
    template<typename T>
    static bool                                                             Deserialize(T& model, SocketMessage& e)
    {
        int count = (int)e.GetLength();
        unsigned char* buf = e.GetBuffer();
        return model.Deserialize(buf, count);
    };
};

class SocketClient;

class SocketHandler
{
private:
    unsigned short                                                          m_CommandId;

public:
    SocketHandler() : m_CommandId(0)
    {

    }
    SocketHandler(unsigned short commandId) : m_CommandId(commandId)
    {

    }
    virtual ~SocketHandler()
    {

    }

public:
    virtual void                                                            ProcessRequest(SocketMessage& message) { };
    virtual bool                                                            ProcessAck(SocketMessage& message) { return false; };
    virtual bool                                                            ProcessTimeout(long long sequenceNo) { return false; };

public:
    virtual unsigned short                                                  GetCommandId() { return this->m_CommandId; }
    virtual int                                                             GetTimeout() { return 3000; }
    virtual int                                                             GetRetryCount() { return 0; /* NOT REACK */ }

public:
    template<typename TResponse>
    bool                                                                    ResponseWrite(SocketClient* socket, long long acksequence, TResponse& response)
    {
        std::strstream stream;
        response.Serialize(stream);
        return ResponseWrite(socket, acksequence, (unsigned char*)stream.str(), (int)stream.tellp());
    }
    bool                                                                    ResponseWrite(
        SocketClient*           socket,
        long long               acksequence,
        unsigned char*          buffer,
        int                     count)
    {
        return ResponseWrite(socket, acksequence, buffer, 0, count);
    }
    virtual bool                                                            ResponseWrite(
        SocketClient*           socket,
        long long               acksequence,
        unsigned char*          buffer,
        int                     offset,
        int                     count);
};

template<class TEventArgs>
class EventHandler
{
#pragma pack(1)
private:
    void*                                                                   m_pTargetPtr;
    bool                                                                    m_bClazzMode;
    void*                                                                   m_pMethodAddr;
    void*                                                                   m_pMethodAddrAux;

public:
    EventHandler() :
        m_pTargetPtr(NULL),
        m_bClazzMode(false),
        m_pMethodAddr(NULL),
        m_pMethodAddrAux(NULL)
    {

    }
    EventHandler(const EventHandler<TEventArgs>& right)
    {
        EventHandler<TEventArgs>& leftRef_ = *this;
        leftRef_ = right;
    }

public:
    inline bool                                                             IsClass() { return m_bClazzMode; }
    inline void*&                                                           GetMethod() { return m_pMethodAddr; }
    inline void*&                                                           GetTarget() { return m_pTargetPtr; }
    inline void                                                             Invoke(void* obj, TEventArgs* e)
    {
        if (m_bClazzMode)
        {
            typedef void(*PFN_EX)(void*, void*, void*, TEventArgs*);
            PFN_EX pfn = (PFN_EX)m_pMethodAddrAux;
            pfn(m_pTargetPtr, m_pMethodAddr, obj, e);
        }
        else
        {
            typedef void(*PFN_EX)(void*, TEventArgs*);
            PFN_EX pfn = (PFN_EX)m_pMethodAddr;
            pfn(obj, e);
        }
    }
    inline void                                                             Clear()
    {
        this->m_bClazzMode = false;
        this->m_pMethodAddr = NULL;
        this->m_pTargetPtr = NULL;
        this->m_pMethodAddrAux = NULL;
    }

public:
    inline const EventHandler<TEventArgs>&                                  operator=(const EventHandler<TEventArgs>& right)
    {
        EventHandler<TEventArgs>& rightRef_ = const_cast<EventHandler<TEventArgs>&>(right);

        this->m_bClazzMode = rightRef_.m_bClazzMode;
        this->m_pTargetPtr = rightRef_.m_pTargetPtr;
        this->m_pMethodAddr = rightRef_.m_pMethodAddr;
        this->m_pMethodAddrAux = rightRef_.m_pMethodAddrAux;

        return *this;
    }

public:
    static EventHandler<TEventArgs>*                                        Create(EventHandler<TEventArgs>* handler_, void(*callback_)(void* obj, TEventArgs* e))
    {
        if (handler_ == NULL || callback_ == NULL)
        {
            return NULL;
        }

        handler_->m_pTargetPtr = NULL;
        handler_->m_bClazzMode = false;
        handler_->m_pMethodAddr = (void*)callback_;
        handler_->m_pMethodAddrAux = NULL;

        return handler_;
    }

#pragma pack()
    template<typename TType>
    static EventHandler<TEventArgs>*                                        Create(EventHandler<TEventArgs>* handler_, TType* target_, void(*callback_)(TType* target, TEventArgs* e))
    {
        if (handler_ == NULL || callback_ == NULL)
        {
            return NULL;
        }
#pragma pack(1)
        class InnerEventClazz
        {
        public:
            static void Invoke(void* target, void* method, void* obj, TEventArgs* e)
            {
                typedef void(*PFN)(TType* target, TEventArgs* e);
                PFN p = NULL;
                {
                    union
                    {
                        PFN x;
                        void* y;
                    } u;
                    u.y = method;
                    p = u.x;
                }
                TType* self = static_cast<TType*>(target);
                p(self, e);
            }
        };

        handler_->m_bClazzMode = true;
        handler_->m_pTargetPtr = target_;
        handler_->m_pMethodAddr = *(void**)&reinterpret_cast<const char&>(callback_);
        handler_->m_pMethodAddrAux = (void*)&InnerEventClazz::Invoke;

        return handler_;
#pragma pack()
    }

    template<typename TType>
    static EventHandler<TEventArgs>*                                        Create(EventHandler<TEventArgs>* handler_, TType* target_, void(*callback_)(TType* target, void* sender, TEventArgs* e))
    {
        if (handler_ == NULL || callback_ == NULL)
        {
            return NULL;
        }
#pragma pack(1) 
        class InnerEventClazz
        {
        public:
            static void Invoke(void* target, void* method, void* obj, TEventArgs* e)
            {
                typedef void(*PFN)(TType* target, void* obj, TEventArgs* e);
                PFN p = NULL;
                {
                    union
                    {
                        PFN x;
                        void* y;
                    } u;
                    u.y = method;
                    p = u.x;
                }
                TType* self = static_cast<TType*>(target);
                p(self, obj, e);
            }
        };

        handler_->m_bClazzMode = true;
        handler_->m_pTargetPtr = target_;
        handler_->m_pMethodAddr = *(void**)&reinterpret_cast<const char&>(callback_);
        handler_->m_pMethodAddrAux = (void*)&InnerEventClazz::Invoke;

        return handler_;
#pragma pack()
    }
};

class SocketClient // BSD: SOCKET, Rector
{
private:
    static const int                                                        MTU = 1500;
    static const int                                                        PPP = 8;
    static const int                                                        IPV4HDR = 20;
    static const int                                                        TCPHDR = 20;
    static const int                                                        MSS = 1400; // HuaWei: 1426~1428
    static const int                                                        HEADERKEY = 0x2A;

private:
    enum SocketState
    {
        SocketState_Default,
        SocketState_Connecting,
        SocketState_Connected,
        SocketState_Disconnect,
        SocketState_CleanUp,
    };

private:
#pragma pack(push, 1)
    struct MessageHeader
    {
        unsigned char                                                       fk;
        int                                                                 len;
        unsigned short                                                      cmd;
        long long                                                           seq;
        long long                                                           id;
    };
#pragma pack(pop)
    class MessageReceiver
    {
    private:
        SocketClient*                                                       m_Socket;
        char                                                                m_Buffer[MSS];
        unsigned char*                                                      m_StreamPtr;
        bool                                                                m_RecvedHeader;
        int                                                                 m_TotalCount;
        int                                                                 m_RecvOffset;
        MessageHeader                                                       m_SaveHeader;

    public:
        MessageReceiver(SocketClient* socket);
        virtual ~MessageReceiver();
        void                                                                Reset();
        int                                                                 DoRecvBuffer();

    private:
        void                                                                DoRecvMessage();
        bool                                                                IsFixedMode();
    };

private:
    IPEndPoint                                                              m_RemoteEP;
    int                                                                     m_SocketHandle;
    SocketState                                                             m_State;
    int                                                                     m_ConnectTimeout;
    int                                                                     m_ReConnectDelayTime;
    int                                                                     m_FixedSize;
    unsigned long long                                                      m_LastCountTime;
    int                                                                     m_MaxHandleFrameNum;
    int                                                                     m_SlidingWindowSize;
    MessageReceiver                                                         m_SocketReceiver;
#ifdef WIN32
    std::recursive_mutex                                                    m_syncobj;
#else
    boost::recursive_mutex                                                  m_syncobj;
#endif

private:
    EventHandler<EventArgs>*                                                m_OnClose;
    EventHandler<EventArgs>*                                                m_OnError;
    EventHandler<EventArgs>*                                                m_OnOpen;
    EventHandler<SocketMessage>*                                            m_OnMessage;

public:
    SocketClient(const IPEndPoint& remoteEP, int fixedSize = 0, int slidingWindowSize = 0);
    virtual ~SocketClient();

private:
    bool                                                                    HandleIsInvalid(int fd);
    void                                                                    ProcessConnected();

protected:
    enum SelectMode
    {
        SelectMode_SelectRead,
        SelectMode_SelectWrite,
        SelectMode_SelectError,
    };
    bool                                                                    Poll(int s, int microSeconds, SelectMode mode);
    int                                                                     GetSocketError(int s);
    bool                                                                    CloseOrAbort(bool aborting);

public:
    virtual bool                                                            IsConnected();
    virtual bool                                                            IsAvailable();

public:
    bool                                                                    IsDisposed();
    bool                                                                    IsFixedMode();
    int                                                                     GetFixedSize();
    int                                                                     DangerousGetHandle();

public:
    int                                                                     GetConnectTimeout();
    void                                                                    SetConnectTimeout(int value);
    int                                                                     GetReConnectDelayTime();
    void                                                                    SetReConnectDelayTime(int value);

public:
    enum
    {
        OPEN_THE_OBJECT_HAS_BEEN_DISPOSED                                   = -1,
        OPEN_CURRENT_STATE_PREVENTS_AN_OPERATION_FROM_BEING_PERFORMED       = -2,
        OPEN_THE_NUMERIC_IP_ADDRESS_IS_INCORRECT                            = -3,
        OPEN_NEW_FD_HANDLES_CANNOT_BE_ALLOCATED_FROM_THE_SYSTEM_SOCKET_POOL = -4, // SSP
        OPEN_UNABLE_TO_SET_SOCKET_FD_THE_NODELAY_SETTINGS                   = -5, 
        OPEN_UNABLE_TO_SET_SOCKET_SLIDING_WINDOW_SIZE_THE_VALUE             = -6,
        OPEN_UNABLE_TO_CONNECT_AND_UNABLE_TO_RETRY_SOCKET_CONNECTION        = -7,
        OPEN_SOCKET_CLIENT_CONNECTION_ESTABLISHED                           = 0,
        OPEN_SOCKET_CLIENT_CONNECTION_ESTABLISHING                          = 1,
        OPEN_TRY_TO_REESTABLISH_A_SOCKET_CONNECTION                         = 2,
    };
    virtual int                                                             Open();
    void                                                                    Close();
    virtual void                                                            Dispose();
    virtual bool                                                            Abort();
    virtual bool                                                            Send(SocketMessage& message);

public:
    virtual IPEndPoint&                                                     GetRemoteEndPoint();
    virtual IPEndPoint                                                      GetLocalEndPoint();

public:
    virtual int                                                             DoEvents();
    virtual int                                                             GetMaxHandleFrameNumber();
    virtual void                                                            SetMaxHandleFrameNumber(int value);

public:
    virtual void                                                            AddErrorEvent(EventHandler<EventArgs>* e) { AddEvent(this->m_OnError, e); };
    virtual void                                                            RemoveErrorEvent(EventHandler<EventArgs>* e) { RemoveEvent(this->m_OnError, e); };

    virtual void                                                            AddCloseEvent(EventHandler<EventArgs>* e) { AddEvent(this->m_OnClose, e); };
    virtual void                                                            RemoveCloseEvent(EventHandler<EventArgs>* e) { RemoveEvent(this->m_OnClose, e); };

    virtual void                                                            AddOpenEvent(EventHandler<EventArgs>* e) { AddEvent(this->m_OnOpen, e); };
    virtual void                                                            RemoveOpenEvent(EventHandler<EventArgs>* e) { RemoveEvent(this->m_OnOpen, e); };

    virtual void                                                            AddMessageEvent(EventHandler<SocketMessage>* e) { AddEvent(this->m_OnMessage, e); };
    virtual void                                                            RemoveMessageEvent(EventHandler<SocketMessage>* e) { RemoveEvent(this->m_OnMessage, e); };

private:
    template<typename TEventArgs>
    void                                                                    AddEvent(EventHandler<TEventArgs>*& x, EventHandler<TEventArgs>* y)
    {
        if (x != y)
        {
            x = y;
        }
    }
    template<typename TEventArgs>
    void                                                                    RemoveEvent(EventHandler<TEventArgs>*& x, EventHandler<TEventArgs>* y)
    {
        if (x == y)
        {
            x = NULL;
        }
    }

protected:
    virtual void                                                            OnReConnect(EventArgs& e);
    virtual void                                                            OnMessage(SocketMessage& e);
    virtual void                                                            OnOpen(EventArgs& e);
    virtual void                                                            OnError(EventArgs& e);
    virtual void                                                            OnClose(EventArgs& e);

protected:
    virtual int                                                             ConnectSocket(int address, int port);
    virtual bool                                                            IsNoDelay();
    virtual bool                                                            IsReduceSendMemoryFragmentation();

public:
    static unsigned long long                                               GetTickCount(bool microseconds = false);
};

#pragma pack(pop) // 结构的字节对齐为0x00000001（严格）