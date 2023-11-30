#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <assert.h>

#include <sys/types.h>
#ifdef WIN32
#include <stdint.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <error.h>
#include <sys/poll.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <strstream>

#include "SocketClient.h"

EventArgs EventArgs::m_Empty;

SocketClient::SocketClient(const IPEndPoint& remoteEP, int fixedSize, int slidingWindowSize) :
    m_RemoteEP(remoteEP),
    m_SocketHandle(~0),
    m_State(SocketState_Default),
    m_ConnectTimeout(1500),
    m_ReConnectDelayTime(1000),
    m_FixedSize(fixedSize),
    m_LastCountTime(0),
    m_MaxHandleFrameNum(0),
    m_SlidingWindowSize(slidingWindowSize),
    m_SocketReceiver(this)
{
    m_OnClose = NULL;
    m_OnError = NULL;
    m_OnOpen = NULL;
    m_OnMessage = NULL;
}

SocketClient::~SocketClient()
{
    Dispose();
}

bool SocketClient::HandleIsInvalid(int fd)
{
    return (fd == 0 || fd == ~0); // 按照BSD的规范，FD默认值一般为~0，不过并不清楚LINUX是否遵循了规范。
}

bool SocketClient::CloseOrAbort(bool aborting)
{
    bool doEvt = false;
    if (this->m_State != SocketState_Default)
    {
        int sockfd = this->m_SocketHandle;
        if (!this->HandleIsInvalid(sockfd))
        {
#ifdef WIN32
            shutdown(sockfd, SD_BOTH);
            closesocket(sockfd);
#else  
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
#endif
            doEvt = true;
        }
    }
    this->m_State = SocketState_Disconnect; // FUTARU
    this->m_SocketHandle = ~0;
    this->m_SocketReceiver.Reset();
    this->m_LastCountTime = SocketClient::GetTickCount();
    if (doEvt)
    {
        if (aborting)
        {
            this->OnError(EventArgs::GetEmpty());
        }
        else
        {
            this->OnClose(EventArgs::GetEmpty());
        }
    }
    return doEvt;
}

void SocketClient::ProcessConnected()
{
    this->m_State = SocketState_Connected;
    this->OnOpen(EventArgs::GetEmpty());
}

bool SocketClient::Poll(int s, int microSeconds, SelectMode mode)
{
    if (this->HandleIsInvalid(s))
    {
        return false;
    }

#ifdef WIN32
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(s, &fdset);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = microSeconds;

    int hr = -1;
    if (mode == SelectMode_SelectRead)
    {
        hr = select((s + 1), &fdset, NULL, NULL, &tv) > 0;
    }
    else if (mode == SelectMode_SelectWrite)
    {
        hr = select((s + 1), NULL, &fdset, NULL, &tv) > 0;
    }
    else
    {
        hr = select((s + 1), NULL, NULL, &fdset, &tv) > 0;
    }

    if (hr > 0)
    {
        return FD_ISSET(s, &fdset);
    }
#else
    struct pollfd fds[1];
    memset(fds, 0, sizeof(fds));

    int events = POLLERR;
    if (mode == SelectMode_SelectRead)
    {
        events = POLLIN;
    }
    else if (mode == SelectMode_SelectWrite)
    {
        events = POLLOUT;
    }
    else
    {
        events = POLLERR;
    }

    fds->fd = s;
    fds->events = events;

    int nfds = 1;
    int hr = poll(fds, nfds, microSeconds / 1000);
    if (hr > 0)
    {
        if ((fds->revents & events) == events)
        {
            return true;
        }
    }
#endif
    return false;
}

int SocketClient::GetSocketError(int s)
{
    int err = 0;
    socklen_t len = sizeof(err);
    int hr = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
    if (hr == 0)
    {
        return err;
    }
    return hr;
}

bool SocketClient::IsConnected()
{
    return this->m_State == SocketState_Connected;
}

bool SocketClient::IsAvailable()
{
    return this->IsConnected();
}

bool SocketClient::IsDisposed()
{
    return this->m_State == SocketState_CleanUp;
}

bool SocketClient::IsFixedMode()
{
    return this->m_FixedSize > 0;
}

int SocketClient::GetFixedSize()
{
    return this->m_FixedSize;
}

