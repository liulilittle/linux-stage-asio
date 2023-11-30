#include <vector>
#include <iostream>
#include <string.h>
#include <assert.h>

#include "XiYouSdkClient.h"

XiYouSdkClient::XiYouSdkClient(const char* nomenclature, const char* platform, unsigned char metadatatoken, unsigned short identifier, const IPEndPoint & remoteEP, int fixedSize, int slidingWindowSize) :
    SocketClient(remoteEP, fixedSize, slidingWindowSize),
    m_LastTimeoutTime(0),
    m_LastActivityTime(0),
    m_LastHeartbeatTime(0)
{
    this->m_LastTimeoutTime = SocketClient::GetTickCount();
    this->m_Nomenclature.szBuf[0] = '\x0';
    this->m_Nomenclature.wLen = 0;

    this->m_Platform.szBuf[0] = '\x0';
    this->m_Platform.wLen = 0;

    this->m_Identifier = metadatatoken & 0xff;
    this->m_Identifier |= (identifier & 0xffff) << 8;

    this->m_Nomenclature.wLen = (unsigned short)::strnlen(nomenclature, sizeof(this->m_Nomenclature.szBuf) - 1);
    if (this->m_Nomenclature.wLen > 0)
    {
        memcpy(this->m_Nomenclature.szBuf, (char*)nomenclature, this->m_Nomenclature.wLen);
        this->m_Nomenclature.szBuf[this->m_Nomenclature.wLen] = '\x0';
    }

    this->m_Platform.wLen = (unsigned short)::strnlen(nomenclature, sizeof(this->m_Platform.szBuf) - 1);
    if (this->m_Platform.wLen > 0)
    {
        memcpy(this->m_Platform.szBuf, (char*)platform, this->m_Platform.wLen);
        this->m_Nomenclature.szBuf[this->m_Nomenclature.wLen] = '\x0';
    }
}

XiYouSdkClient::~XiYouSdkClient()
{
    Dispose();
}

XiYouSdkClient::SocketInvokeInfo XiYouSdkClient::FetchInvokeInfo(long long sequenceno)
{
    SocketInvokeInfo info;
    info.createtime = -1;

    InvokeContainer::iterator it = this->m_SocketInvokes.find(sequenceno);
    if (it != this->m_SocketInvokes.end())
    {
        info = it->second;
        this->m_SocketInvokes.erase(it);
    }
    return info;
}

void XiYouSdkClient::OnOpen(EventArgs & e)
{
    this->SendEstablishedOrHeartBeatMessage(false);
    this->ResetLastTick(SocketClient::GetTickCount());
    SocketClient::OnOpen(e);
}

void XiYouSdkClient::OnMessage(SocketMessage& e)
{
    this->ResetLastTick(SocketClient::GetTickCount());
    if (e.GetCommandId() != XiYouSdkCommands_Heartbeat)
    {
        SocketHandler* handler = this->GetHandler((XiYouSdkCommands)e.GetCommandId());
        if (handler != NULL)
        {
            SocketInvokeInfo info = this->FetchInvokeInfo(e.GetSequenceNo());
            if (info.createtime != -1)
            {
                if (NULL != info.sendmsgbuf)
                {
                    delete[] info.sendmsgbuf;
                }

                if (!handler->ProcessAck(e))
                {
                    EventHandler<XiYouSdkInvokeEventArgs>* callback = info.callback;
                    if (NULL != callback)
                    {
                        XiYouSdkInvokeEventArgs invoke;

                        invoke.GetCommandId() = info.handler->GetCommandId();
                        invoke.GetSequenceNo() = info.sequenceno;
                        invoke.GetClient() = this;
                        invoke.GetMessage() = &e;
                        invoke.GetState() = info.state;
                        invoke.GetError() = XiYouSdkInvokeEventArgs::XiYouSdkInvoke_Success;
                        invoke.GetElapsedMilliseconds() = (SocketClient::GetTickCount() - info.createtime);

                        callback->Invoke(this, &invoke);
                    }
                }
            }
            else
            {
                handler->ProcessRequest(e);
            }
        }
        SocketClient::OnMessage(e);
    }
}

void XiYouSdkClient::OnTimeout(XiYouSdkInvokeEventArgs& e)
{

}

void XiYouSdkClient::OnError(EventArgs& e)
{
    this->ResetLastTick();
    SocketClient::OnError(e);
}

void XiYouSdkClient::OnClose(EventArgs& e)
{
    this->ResetLastTick();
    SocketClient::OnClose(e);
}

