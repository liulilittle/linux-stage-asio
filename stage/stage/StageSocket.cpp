#include "StageSocket.h"
#include "IStageSocketHandler.h"
#include "util/stage/cipher/RC4.h"
#include "util/stage/compression/compress.hpp"
#include "util/stage/compression/decompress.hpp"

namespace ep
{
    namespace stage
    {
        const int StageSocketBuilder::MAX_FILEDESCRIPTOR;
        const int StageSocketBuilder::MIN_FILEDESCRIPTOR;

        StageSocket::StageSocket(
            const std::shared_ptr<Hosting>&                     hosting,
            const std::shared_ptr<IStageSocketHandler>&         handler,
            Byte                                                fk,
            std::shared_ptr<boost::asio::ip::tcp::socket>&      socket,
            bool                                                transparent,
            bool                                                onlyRoutingTransparent)
            : Socket(hosting, fk, socket)
        {
            _builder.SetHosting(hosting);
            _builder.SetKf(fk);
            _builder.SetTransparent(transparent);
            _builder.SetSocketHandler(handler);
            _builder.SetOnlyRoutingTransparent(onlyRoutingTransparent);
            Initialize(hosting, handler, transparent);
        }

        StageSocket::StageSocket(StageSocketBuilder&            builder)
            : Socket(builder.GetHosting(), builder.GetKf(), builder.GetHostName(), builder.GetPort())
            , _builder(builder)
        {
            Initialize(builder.GetHosting(), builder.GetSocketHandler(), builder.IsTransparent());
            LinkNo = 0;
            LinkType = builder.GetLinkType();
        }

        void StageSocket::Initialize(const std::shared_ptr<Hosting>& hosting, const std::shared_ptr<IStageSocketHandler>& handler, bool transparent)
        {
            // 初始化私有变量
            _establish = (false);
            _recvauthAAA = (false);
            _openedSKT = (false);
            _transparent = (transparent);
            _servering = (false);
            _compressedMessages = (true);
            _waitAuthAAAStartTime = (0);
            _lastInitiateHeartbeatTime = (0);
            _socketHandler = (handler);
            _subtract = (0);
            _modVt = (0);
            _lastActivityTime = (Hosting::GetTickCount());

            //初始化公共变量
            Tag = (NULL);
            LinkNo = (0);
            LinkType = (0);
            PlatformName = ("");
            ServerNo = (0);
            SvrAreaNo = (0);

            // 添加默认的透传协议
            _transparentProtocol.insert(model::Commands_AuthenticationAAA);
            _transparentProtocol.insert(model::Commands_Heartbeat);
        }

        bool StageSocket::Available()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (IsDisposed()) {
                return false;
            }
            if (!_establish)  {
                return false;
            }
            return Socket::Available();
        }