int SocketClient::DangerousGetHandle()
{
    return this->m_SocketHandle;
}

int SocketClient::GetConnectTimeout()
{
    return this->m_ConnectTimeout;
}

void SocketClient::SetConnectTimeout(int value)
{
    this->m_ConnectTimeout = value;
}

int SocketClient::GetReConnectDelayTime()
{
    return this->m_ReConnectDelayTime;
}

void SocketClient::SetReConnectDelayTime(int value)
{
    this->m_ReConnectDelayTime = value;
}

int SocketClient::Open()
{
    if (this->IsDisposed())
    {
        return SocketClient::OPEN_THE_OBJECT_HAS_BEEN_DISPOSED;
    }

    int sockfd = this->m_SocketHandle;
    if (this->HandleIsInvalid(sockfd))
    {
        return this->ConnectSocket(*(const int*)this->m_RemoteEP.GetAddress().GetAddressBytes(), this->m_RemoteEP.GetPort());
    }
    return SocketClient::OPEN_CURRENT_STATE_PREVENTS_AN_OPERATION_FROM_BEING_PERFORMED;
}

void SocketClient::Close()
{
    this->Dispose();
}

void SocketClient::Dispose()
{
    if (!this->IsDisposed())
    {
        this->CloseOrAbort(false);
        this->m_State = SocketState_CleanUp;
    }
}

bool SocketClient::Abort()
{
    return this->CloseOrAbort(true);
}

bool SocketClient::Send(SocketMessage& message)
{
    if (this->IsDisposed())
    {
        return false;
    }
    if (this->m_State != SocketState_Connected)
    {
        return false;
    }
    int sockfd = this->m_SocketHandle;
    if (this->HandleIsInvalid(sockfd))
    {
        return false;
    }
    char header_buffer[sizeof(MessageHeader)];
    MessageHeader* header_ptr = (MessageHeader*)header_buffer;
    header_ptr->fk = HEADERKEY;

    header_ptr->id = message.GetIdentifier();
    header_ptr->cmd = message.GetCommandId();
    header_ptr->seq = message.GetSequenceNo();
    header_ptr->len = (int)message.GetLength();

    if (NULL == message.GetBuffer() || message.GetOffset() < 0)
    {
        header_ptr->len = 0;
    }

    if (this->IsReduceSendMemoryFragmentation())
    {
        int hr = 0;
        try
        {
            m_syncobj.lock();
            do
            {
                // 可以向下方式发送数据报，当然有个前提就是说这个套接字不能同一时间内被多个人调用send不然包就会出现故障。
                int flag = 0;
#ifndef WIN32
                flag = MSG_NOSIGNAL;
#endif
                hr = send(sockfd, header_buffer, sizeof(MessageHeader), flag); // 帧头的片
                if (hr < 0)
                {
                    break;
                }

                if (header_ptr->len > 0)
                {
                    char* packet_data = (char*)(message.GetBuffer() + message.GetOffset());
                    hr = send(sockfd, packet_data, header_ptr->len, flag); // 载荷的片
                                                                           // TCP/IP协议簇对于以上的情况会自动组包的，调用send != 立即发出，它需要先放入发送的滑块窗口(send_cwnd)
                }
            } while (false);
            m_syncobj.unlock();
        }
        catch (...)
        {
            hr = -1;
        }

        if (hr < 0)
        {
            this->CloseOrAbort(true);
            return false;
        }
    }
    else
    {
        std::strstream stream;
        stream.write(header_buffer, sizeof(MessageHeader));

        if (header_ptr->len > 0)
        {
            char* packet_data = (char*)(message.GetBuffer() + message.GetOffset());
            stream.write(packet_data, header_ptr->len);
        }

        int hr = 0;
        try
        {
            hr = send(sockfd, (char*)stream.str(), (int)stream.tellp(), 0);
        }
        catch (...)
        {
            hr = -1;
        }

        if (hr < 0)
        {
            this->CloseOrAbort(true);
            return false;
        }
    }

    return true;
}

IPEndPoint& SocketClient::GetRemoteEndPoint()
{
    return this->m_RemoteEP;
}