bool XiYouSdkClient::SendEstablishedOrHeartBeatMessage(bool heartbeat)
{
    SocketMessage message;
    if (heartbeat)
    {
        message.GetCommandId() = XiYouSdkCommands_Heartbeat;
    }
    else
    {
        message.GetCommandId() = XiYouSdkCommands_Established;
    }
    message.GetIdentifier() = this->m_Identifier;
    message.GetClient() = this;
    message.GetSequenceNo() = SocketMessage::NewId();
    message.GetBuffer() = NULL;
    message.GetOffset() = 0;
    message.GetLength() = 0;

    if (message.GetCommandId() == XiYouSdkCommands_Established)
    {
        message.GetBuffer() = (unsigned char*)&this->m_Nomenclature;
        message.GetLength() =
            this->m_Nomenclature.wLen +
            sizeof(this->m_Nomenclature.wLen) +
            this->m_Platform.wLen +
            sizeof(this->m_Platform.wLen);
    }

    return SocketClient::Send(message);
}

int XiYouSdkClient::GetTimeoutInterval()
{
    return 100;
}

int XiYouSdkClient::GetMaxSilencingTime()
{
    return 30000;
}

void XiYouSdkClient::ProcessTimeout()
{
    unsigned long long nowtick64 = SocketClient::GetTickCount();
    unsigned long long nowdiff64 = (nowtick64 - this->m_LastTimeoutTime);
    if (nowdiff64 < (unsigned long long)this->GetTimeoutInterval())
    {
        return /*false*/;
    }
    else
    {
        this->m_LastTimeoutTime = nowtick64;
    }
    std::vector<long long>& erases = this->m_SocketInvokeErases;
    if (!erases.empty())
    {
        erases.clear();
    }
    InvokeContainer::iterator current = this->m_SocketInvokes.begin();
    InvokeContainer::iterator end = this->m_SocketInvokes.end();
    for (; current != end; ++current)
    {
        SocketInvokeInfo& info = current->second;
        if ((SocketClient::GetTickCount() - info.createtime) >= (unsigned long long)info.handler->GetTimeout())
        {
            EventHandler<XiYouSdkInvokeEventArgs>* callback = info.callback;
            SocketHandler* handler = info.handler;

            if (handler->GetRetryCount() > info.currentretrycount++) // 重试ACK
            {
                SocketMessage message;

                message.GetCommandId() = handler->GetCommandId();
                message.GetIdentifier() = this->m_Identifier;
                message.GetClient() = this;
                message.GetSequenceNo() = info.sequenceno;
                message.GetOffset() = 0;
                message.GetBuffer() = (unsigned char*)info.sendmsgbuf;
                message.GetLength() = info.sendmsgbuflen;

                info.createtime = SocketClient::GetTickCount();

                SocketClient::Send(message);
            }
            else
            {
                if (NULL != info.sendmsgbuf)
                {
                    delete[] info.sendmsgbuf;
                }
                info.sendmsgbuflen = 0;
                info.sendmsgbuf = NULL;

                XiYouSdkInvokeEventArgs e;

                e.GetCommandId() = handler->GetCommandId();
                e.GetSequenceNo() = info.sequenceno;
                e.GetClient() = this;
                e.GetMessage() = NULL;
                e.GetState() = info.state;
                e.GetError() = XiYouSdkInvokeEventArgs::XiYouSdkInvoke_Timeout;
                e.GetElapsedMilliseconds() = (SocketClient::GetTickCount() - info.createtime);

                if (!handler->ProcessTimeout(info.sequenceno))
                {
                    if (NULL != callback)
                    {
                        callback->Invoke(this, &e);
                    }
                    this->OnTimeout(e);
                }

                erases.push_back(info.sequenceno);
            }
        }
    }
    for (unsigned int i = 0, len = (unsigned int)erases.size(); i < len; i++)
    {
        long long sequenceno = erases[i];
        InvokeContainer::iterator it = this->m_SocketInvokes.find(sequenceno);
        if (it != this->m_SocketInvokes.end())
        {
            this->m_SocketInvokes.erase(it);
        }
    }
}

void XiYouSdkClient::ResetLastTick(unsigned long long ticks)
{
    this->m_LastActivityTime = ticks;
    this->m_LastHeartbeatTime = ticks;
}

int XiYouSdkClient::DoEvents()
{
    this->ProcessTimeout();
    do
    {
        unsigned long long ticks = SocketClient::GetTickCount();
        int MaxSilencingTime = this->GetMaxSilencingTime();
        assert(MaxSilencingTime > 0);
        if ((0 != this->m_LastActivityTime) && (ticks - this->m_LastActivityTime) >= (unsigned long long)MaxSilencingTime)
        {
            SocketClient::CloseOrAbort(true);
        }
        else if ((0 != this->m_LastHeartbeatTime) && (ticks - this->m_LastHeartbeatTime) >= 1000)
        {
            this->SendEstablishedOrHeartBeatMessage(true);
            this->m_LastHeartbeatTime = SocketClient::GetTickCount();
        }
    } while (false);
    return SocketClient::DoEvents();
}

