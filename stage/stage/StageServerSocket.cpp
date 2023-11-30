#include "StageSocket.h"
#include "StageClient.h"
#include "StageServerSocket.h"
#include "StageVirtualSocket.h"
#include "IStageSocketHandler.h"
#include "StageSocketListener.h"

namespace ep
{
    namespace stage
    {
        StageServerSocket::StageServerSocket(const std::shared_ptr<StageClient>& transmission,
            const std::string&                  platform,
            int                                 serverNo,
            Byte                                linkType,
            UInt32                              address,
            int                                 port)
            : StageVirtualSocket(transmission, linkType, address, port)
            , _platform(platform)
            , _serverNo(serverNo)
            , _linkType(linkType)
        {

        }

        StageServerSocket::~StageServerSocket()
        {
            if (this) {
                Dispose();
            }
        }

        Byte StageServerSocket::GetLinkType()
        {
            if (!this) {
                return 0;
            }
            return _linkType;
        }

        const std::string& StageServerSocket::GetPlatform()
        {
            return _platform;
        }

        int StageServerSocket::GetServerNo()
        {
            if (!this) {
                return 0;
            }
            return _serverNo;
        }

        std::shared_ptr<StageServerSocket> StageServerSocket::GetPointer()
        {
            auto po = GetPtr();
            if (!po) {
                return make_shared_null<StageServerSocket>();
            }
            return Object::as<StageServerSocket>(po);
        }

        bool StageServerSocket::Send(const Message& message, const SendAsyncCallback& callback)
        {
            if (!this) {
                return false;
            }

            std::shared_ptr<StageClient> transmission;
            if (!GetTransmissionObject(transmission)) {
                return false;
            }

            Transitroute transitroute;
            transitroute.LinkType = GetLinkType();
            transitroute.CommandId = message.CommandId;
            transitroute.SequenceNo = message.SequenceNo;
            transitroute.Platform = GetPlatform();
            transitroute.ServerNo = GetServerNo();
            transitroute.Payload = message.Payload;
            return transmission->Send(transitroute, callback);
        }

        void StageServerSocket::Dispose()
        {
            StageVirtualSocket::Dispose();
        }

        bool StageServerSocket::DestroyObject(std::shared_ptr<StageClient>& transmission)
        {
            if (!this) {
                return false;
            }
            if (!transmission) {
                return false;
            }
            Byte linkType = GetLinkType();
            int serverNo = GetServerNo();
            return transmission->AbortServerSocket(GetPlatform(), linkType, serverNo) ? true : false;
        }

        bool StageServerSocket::PostLastAbortIL()
        {
            return false;
        }

        void StageServerSocket::OnOpen(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageServerSocket> socket = GetPointer();
                transmission->OnOpen(socket, e);
            }
            ISocket::OnOpen(e);
        }

        void StageServerSocket::OnAbort(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageServerSocket> socket = GetPointer();
                transmission->OnAbort(socket, e);
            }
            ISocket::OnAbort(e);
        }

        void StageServerSocket::OnClose(EventArgs& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageServerSocket> socket = GetPointer();
                transmission->OnAbort(socket, e);
            }
            ISocket::OnClose(e);
        }

        void StageServerSocket::OnMessage(std::shared_ptr<Message>& e)
        {
            std::shared_ptr<StageClient> transmission = GetTransmissionObject();
            if (transmission) {
                std::shared_ptr<StageServerSocket> socket = GetPointer();
                transmission->OnMessage(socket, e);
            }
            ISocket::OnMessage(e);
        }
    }
}