IPEndPoint SocketClient::GetLocalEndPoint()
{
    int sockfd = this->m_SocketHandle;
    if (!this->HandleIsInvalid(sockfd))
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (0 == getsockname(sockfd, (struct sockaddr*)&addr, &len))
        {
            return IPEndPoint(addr.sin_addr.s_addr, ntohs(addr.sin_port));
        }
    }
    return IPEndPoint();
}

int SocketClient::DoEvents()
{
    int events_count = 0;
    if (this->IsDisposed())
    {
        return events_count;
    }

    int sockfd = this->m_SocketHandle;
    if (this->m_State == SocketState_Connecting)
    {
        unsigned long long diffval = (SocketClient::GetTickCount() - this->m_LastCountTime);
        if (diffval >= (unsigned long long)this->m_ConnectTimeout)
        {
            this->CloseOrAbort(true);
        }
        else if (this->Poll(sockfd, 0, SelectMode_SelectWrite))
        {
            if (0 != this->GetSocketError(sockfd))
            {
                this->CloseOrAbort(true);
            }
            else
            {
                this->ProcessConnected();
            }
        }
    }
    else if (this->m_State == SocketState_Connected)
    {
        while (this->Poll(sockfd, 0, SelectMode_SelectRead)) // fetch recv signal
        {
            events_count += this->m_SocketReceiver.DoRecvBuffer();
            if (this->m_MaxHandleFrameNum > 0 && events_count >= this->m_MaxHandleFrameNum)
            {
                break;
            }
        }
        return events_count;
    }
    else if (this->m_State == SocketState_Disconnect)
    {
        unsigned long long diffval = (SocketClient::GetTickCount() - this->m_LastCountTime);
        if (this->m_ReConnectDelayTime <= 0)
        {
            return 0;
        }

        if (diffval >= (unsigned long long)this->m_ReConnectDelayTime)
        {
            this->m_State = SocketState_Default;
            int hr = this->Open();
            if (hr < 0)
            {
                this->CloseOrAbort(true);
            }
        }
    }
    return events_count;
}

void SocketClient::SetMaxHandleFrameNumber(int value)
{
    this->m_MaxHandleFrameNum = value;
}

int SocketClient::GetMaxHandleFrameNumber()
{
    return this->m_MaxHandleFrameNum;
}

void SocketClient::OnReConnect(EventArgs& e)
{

}

void SocketClient::OnMessage(SocketMessage& e)
{
    EventHandler<SocketMessage>* evt = this->m_OnMessage;
    if (evt != NULL)
    {
        evt->Invoke(this, &e);
    }
}

void SocketClient::OnOpen(EventArgs& e)
{
    EventHandler<EventArgs>* evt = this->m_OnOpen;
    if (evt != NULL)
    {
        evt->Invoke(this, &e);
    }
}

void SocketClient::OnError(EventArgs& e)
{
    EventHandler<EventArgs>* evt = this->m_OnError;
    if (evt != NULL)
    {
        evt->Invoke(this, &e);
    }
}

void SocketClient::OnClose(EventArgs& e)
{
    EventHandler<EventArgs>* evt = this->m_OnClose;
    if (evt != NULL)
    {
        evt->Invoke(this, &e);
    }
}

int SocketClient::ConnectSocket(int address, int port)
{
    if (this->IsDisposed())
    {
        return SocketClient::OPEN_THE_OBJECT_HAS_BEEN_DISPOSED;
    }

    if (this->m_State != SocketClient::SocketState_Default)
    {
        return SocketClient::OPEN_CURRENT_STATE_PREVENTS_AN_OPERATION_FROM_BEING_PERFORMED;
    }

    if (address == 0 || port == 0)
    {
        return SocketClient::OPEN_THE_NUMERIC_IP_ADDRESS_IS_INCORRECT;
    }

    int sockfd = (int)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->HandleIsInvalid(sockfd))
    {
        return SocketClient::OPEN_NEW_FD_HANDLES_CANNOT_BE_ALLOCATED_FROM_THE_SYSTEM_SOCKET_POOL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((unsigned short)port);
    server.sin_addr.s_addr = address;

    register int hr = 0;
    if (this->m_SlidingWindowSize > 0) // 设置滑块窗口缓冲区尺寸意味着无阻塞异步消息的投递；若不设定则意味着同步投递。
    {
#ifdef WIN32
        unsigned long flags = 1;
        ioctlsocket(sockfd, FIONBIO, &flags);
#else
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); // NONBLOCK
#endif
        int nSlidingWindowSize = this->m_SlidingWindowSize;
        if (nSlidingWindowSize < 525288)
        {
            nSlidingWindowSize = 525288;
        }

        hr = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&nSlidingWindowSize, sizeof(nSlidingWindowSize));
        if (0 != hr) // assert(0 == hr);
        {
            return SocketClient::OPEN_UNABLE_TO_SET_SOCKET_SLIDING_WINDOW_SIZE_THE_VALUE;
        }
    }
    this->m_SocketHandle = sockfd;
    this->m_LastCountTime = SocketClient::GetTickCount();

    int flag = this->IsNoDelay() ? 1 : 0;
    hr = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (hr < 0)
    {
        this->CloseOrAbort(true);
        return SocketClient::OPEN_UNABLE_TO_SET_SOCKET_FD_THE_NODELAY_SETTINGS;
    }

    hr = connect(sockfd, (struct sockaddr*)&server, sizeof(server));
