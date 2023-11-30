#pragma once

#include "util/stage/core/Object.h"
#include "util/stage/core/EventArgs.h"
#include "util/stage/core/BufferSegment.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/core/Random.h"
#include "util/stage/net/Socket.h"
#include "util/stage/threading/Timer.h"
#include "util/stage/threading/TimerScheduler.h"
#include "util/stage/stage/model/Inc.h"

namespace ep
{
    namespace stage
    {
        using ep::net::ISocket;
        using ep::net::Socket;
        using ep::net::Message;
        using ep::net::Transitroute;
        using ep::net::BulkManyMessage;
        using ep::net::TBulkManyMessage;
        using ep::threading::Timer;
        using ep::threading::TimerScheduler;
        using ep::stage::model::Commands;
        using ep::stage::model::Error;
        using ep::stage::model::AuthenticationAAARequest;
        using ep::stage::model::AuthenticationAAAResponse;

        class StageSocket;
        class StageSocketBuilder;
        class IStageSocketHandler;

        class StageSocketBuilder
        {
            friend StageSocket;

        private:
            mutable int                                                                         _svrAreaNo;
            mutable int                                                                         _serverNo;
            mutable int                                                                         _port;
            mutable Byte                                                                        _chLinkType;
            mutable Byte                                                                        _fk;
            mutable bool                                                                        _transparent;
            mutable UShort                                                                      _maxFD;
            mutable std::string                                                                 _platform;
            mutable std::string                                                                 _hostName;
            mutable std::shared_ptr<IStageSocketHandler>                                        _socketHandler;
            mutable std::shared_ptr<Hosting>                                                    _hosting;
            mutable bool                                                                        _onlyRoutingTransparent;
            mutable bool                                                                        _needToVerifyMaskOff;

        public:
            StageSocketBuilder();
            virtual ~StageSocketBuilder();

        public:
            virtual void                                                                        Clear();

        public:
            static const int                                                                    MIN_FILEDESCRIPTOR = 1;
            static const int                                                                    MAX_FILEDESCRIPTOR = 0xffff;

