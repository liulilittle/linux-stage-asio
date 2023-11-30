#pragma once

#include <map>
#ifdef WIN32
#include <hash_map>
#include <hash_set>
#else

#include "util/Util.h"
#endif

#include "SocketClient.h"

enum XiYouSdkCommands
{
    XiYouSdkCommands_Heartbeat                                          = 0x7801, // 心跳
    XiYouSdkCommands_Established                                        = 0x7802, // 建立
    XiYouSdkCommands_LoginAccount                                       = 0x7803, // 账户鉴权
    XiYouSdkCommands_FeedbackAccount                                    = 0x7804, // 用户反馈
    XiYouSdkCommands_CreateOrder                                        = 0x7805, // 创建订单
    XiYouSdkCommands_GMWebInstruction                                   = 0x7806, // GM指令
    XiYouSdkCommands_PayAckOrder                                        = 0x7807, // 支付订单
};

class XiYouSdkInvokeEventArgs : public EventArgs
{
public:
    enum Error
    {
        XiYouSdkInvoke_Error,
        XiYouSdkInvoke_Timeout,
        XiYouSdkInvoke_Success
    };

private:
    XiYouSdkInvokeEventArgs::Error                                      m_Error;
    SocketClient*                                                       m_Socket;
    long long                                                           m_SequenceNo;
    unsigned short                                                      m_CommandId;
    long long                                                           m_ElapsedMilliseconds;
    void*                                                               m_State;
    SocketMessage*                                                      m_Message;

public:
    XiYouSdkInvokeEventArgs() :
        m_Error(XiYouSdkInvokeEventArgs::XiYouSdkInvoke_Error),
        m_Socket(NULL),
        m_SequenceNo(0),
        m_CommandId(0),
        m_ElapsedMilliseconds(-1),
        m_State(NULL),
        m_Message(NULL)
    {

    }

public:
    long long&                                                          GetSequenceNo() { return m_SequenceNo; };
    unsigned short&                                                     GetCommandId() { return m_CommandId; };

public:
    SocketMessage*&                                                     GetMessage() { return m_Message; };
    SocketClient*&                                                      GetClient() { return m_Socket; }
    void*&                                                              GetState() { return m_State; }
    XiYouSdkInvokeEventArgs::Error&                                     GetError() { return m_Error; };

public:
    long long&                                                          GetElapsedMilliseconds() { return m_ElapsedMilliseconds; }
};

class XiYouSdkClient : public SocketClient
{
private:
    struct SocketInvokeInfo
    {
        SocketHandler*                                                  handler;
        EventHandler<XiYouSdkInvokeEventArgs>*                          callback;
        void*                                                           state;
        long long                                                       sequenceno;
        long long                                                       createtime;
        int                                                             currentretrycount;
        const char*                                                     sendmsgbuf;
        int                                                             sendmsgbuflen;
    };

private:
#ifdef WIN32
    typedef stdext::hash_map<unsigned short, SocketHandler*>            HandlerContainer;
    typedef stdext::hash_map<long long, SocketInvokeInfo>               InvokeContainer;
#else
    typedef HASHMAPDEFINE(unsigned short, SocketHandler*)               HandlerContainer;
    typedef HASHMAPDEFINE(long long, SocketInvokeInfo)                  InvokeContainer;
#endif

private:
    long long                                                           m_Identifier;
    volatile unsigned long long                                         m_LastTimeoutTime;
    HandlerContainer                                                    m_SocketHandlers;
    InvokeContainer                                                     m_SocketInvokes;
    std::vector<long long>                                              m_SocketInvokeErases;
    struct
    {
        struct
        {
            unsigned short                                              wLen;
            unsigned char                                               szBuf[260];
        }                                                               m_Nomenclature;
        struct
        {
            unsigned short                                              wLen;
            unsigned char                                               szBuf[260];
        }                                                               m_Platform;
    };
    unsigned long long                                                  m_LastActivityTime;
    unsigned long long                                                  m_LastHeartbeatTime;

public:
    XiYouSdkClient(const char* nomenclature, const char* platform, unsigned char metadatatoken, unsigned short identifier, const IPEndPoint& remoteEP, int fixedSize = 0, int slidingWindowSize = 0);
    virtual ~XiYouSdkClient();

private:
    SocketInvokeInfo                                                    FetchInvokeInfo(long long sequenceno);

protected:
    virtual void                                                        OnOpen(EventArgs& e);
    virtual void                                                        OnMessage(SocketMessage& e);
    virtual void                                                        OnTimeout(XiYouSdkInvokeEventArgs& e);
    virtual void                                                        OnError(EventArgs& e);
    virtual void                                                        OnClose(EventArgs& e);

protected:
    bool                                                                SendEstablishedOrHeartBeatMessage(bool heartbeat);
    virtual int                                                         GetTimeoutInterval();
    virtual int                                                         GetMaxSilencingTime();

private:
    void                                                                ProcessTimeout();
    void                                                                ResetLastTick(unsigned long long ticks = 0);

public:
    virtual int                                                         DoEvents();
    virtual void                                                        Dispose();

public:
    virtual bool                                                        AddHandler(SocketHandler* handler);
    virtual SocketHandler*                                              RemoveHandler(XiYouSdkCommands command);
    virtual SocketHandler*                                              GetHandler(XiYouSdkCommands command);

public:
    virtual int                                                         GetAllHandler(std::vector<SocketHandler*>& s);
    virtual int                                                         RemoveAllHandler(std::vector<SocketHandler*>* s = NULL);
    unsafe virtual int                                                  DeleteAllHandler();

public:
    bool                                                                InvokeAsync(XiYouSdkCommands command, const char* data, int datalen, EventHandler<XiYouSdkInvokeEventArgs>* callback, const void* state);
    bool                                                                InvokeAsync(XiYouSdkCommands command, const char* data, int datalen, long long sequenceno, EventHandler<XiYouSdkInvokeEventArgs>* callback, const void* state);

public:
    static XiYouSdkClient*                                              Cast(const void* obj) { return obj == NULL ? NULL : static_cast<XiYouSdkClient*>(const_cast<void*>(obj)); }
};