#if !defined(WIN32)
    bool connecting = (errno == EINPROGRESS);
#else
    bool connecting = (WSAGetLastError() == WSAEWOULDBLOCK);
#endif
    if (hr == 0)
    {
        this->ProcessConnected();
        return SocketClient::OPEN_SOCKET_CLIENT_CONNECTION_ESTABLISHED;
    }
    else if (hr < 0 && connecting)
    {
        this->m_State = SocketState_Connecting;
        return SocketClient::OPEN_SOCKET_CLIENT_CONNECTION_ESTABLISHING;
    }
    else
    {
        this->CloseOrAbort(true);
        if (0 >= this->m_SlidingWindowSize && this->m_ReConnectDelayTime > 0)
        {
            return SocketClient::OPEN_TRY_TO_REESTABLISH_A_SOCKET_CONNECTION;
        }
        else
        {
            return SocketClient::OPEN_UNABLE_TO_CONNECT_AND_UNABLE_TO_RETRY_SOCKET_CONNECTION;
        }
    }
}

bool SocketClient::IsNoDelay() // TCP_NODELAY(Nagle)
{
    return true;
}

bool SocketClient::IsReduceSendMemoryFragmentation()
{
    return false;
}

unsigned long long SocketClient::GetTickCount(bool microseconds)
{
#ifdef WIN32
	static LARGE_INTEGER ticksPerSecond; // (unsigned long long)GetTickCount64();
	LARGE_INTEGER ticks;
	if (!ticksPerSecond.QuadPart)
	{
		QueryPerformanceFrequency(&ticksPerSecond);
	}

	QueryPerformanceCounter(&ticks);
	if (microseconds)
	{
		double cpufreq = (double)(ticksPerSecond.QuadPart / 1000000);
		unsigned long long nowtick = (unsigned long long)(ticks.QuadPart / cpufreq);
		return nowtick;
	}
	else
	{
		unsigned long long seconds = ticks.QuadPart / ticksPerSecond.QuadPart;
		unsigned long long milliseconds = 1000 * (ticks.QuadPart - (ticksPerSecond.QuadPart * seconds)) / ticksPerSecond.QuadPart;
		unsigned long long nowtick = seconds * 1000 + milliseconds;
		return (unsigned long long)nowtick;
	}
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);

	if (microseconds)
	{
		unsigned long long nowtick = (unsigned long long)tv.tv_sec * 1000000;
		nowtick += tv.tv_usec;
		return nowtick;
	}

	return ((unsigned long long)tv.tv_usec / 1000) + ((unsigned long long)tv.tv_sec * 1000);
#endif
}

IPAddress::IPAddress()
{
    this->m_AddressSize = 0;
    this->m_AddressFamily = AddressFamily_None;
}

IPAddress::IPAddress(int address)
{
    int& m_Addr = *(int*)this->m_Address;
    m_Addr = address;
    this->m_AddressSize = 4;
    this->m_AddressFamily = AddressFamily_InterNetwork;
}