        Error StageSocket::ProcessAuthentication(std::shared_ptr<AuthenticationAAARequest>& request, unsigned int ackNo, const CompletedAuthentication& completedAuthentication)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (IsDisposed()) {
                return model::Error_TheObjectHasBeenFreed;
            }
            if (!_socketHandler) {
                if (!completedAuthentication) {
                    return model::Error_Success;
                }
                completedAuthentication(model::Error_Success);
                return model::Error_PendingAuthentication;
            }
            std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
            return _socketHandler->ProcessAuthentication(socket, request, ackNo, completedAuthentication);
        }

        Error StageSocket::ProcessHeartbeat(std::shared_ptr<Message>& message)
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            if (IsDisposed()) {
                return model::Error_TheObjectHasBeenFreed;
            }
            if (!_socketHandler) {
                return model::Error_Success;
            }
            std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
            return _socketHandler->ProcessHeartbeat(socket, message);
        }

        void StageSocket::Dispose()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            _establish = false;
            if (_tickAlways) {
                _tickAlways->Stop();
                _tickAlways->Dispose();
                _tickAlways.reset();
            }
            if (_socketHandler) {
                _socketHandler.reset();
            }
            if (!_key.empty()) {
                _key = "";
            }
            if (!PlatformName.empty()) {
                PlatformName = "";
            }
            if (!_transparentProtocol.empty()) {
                _transparentProtocol.clear();
            }
            Socket::Dispose();
        }

        std::shared_ptr<IStageSocketHandler> StageSocket::GetSocketHandler()
        {
            std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
            return _socketHandler;
        }

        StageSocket::TransparentProtocol* StageSocket::GetTransparentProtocol()
        {
            return Object::addressof(_transparentProtocol);
        }

        StageSocketBuilder* StageSocket::GetBuilder() 
        {
            return Object::addressof(_builder);
        }

        std::string StageSocket::NewSslKey()
        {
            char sz[11];
            for (int i = 0; i < 10; i++) {
                int seed = _randomer.Next(1, 255);
                _randomer.SetSeed(seed);
                sz[i] = (char)seed;
            }
            sz[10] = '\x0';
            return sz;
        }

        int StageSocket::NewModVt()
        {
            int seed = _randomer.Next(1, 256);
            _randomer.SetSeed(seed);
            return seed;
        }

        int StageSocket::NewSubtractCode()
        {
            int seed = _randomer.Next(1, 255);
            _randomer.SetSeed(seed);
            return seed;
        }

        bool StageSocket::CompletelyAuthentication(Error error, std::shared_ptr<AuthenticationAAARequest>& request, unsigned int ackNo)
        {
            if (!request) {
                return false;
            }

            if (0 == _subtract) {
                _subtract = NewSubtractCode();
            }

            if (0 == _modVt) {
                _modVt = NewModVt();
            }

            if (_key.empty()) {
                _key = NewSslKey();
            }

            AuthenticationAAAResponse response;
            response.SetCode(error);
            response.SetKey(_key.data(), _key.size());
            response.SetSubtract(_subtract);
            response.SetModVt(_modVt);
            response.SetLinkNo(LinkNo);
            response.SetLinkType(request->GetLinkType());

            if (!Send2(response, model::Commands_AuthenticationAAA, ackNo, NULL)) {
                return false;
            }
            return error == model::Error_Success;
        }

        bool StageSocket::CompletelyAuthentication(std::shared_ptr<AuthenticationAAAResponse>& response)
        {
            if (!this || !response) {
                return false;
            }

            Error error = response->GetCode();
            if (error != model::Error_Success) {
                return false;
            }

            if (LinkType != response->GetLinkType()) {
                return false;
            }

            const char* sz = response->GetKey();
            int len = response->GetKeyLength();
            if (NULL == sz && 0 != len) {
                return false;
            }

            if (NULL == sz || len <= 0) {
                _key = "";
            }
            else {
                _key = std::string(sz, len);
            }
            _subtract = response->GetSubtract();
            _modVt = response->GetModVt();
            LinkNo = response->GetLinkNo();
            return true;
        }

        bool StageSocket::InitiateHeartbeat(UInt64 ackNo)
        {
            Message message;
            message.CommandId = model::Commands_Heartbeat;
            message.SequenceNo = ackNo;
            return Send(message, NULL);
        }

        bool StageSocket::IsCriticalCommandId(Commands commandId)
        {
            return commandId < model::Commands_LCriticalSI || commandId >= model::Commands_RCriticalSI;
        }

        bool StageSocket::IsNeedToVerifyMaskOff()
        {
            StageSocketBuilder* builder = GetBuilder();
            if (!builder) {
                return Socket::IsNeedToVerifyMaskOff();
            }
            return builder->IsNeedToVerifyMaskOff();
        }

        bool StageSocket::IsEnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            return _compressedMessages;
        }

        bool StageSocket::EnabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            _compressedMessages = true;
            return true;
        }

        bool StageSocket::DisabledCompressedMessages()
        {
            if (!this) {
                return false;
            }
            _compressedMessages = false;
            return true;
        }

        void StageSocket::Open(bool servering, bool pullUpListen)
        {
            int exception = 0;
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (IsDisposed()) {
                    exception = 1;
                    break;
                }

                if (_openedSKT) {
                    exception = 2;
                    break;
                }

                if (servering != IsServeringMode()) {
                    exception = 3;
                    break;
                }

                _openedSKT = true;
                _servering = servering;
                _waitAuthAAAStartTime = Hosting::GetTickCount();

                std::shared_ptr<StageSocket> self = Object::as<StageSocket>(GetPtr());
                if (!_tickAlways) {
                    _tickAlways = Timer::Create<Timer>();
                    if (!_tickAlways) {
                        exception = 4;
                        break;
                    }
                    _tickAlways->SetInterval(GetTickAlwaysIntervalTime());
                    _tickAlways->TickEvent = [self] (std::shared_ptr<Timer>& sender, Timer::TickEventArgs& e) {
                        bool closing = true;
                        do {
                            if (!self) {
                                break;
                            }

                            std::unique_lock<SyncObjectBlock> scoped(self->GetSynchronizingObject());
                            if (self->IsDisposed()) {
                                break;
                            }

                            closing = !self->ProcessTickAlways();
                        } while (0, 0);
                        if (closing) {
                            self->AbortINT3();
                            if (sender) {
                                sender->Stop();
                            }
                        }
                    };
                    _tickAlways->Start();
                }
                if (servering) {
                    if (_socketHandler && !_socketHandler->ProcessAccept(self)) {
                        AbortINT3();
                        break;
                    }
                    PullUpListen();
                }
                else {
                    Socket::Open(servering, pullUpListen);
                }
            } while (0, 0);
            switch (exception)
            {
            case 1:
                throw std::runtime_error("Managed and unmanaged resources held by the current socket object have been released");
                break;
            case 2:
                throw std::runtime_error("The state of the current object has been turned on to invalidate the current operation");
                break;
            case 3:
                throw std::runtime_error("Socket opening error the way the socket is opened does not match the socket type");
                break;
            case 4:
                throw std::runtime_error("The always timer object cannot be instantiated, usually out of memory");
                break;
            default:
                break;
            }
        }

        Socket& StageSocket::Open()
        {
            return Socket::Open();
        }

        void StageSocket::ProcessInput(std::shared_ptr<Message>& message)
        {
            int events = 0;
            do {
                if (!message) {
                    events = -1;
                    break;
                }

                std::shared_ptr<StageSocket> self = Object::as<StageSocket>(GetPtr());
                if (!self) {
                    events = -2;
                    break;
                }

                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());
                if (IsDisposed()) {
                    events = -3;
                    break;
                }

                _lastActivityTime = Hosting::GetTickCount();
                if (IsCriticalCommandId((Commands)message->CommandId)) {
                    events = -4;
                    break;
                }

                if (!_establish) {
                    if (message->CommandId != model::Commands_AuthenticationAAA) {
                        events = -1001;
                        break;
                    }

                    if (_recvauthAAA) {
                        events = -1002;
                        break;
                    }

                    UInt64 ackNo = message->SequenceNo;
                    if (0 != ackNo) {
                        events = -1003;
                        break;
                    }

                    events = 1;
                    _recvauthAAA = true;
                    _waitAuthAAAStartTime = Hosting::GetTickCount();
                    if (_servering) {
                        auto request = Message::Deserialize<AuthenticationAAARequest>(message.get());
                        if (!request) {
                            events = -1004;
                            break;
                        }

                        CompletedAuthentication completelyWarp = [self, request, ackNo](Error error) {
                            if (!self) {
                                return;
                            }
                            std::unique_lock<SyncObjectBlock> scope(self->GetSynchronizingObject());
                            if (self->IsDisposed()) {
                                self->AbortINT3();
                                return;
                            }
                            if (error == model::Error_PendingAuthentication) {
                                self->AbortINT3();
                                return;
                            }
                            auto requests = Object::constcast(request);
                            if (!self->CompletelyAuthentication(error, requests, ackNo)) {
                                self->AbortINT3();
                                return;
                            }
                            self->_establish = true;
                            self->_lastInitiateHeartbeatTime = Hosting::GetTickCount();
                            self->ForwardOpenInvoke();
                        };

                        StageSocketBuilder* builder = GetBuilder();
                        if (builder) {
                            builder->SetLinkType(request->GetLinkType());
                            builder->SetPlatform(request->GetPlatform());
                            builder->SetSvrAreaNo(request->GetSvrAreaNo());
                            builder->SetServerNo(request->GetServerNo());
                            builder->SetMaxFiledescriptor(request->GetMaxFD());
                        }

                        Error error = ProcessAuthentication(request, ackNo, completelyWarp);
                        if (error != model::Error_PendingAuthentication) {
                            completelyWarp(error);
                        }
                    }
                    else {
                        auto response = Message::Deserialize<AuthenticationAAAResponse>(message->Payload.get());
                        if (!response) {
                            events = -1005;
                            break;
                        }

                        std::unique_lock<SyncObjectBlock> scope(GetSynchronizingObject());
                        if (!CompletelyAuthentication(response)) {
                            events = -1006;
                            break;
                        }

                        _establish = true;
                        _lastInitiateHeartbeatTime = Hosting::GetTickCount();
                        ForwardOpenInvoke();
                    }
                }
                else {
                    if (message->CommandId == model::Commands_AuthenticationAAA) {
                        events = -2001;
                        break;
                    }

                    if (message->CommandId == model::Commands_Heartbeat) {
                        Error error = ProcessHeartbeat(message);
                        if (error == model::Error_Success) {
                            events = 2;
                            if (_servering) {
                                if (!InitiateHeartbeat(message->SequenceNo)) {
                                    events = -2002;
                                }
                            }
                        }
                        else {
                            events = -2003;
                        }
                        break;
                    }

                    StageSocketBuilder* builder = GetBuilder();
                    if (NULL != builder && !_servering) {
                        if (builder->IsOnlyRoutingTransparent()) {
                            if (message->CommandId != model::Commands_RoutingTransparent) {
                                events = -2004;
                                break;
                            }
                        }
                    }

                }
            } while (0, 0);
            if (events < 0) {
                AbortINT3();
            }
            else if (events == 0) {
                if (message) {
                    Socket::ProcessInput(message);
                }
            }
        }

        bool StageSocket::ProcessSynced()
        {
            if (!InitiateAuthentication()) {
                return false;
            }
            // 拉起当前套接字实例的侦听器（反应介于大于等于零时）
            return PullUpListen() >= 0; // TCP_SYN_RECVED
        }

        bool StageSocket::InitiateAuthentication()
        {
            StageSocketBuilder* builder = GetBuilder();
            if (!builder) {
                return false;
            }

            AuthenticationAAARequest request;
            do {
                auto& platform = builder->GetPlatform();
                request.SetPlatform(platform.data(), platform.size());
                request.SetLinkType(builder->GetLinkType());
                request.SetMaxFD(builder->GetMaxFiledescriptor());
                request.SetServerNo(builder->GetServerNo());
                request.SetSvrAreaNo(builder->GetSvrAreaNo());
            } while (0, 0);
            return Send2(request, model::Commands_AuthenticationAAA, 0, NULL);
        }

        std::shared_ptr<Message> StageSocket::DecryptMessage(const std::shared_ptr<Message>& message, bool compressed)
        {
            if (_transparent) {
                return message;
            }
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

                // 本协议是否为透传协议
                if (_transparentProtocol.find(message->CommandId) != _transparentProtocol.end()) {
                    return message;
                }
            } while (0, 0);
            // 对协议进行解密
            do {
                std::shared_ptr<BufferSegment> segment = message->Payload;
                if (!segment) {
                    break;
                }

                unsigned char* payload = segment->UnsafeAddrOfPinnedArray();
                int counts = segment->Length;
                if (counts < 0 || (counts != 0 && NULL == payload)) {
                    return make_shared_null<Message>();
                }

                cipher::rc4_crypt(_modVt, (unsigned char*)_key.data(), _key.length(), payload, counts, _subtract, 0);
            } while (0, 0);
            // 对协议进行解压(LZ77)
            do {
                if (!compressed) {
                    break;
                }

                std::shared_ptr<BufferSegment> segment = message->Payload;
                if (!segment) {
                    break;
                }

                unsigned char* payload = segment->UnsafeAddrOfPinnedArray();
                int counts = segment->Length;
                if (counts < 0 || (counts != 0 && NULL == payload)) {
                    return make_shared_null<Message>();
                }

                std::shared_ptr<std::string> output = make_shared_object<std::string>();
                if (!output) {
                    return make_shared_null<Message>();
                }

                gzip::Decompressor decompressor;
                try {
                    decompressor.decompress(*output, (char*)payload, counts);
                }
                catch (std::exception&) {
                    return make_shared_null<Message>();
                }

                if (output->empty()) { // 无法使用赫夫曼LZ77算法进行解压时
                    return make_shared_null<Message>();
                }

                unsigned char* sz = (unsigned char*)output->data();
                std::shared_ptr<unsigned char> contents = std::shared_ptr<unsigned char>(sz, [output](unsigned char* p) {
                    if (output) {
                        Object::constcast(output).reset();
                    }
                });

                if (!contents) {
                    return make_shared_null<Message>();
                }

                segment->Buffer = contents;
                segment->Offset = 0;
                segment->Length = output->size();
            } while (0, 0);
            return message;
        }

        std::shared_ptr<unsigned char> StageSocket::EncryptMessage(UShort commandId, const std::shared_ptr<unsigned char>& payload, int& counts, bool& compressed)
        {
            compressed = false;
            if (_transparent) {
                return payload;
            }
            do {
                std::unique_lock<SyncObjectBlock> scoped(GetSynchronizingObject());

                // 本协议是否为透传协议
                if (_transparentProtocol.find(commandId) != _transparentProtocol.end()) {
                    return payload;
                }
            } while (0, 0);
            // 对协议进行压缩(LZ77)
            std::shared_ptr<unsigned char> result = payload;
            do {
                if (!IsEnabledCompressedMessages()) {
                    break;
                }

                if (0 == counts) {
                    break;
                }

                if (counts < 0 || (counts != 0 && NULL == result)) {
                    counts = ~0;
                    return make_shared_null<unsigned char>();
                }

                std::shared_ptr<std::string> output = make_shared_object<std::string>();
                if (!output) {
                    counts = ~0;
                    return make_shared_null<unsigned char>();
                }

                gzip::Compressor encompressor;
                try {
                    encompressor.compress(*output, (char*)result.get(), counts);
                }
                catch (std::exception&) {
                    counts = ~0;
                    return make_shared_null<unsigned char>();
                }

                if (output->empty()) { // 无法使用赫夫曼LZ77算法进行压缩时
                    counts = ~0;
                    return make_shared_null<unsigned char>();
                }

                unsigned char* sz = (unsigned char*)output->data();
                std::shared_ptr<unsigned char> contents = std::shared_ptr<unsigned char>(sz, [output](unsigned char* p) {
                    std::shared_ptr<std::string>& outputs = Object::constcast(output);
                    if (outputs) {
                        outputs = make_shared_null<std::string>();
                    }
                });

                if (!contents) {
                    counts = ~0;
                    return make_shared_null<unsigned char>();
                }

                unsigned long plsz = output->size();
                if ((Int64)plsz < (Int64)counts) { // 设压缩流以后字节数量小于压缩前的大小则视未压缩
                    compressed = true;
                    counts = plsz;
                    result = contents;
                }
            } while (0, 0);
            // 对协议进行加密
            do {
                if (!result) {
                    break;
                }
                if (!compressed) { // 当前流未被压缩的时候则动态分配一块新的流存并且把原先的流复制进来
                    if (counts > 0) {
                        std::shared_ptr<unsigned char> shadow = make_shared_array<unsigned char>(counts);
                        if (!shadow) {
                            break;
                        }
                        else {
                            memcpy(shadow.get(), result.get(), counts); // 复制结果引用的二进制流块到影子流数据块
                        }
                        result = shadow;
                    }
                }
                cipher::rc4_crypt(_modVt, (unsigned char*)_key.data(), _key.length(), result.get(), counts, _subtract, 1);
            } while (0, 0);
            return result;
        }

        int StageSocket::GetAuthenticationWaitTime()
        {
            return 5000;
        }

        int StageSocket::GetMaxInactiveTime()
        {
            return 30000;
        }

        int StageSocket::GetTickAlwaysIntervalTime()
        {
            return 500;
        }

        int StageSocket::GetInitiateHeartbeatIntervalTime() {
            return 10000;
        }

        bool StageSocket::ProcessTickAlways()
        {
            UInt64 ullCurrentTime = Hosting::GetTickCount();
            if (!_establish) {
                if (_waitAuthAAAStartTime > ullCurrentTime || (ullCurrentTime - _waitAuthAAAStartTime) >= (UInt64)GetAuthenticationWaitTime()) {
                    _waitAuthAAAStartTime = ullCurrentTime;
                    return false;
                }
            }
            else {
                if (_lastActivityTime > ullCurrentTime || (ullCurrentTime - _lastActivityTime) >= (UInt64)GetMaxInactiveTime()) {
                    _lastActivityTime = ullCurrentTime;
                    return false;
                }

                if (!_servering) { // 在客户端模式工作的情况下则定期向服务器发起心跳任务
                    if (_lastInitiateHeartbeatTime > ullCurrentTime || (ullCurrentTime - _lastInitiateHeartbeatTime) >= (UInt64)GetInitiateHeartbeatIntervalTime()) {
                        _lastInitiateHeartbeatTime = ullCurrentTime;

                        UInt64 ackNo = Message::NewId();
                        if (!InitiateHeartbeat(ackNo)) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        void StageSocket::OnOpen(EventArgs& e)
        {
            if (_socketHandler) {
                std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
                if (!_socketHandler->ProcessOpen(socket)) {
                    AbortINT3();
                    return;
                }
            }
            Socket::OnOpen(e);
        }

        void StageSocket::OnAbort(EventArgs& e)
        {
            if (_socketHandler) {
                std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
                _socketHandler->ProcessAbort(socket, true);
            }
            Socket::OnAbort(e);
        }

        void StageSocket::OnClose(EventArgs& e)
        {
            if (_socketHandler) {
                std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
                _socketHandler->ProcessAbort(socket, false);
            }
            Socket::OnClose(e);
        }

        void StageSocket::OnMessage(std::shared_ptr<Message>& e)
        {
            if (_socketHandler) {
                std::shared_ptr<StageSocket> socket = Object::as<StageSocket>(GetPtr());
                if (!_socketHandler->ProcessMessage(socket, e)) {
                    AbortINT3();
                    return;
                }
            }
            Socket::OnMessage(e);
        }

        void StageSocket::ForwardOpenInvoke()
        {
            if (_servering) {
                Socket::Open(_servering, false);
            }
            else {
                OnOpen(EventArgs::Empty);
            }
        }

        StageSocketBuilder::StageSocketBuilder()
        {
            Clear();
        }

        StageSocketBuilder::~StageSocketBuilder()
        {
            Clear();
        }

        void StageSocketBuilder::Clear()
        {
            _svrAreaNo = (0);
            _serverNo = (0);
            _port = (0);
            _chLinkType = (0);
            _fk = (Socket::ServerKf);
            _transparent = (true);
            _maxFD = (MIN_FILEDESCRIPTOR);
            _platform = ("");
            _hostName = ("");
            _onlyRoutingTransparent = (true);
            _needToVerifyMaskOff = (false);
            if (_socketHandler) {
                _socketHandler.reset();
            }
            if (_hosting) {
                _hosting.reset();
            }
        }
    }
}