        public:
            virtual StageSocketBuilder&                                                         SetPlatform(const std::string& value) {
                _platform = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetHostName(const std::string& value) {
                _hostName = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetSvrAreaNo(int value) {
                _svrAreaNo = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetServerNo(int value) {
                _serverNo = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetPort(int value) {
                _port = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetLinkType(int value) {
                _chLinkType = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetSocketHandler(const std::shared_ptr<IStageSocketHandler>& value) {
                _socketHandler = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetHosting(const std::shared_ptr<Hosting>& value) {
                _hosting = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetKf(Byte value) {
                _fk = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetTransparent(bool value) {
                _transparent = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetOnlyRoutingTransparent(bool value) {
                _onlyRoutingTransparent = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetMaxFiledescriptor(int value) {
                if (value < MIN_FILEDESCRIPTOR) {
                    value = MIN_FILEDESCRIPTOR;
                }
                else if (value > MAX_FILEDESCRIPTOR) {
                    value = MAX_FILEDESCRIPTOR;
                }
                _maxFD = value;
                return *this;
            }
            virtual StageSocketBuilder&                                                         SetNeedToVerifyMaskOff(bool value) {
                _needToVerifyMaskOff = value;
                return *this;
            }

        public:
            virtual const std::string&                                                          GetPlatform() { return _platform; }
            virtual const std::string&                                                          GetHostName() { return _hostName; }
            virtual const Byte&                                                                 GetKf() { return _fk; }
            virtual const int&                                                                  GetPort() { return _port; }
            virtual const int&                                                                  GetSvrAreaNo() { return _svrAreaNo; }
            virtual const int&                                                                  GetServerNo() { return _serverNo; }
            virtual const Byte&                                                                 GetLinkType() { return _chLinkType; }
            virtual const bool&                                                                 IsTransparent() { return _transparent; }
            virtual const bool&                                                                 IsOnlyRoutingTransparent() { return _onlyRoutingTransparent; }
            virtual const std::shared_ptr<IStageSocketHandler>&                                 GetSocketHandler() { return _socketHandler; }
            virtual const std::shared_ptr<Hosting>&                                             GetHosting() { return _hosting; }
            virtual const UShort&                                                               GetMaxFiledescriptor() { return _maxFD; }
            virtual const bool&                                                                 IsNeedToVerifyMaskOff() { return _needToVerifyMaskOff; }
        };

        class StageSocket : public Socket
        {
            friend IStageSocketHandler;

        public:
            typedef HASHSETDEFINE(UShort)                                                       TransparentProtocol;
            typedef std::function<void(Error error)>                                            CompletedAuthentication;

        public:
            void*                                                                               Tag;
            UShort                                                                              LinkNo;
            Byte                                                                                LinkType;
            std::string                                                                         PlatformName;
            UShort                                                                              ServerNo;
            UShort                                                                              SvrAreaNo;

        public:
            StageSocket(
                const std::shared_ptr<Hosting>&                                                 hosting,
                const std::shared_ptr<IStageSocketHandler>&                                     handler,
                Byte                                                                            fk,
                std::shared_ptr<boost::asio::ip::tcp::socket>&                                  socket,
                bool                                                                            transparent, 
                bool                                                                            onlyRoutingTransparent = true);
            StageSocket(StageSocketBuilder&                                                     builder);

        public:
            virtual bool                                                                        Available();
            virtual Error                                                                       ProcessAuthentication(std::shared_ptr<AuthenticationAAARequest>& request, unsigned int ackNo, const CompletedAuthentication& completedAuthentication);
            virtual Error                                                                       ProcessHeartbeat(std::shared_ptr<Message>& message);
            virtual Socket&                                                                     Open();

        public:
            template<typename TPayloadObject>
            inline bool                                                                         Send2(TPayloadObject& message, Commands commands, UInt32 ackNo, const SendAsyncCallback& callback)
            {
                if (!this)
                {
                    return false;
                }

                Message packet;
                if (!Message::New(packet, (UShort)commands, ackNo, message)) {
                    return false;
                }

                return Socket::Send(packet, NULL);
            }
            virtual void                                                                        Dispose();
            virtual std::shared_ptr<IStageSocketHandler>                                        GetSocketHandler();
            virtual TransparentProtocol*                                                        GetTransparentProtocol();
            virtual StageSocketBuilder*                                                         GetBuilder();

        public:
            virtual bool                                                                        IsEnabledCompressedMessages();
            virtual bool                                                                        EnabledCompressedMessages();
            virtual bool                                                                        DisabledCompressedMessages();

        protected:
            virtual std::string                                                                 NewSslKey();
            virtual int                                                                         NewModVt();
            virtual int                                                                         NewSubtractCode();
            virtual bool                                                                        CompletelyAuthentication(Error error, std::shared_ptr<AuthenticationAAARequest>& request, unsigned int ackNo);
            virtual bool                                                                        InitiateHeartbeat(UInt64 ackNo);
            virtual bool                                                                        InitiateAuthentication();
            virtual bool                                                                        IsCriticalCommandId(Commands commandId);
            virtual bool                                                                        IsNeedToVerifyMaskOff();

        protected:
            virtual void                                                                        Open(bool servering, bool pullUpListen);
            virtual void                                                                        ProcessInput(std::shared_ptr<Message>& message);
            virtual bool                                                                        ProcessSynced();
            virtual bool                                                                        CompletelyAuthentication(std::shared_ptr<AuthenticationAAAResponse>& response);
            virtual std::shared_ptr<Message>                                                    DecryptMessage(const std::shared_ptr<Message>& message, bool compressed);
            virtual std::shared_ptr<unsigned char>                                              EncryptMessage(UShort commandId, const std::shared_ptr<unsigned char>& payload, int& counts, bool& compressed);

        protected:
            virtual int                                                                         GetAuthenticationWaitTime();
            virtual int                                                                         GetMaxInactiveTime();
            virtual int                                                                         GetTickAlwaysIntervalTime();
            virtual int                                                                         GetInitiateHeartbeatIntervalTime();
            virtual bool                                                                        ProcessTickAlways();

        protected:
            virtual void                                                                        OnOpen(EventArgs& e);
            virtual void                                                                        OnAbort(EventArgs& e);
            virtual void                                                                        OnClose(EventArgs& e);
            virtual void                                                                        OnMessage(std::shared_ptr<Message>& e);

        private:
            void                                                                                ForwardOpenInvoke();
            void                                                                                Initialize(
                const std::shared_ptr<Hosting>&                                                 hosting,
                const std::shared_ptr<IStageSocketHandler>&                                     handler,
                bool                                                                            transparent);

        private:
            struct
            {
                bool                                                                            _establish : 1;
                bool                                                                            _recvauthAAA : 1;
                bool                                                                            _openedSKT : 1;
                bool                                                                            _transparent : 1;
                bool                                                                            _compressedMessages : 1;
                bool                                                                            _servering : 3;
            };
            std::shared_ptr<Timer>                                                              _tickAlways;
            UInt64                                                                              _waitAuthAAAStartTime;
            UInt64                                                                              _lastActivityTime;
            UInt64                                                                              _lastInitiateHeartbeatTime;
            std::shared_ptr<IStageSocketHandler>                                                _socketHandler;
            std::string                                                                         _key;
            Byte                                                                                _subtract;
            Random                                                                              _randomer;
            Byte                                                                                _modVt;
            TransparentProtocol                                                                 _transparentProtocol;
            StageSocketBuilder                                                                  _builder;
        };
    }
}