IPAddress::IPAddress(const char* address)
{
    int& m_Addr = *(int*)this->m_Address;
    if (address == NULL)
    {
        m_Addr = 0;
        this->m_AddressSize = 0;
        this->m_AddressFamily = AddressFamily_None;
    }
    else
    {

        struct sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(0);

        int hr = inet_pton(AF_INET, address, &sockaddr.sin_addr);
        if (1 != hr)
        {
            m_Addr = 0;
            this->m_AddressSize = 0;
            this->m_AddressFamily = AddressFamily_None;
        }
        else
        {
            m_Addr = sockaddr.sin_addr.s_addr;
            this->m_AddressSize = 4;
            this->m_AddressFamily = AddressFamily_InterNetwork;
        }
    }
}

AddressFamily IPAddress::GetAddressFamily()
{
    return this->m_AddressFamily;
}

unsigned char* IPAddress::GetAddressBytes()
{
    return (unsigned char*)&this->m_Address;
}

int IPAddress::GetAddressBytesLength()
{
    return this->m_AddressSize;
}

long long int IPAddress::GetAddress()
{
    if (m_AddressFamily == AddressFamily_InterNetwork)
    {
        return *(unsigned int*)m_Address;
    }
    else if (m_AddressFamily == AddressFamily_None)
    {
        return 0;
    }
    return *(long long int*)m_Address;
}

std::string IPAddress::ToString()
{
    if (m_AddressFamily == AddressFamily_InterNetwork)
    {
        char buffer[100];
        sprintf(buffer, "%d.%d.%d.%d", (unsigned char)m_Address[0], (unsigned char)m_Address[1], (unsigned char)m_Address[2], (unsigned char)m_Address[3]);
        return buffer;
    }

    return "";
}

IPEndPoint::IPEndPoint()
{

}

IPEndPoint::IPEndPoint(const IPAddress & address, int port):
    m_Port(port),
    m_Address(address)
{

}

IPAddress& IPEndPoint::GetAddress()
{
    return this->m_Address;
}

int IPEndPoint::GetPort()
{
    return this->m_Port;
}

AddressFamily IPEndPoint::GetAddressFamily()
{
    return this->m_Address.GetAddressFamily();
}

SocketClient::MessageReceiver::MessageReceiver(SocketClient * socket_)
{
    this->m_Socket = socket_;
    if (this->IsFixedMode())
    {
        this->m_StreamPtr = new unsigned char[this->m_Socket->GetFixedSize()];
    }
    this->Reset();
}

SocketClient::MessageReceiver::~MessageReceiver()
{
    if (this->IsFixedMode())
    {
        unsigned char* stream_ptr = this->m_StreamPtr;
        if (stream_ptr != NULL)
        {
            delete stream_ptr;
        }
        this->m_StreamPtr = NULL;
    }
}

void SocketClient::MessageReceiver::Reset()
{
    this->m_TotalCount = 0;
    this->m_RecvOffset = 0;
    this->m_RecvedHeader = false;
    if (!this->IsFixedMode())
    {
        this->m_StreamPtr = NULL;
    }
}

int SocketClient::MessageReceiver::DoRecvBuffer()
{
    int events = 0;
    int s = this->m_Socket->DangerousGetHandle();
    int recvcount = sizeof(MessageHeader);
    if (this->m_RecvedHeader)
    {
        recvcount = (int)(this->m_TotalCount - this->m_RecvOffset);
        if (recvcount > MSS)
        {
            recvcount = MSS;
        }
    }
    int len = ::recv(s, this->m_Buffer, recvcount, 0);
    if (len < 0 || len == 0) // RST
    {
        this->m_Socket->CloseOrAbort(false);
    }
    else
    {
        if (!this->m_RecvedHeader)
        {
            MessageHeader* header = (MessageHeader*)this->m_Buffer; // 帧头不可能无法收足
            if (recvcount != sizeof(MessageHeader) || header->fk != HEADERKEY || header->len < 0)
            {
                this->m_Socket->CloseOrAbort(true);
            }
            else
            {
                this->m_RecvedHeader = true; // 收取到帧头
                this->m_RecvOffset = 0;
                this->m_TotalCount = header->len;

                if (!this->IsFixedMode())
                {
                    this->m_StreamPtr = NULL;
                }

                this->m_SaveHeader = *header;

				// 收到帧头的时候设一帧事件（提升网络事件处理积极性）
                events++;

                if (header->len > 0)
                {
                    if (!this->IsFixedMode())
                    {
                        this->m_StreamPtr = new unsigned char[header->len + 1];
                    }
                }
                else
                {
                    this->DoRecvMessage();
                }
            }
        }
        else
        {
            this->m_RecvOffset += len;
            int sp = (this->m_TotalCount - this->m_RecvOffset);
            if (sp < 0 || len > MSS) // 这玩意是不可能的
            {
                this->m_Socket->CloseOrAbort(true);
            }
            else
            {
                int offset = (this->m_RecvOffset - len);
                memcpy(this->m_StreamPtr + offset, this->m_Buffer, len);
                if (sp == 0) // 这表明这个包收取完毕了。
                {
					// 收取帧结束时设定一帧事件
                    events++; 
                    this->DoRecvMessage();
                }
            }
        }
    }
    return events;
}