void XiYouSdkClient::Dispose()
{
    if (!IsDisposed())
    {
        m_LastActivityTime = 0;
        m_LastHeartbeatTime = 0;
        m_LastTimeoutTime = 0;
        m_Nomenclature.wLen = 0;

        m_SocketHandlers.clear();
        m_SocketInvokes.clear();
        m_SocketInvokeErases.clear();
    }
    SocketClient::Dispose();
}

bool XiYouSdkClient::AddHandler(SocketHandler* handler)
{
    if (handler == NULL)
    {
        return false;
    }
    XiYouSdkCommands command = (XiYouSdkCommands)handler->GetCommandId();
    if (NULL != this->GetHandler(command))
    {
        return false;
    }
    else
    {
        this->m_SocketHandlers.insert(std::map<XiYouSdkCommands, SocketHandler*>::value_type(command, handler));
    }
    return false;
}

SocketHandler* XiYouSdkClient::RemoveHandler(XiYouSdkCommands command)
{
    SocketHandler* handler = NULL;
    HandlerContainer::iterator it = this->m_SocketHandlers.find(command);
    if (it != this->m_SocketHandlers.end())
    {
        handler = it->second;
        this->m_SocketHandlers.erase(it);
    }
    return handler;
}

SocketHandler* XiYouSdkClient::GetHandler(XiYouSdkCommands command)
{
    HandlerContainer::iterator it = this->m_SocketHandlers.find(command);
    if (it != this->m_SocketHandlers.end())
    {
        return it->second;
    }
    return NULL;
}

int XiYouSdkClient::GetAllHandler(std::vector<SocketHandler*>& s)
{
    int l = 0;
    for (HandlerContainer::iterator 
        i = m_SocketHandlers.begin(); 
        i != m_SocketHandlers.end(); 
        ++i)
    {
        SocketHandler* h = i->second;
        if (NULL != h)
        {
            s.push_back(h);
            ++l;
        }
    }
    return l;
}

int XiYouSdkClient::RemoveAllHandler(std::vector<SocketHandler*>* s)
{
    int l = (int)m_SocketHandlers.size();
    if (s != NULL)
    {
        l = GetAllHandler(*s);
    }
    m_SocketHandlers.clear();
    return l;
}

int XiYouSdkClient::DeleteAllHandler()
{
    int l = 0;
    for (HandlerContainer::iterator
        i = m_SocketHandlers.begin();
        i != m_SocketHandlers.end();
        ++i)
    {
        SocketHandler* h = i->second;
        if (NULL == h)
        {
            continue;
        }
        else
        {
            ++l;
        }
        delete h;
    }
    m_SocketHandlers.clear();
    return l;
}

bool XiYouSdkClient::InvokeAsync(XiYouSdkCommands command, const char* data, int datalen, EventHandler<XiYouSdkInvokeEventArgs>* callback, const void* state)
{
    return this->InvokeAsync(command, data, datalen, SocketMessage::NewId(), callback, state);
}

bool XiYouSdkClient::InvokeAsync(XiYouSdkCommands command, const char* data, int datalen, long long sequenceno, EventHandler<XiYouSdkInvokeEventArgs>* callback, const void* state)
{
    SocketHandler* handler = this->GetHandler(command);
    if (NULL == handler)
    {
        return false;
    }
    if (this->m_SocketInvokes.find(sequenceno) != this->m_SocketInvokes.end())
    {
        return false;
    }
    SocketMessage message;

    message.GetCommandId() = command;
    message.GetIdentifier() = this->m_Identifier;
    message.GetClient() = this;
    message.GetSequenceNo() = sequenceno;
    message.GetOffset() = 0;
    message.GetBuffer() = (unsigned char*)data;
    message.GetLength() = datalen;

    int retrycount = handler->GetRetryCount();
    if (!SocketClient::Send(message))
    {
        if (retrycount == 0)
        {
            return false;
        }
    }

    SocketInvokeInfo info;

    info.createtime = SocketClient::GetTickCount();
    info.handler = handler;
    info.state = (void*)state;
    info.callback = callback;
    info.sequenceno = sequenceno;

    info.currentretrycount = 0; // 当前重试ACK次数
    if (datalen <= 0 || retrycount <= 0)
    {
        info.sendmsgbuf = NULL;
        info.sendmsgbuflen = 0;
    }
    else
    {
        info.sendmsgbuf = new char[datalen];
        info.sendmsgbuflen = datalen;
        memcpy((char*)info.sendmsgbuf, data, datalen);
    }

    this->m_SocketInvokes.insert(InvokeContainer::value_type(sequenceno, info));

    return true;
}