void SocketClient::MessageReceiver::DoRecvMessage()
{
    unsigned char* stream_ptr = this->m_StreamPtr;
    SocketMessage message_inst;
    /// ***************************************** REACTOR HANDLE RECVED PACKET ****************************************************************
    SocketMessage* message_ptr = NULL;
    if (this->IsFixedMode())
    {
        message_ptr = &message_inst;
    }
    else
    {
        message_ptr = new SocketMessage;
    }
    if (this->m_SaveHeader.len > 0)
    {
        if (stream_ptr == NULL)
        {
            this->m_Socket->CloseOrAbort(true);
            return;
        }
        if (!this->IsFixedMode() || (this->m_SaveHeader.len + 1) < this->m_Socket->GetFixedSize())
        {
            stream_ptr[this->m_SaveHeader.len] = '\x00'; // 此无意义仅仅只是为了易于调试
        }
    }
    message_ptr->GetBuffer() = stream_ptr;
    message_ptr->GetCommandId() = this->m_SaveHeader.cmd;
    message_ptr->GetIdentifier() = this->m_SaveHeader.id;
    message_ptr->GetLength() = this->m_SaveHeader.len;
    message_ptr->GetSequenceNo() = this->m_SaveHeader.seq;
    message_ptr->GetClient() = this->m_Socket;

    this->m_Socket->OnMessage(*message_ptr);
    this->Reset();
}

bool SocketClient::MessageReceiver::IsFixedMode()
{
    return this->m_Socket->GetFixedSize() > 0;
}

SocketMessage::SocketMessage() :
    m_Socket(NULL),
    m_Buffer(NULL),
    m_Offest(0),
    m_Length(0),
    m_CommandId(0),
    m_Identifier(0),
    m_SequenceNo(0)
{

}

SocketMessage::~SocketMessage()
{
    this->Clear();
}

void SocketMessage::Clear()
{
    SocketClient* socket_ = this->m_Socket;
    if (socket_ != NULL)
    {
        if (!socket_->IsFixedMode())
        {
            unsigned char* buffer = this->m_Buffer;
            if (buffer != NULL)
            {
                delete[] buffer;
            }
        }
    }
    this->m_Buffer = NULL;
    this->m_Socket = NULL;
    this->m_Offest = 0;
    this->m_Length = 0;
    this->m_CommandId = 0;
    this->m_Identifier = 0;
    this->m_SequenceNo = 0;
}

bool SocketMessage::IsFixedMode()
{
    SocketClient* socket_ = this->m_Socket;
    if (socket_ == NULL)
    {
        return false;
    }
    return socket_->IsFixedMode();
}

long long SocketMessage::NewId()
{
    static long long autoIncrSeqNo = rand();
    while (0 == ++autoIncrSeqNo);
    return autoIncrSeqNo;
}

bool SocketHandler::ResponseWrite(SocketClient* socket, long long acksequence, unsigned char* buffer, int offset, int count)
{
    if (NULL == socket)
    {
        return false;
    }
    if (buffer == NULL && count != 0)
    {
        return false;
    }
    SocketMessage message;
    message.GetClient() = socket;
    message.GetCommandId() = GetCommandId();
    message.GetLength() = count;
    message.GetBuffer() = buffer;
    message.GetOffset() = offset;
    message.GetIdentifier() = 0;
    message.GetSequenceNo() = acksequence;

    return (*socket).Send(message);
}
