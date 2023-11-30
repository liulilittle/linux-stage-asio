#include "ServerExchanger.h"
#include "util/stage/core/Hosting.h"
#include "util/stage/net/Socket.h"
#include "util/stage/net/SocketListener.h"
#include "util/stage/threading/Stranding.h"
#include "util/stage/threading/TimerScheduler.h"
#include "util/stage/stage/StageSocket.h"
#include "util/stage/stage/StageClient.h"
#include "util/stage/stage/StageServerSocket.h"
#include "util/stage/stage/StageVirtualSocket.h"
#include "util/stage/stage/StageSocketListener.h"
#include "util/stage/stage/StageSocketCommunication.h"

bool ServerExchanger::RegisterFunc(int cmd, const Handler& handler)
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);
    if (NULL == handler) {
        return false;
    }

    m_HandlerTable[cmd] = handler;
    return true;
}

ServerExchanger::Handler ServerExchanger::UnregisterFunc(int cmd)
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);

    HandlerTable::iterator il = m_HandlerTable.find(cmd);
    if (il == m_HandlerTable.end()) {
        return NULL;
    }
    
    Handler handler = il->second;
    m_HandlerTable.erase(il);
    return handler;
}

int ServerExchanger::SendJit(SSPKG& pkg, int cmd, unsigned int busAddr, unsigned char linkType)
{
    typedef ServerExchanger Error;

    VirtualBus bus;
    if (0 != busAddr) {
        if (!GetVirtualBus(busAddr, bus)) {
            return Error::Error_UnableToGetVirtualBusObject;
        }
    }

    std::shared_ptr<ep::stage::StageClient> channel = m_StageClient;
    if (!channel || channel->IsDisposed()) {
        return Error::Error_ChannelHasBeenAbandonedToHoldOrDisposed;
    }

    std::shared_ptr<ep::Hosting> hosting = channel->GetHosting();
    if (!hosting) {
        return Error::Error_CouldNotGetLocalhostObject;
    }

    size_t size = 0;
    unsigned char packet[sizeof(pkg)];

    pkg.stHead.bVersion = MsgIdTraiter<SSPKG>::ID;
    pkg.stHead.dwCmd = cmd;
    pkg.stHead.dwBusid = busAddr;

    int error = pkg.pack((char*)packet, sizeof(packet), &size);
    if (error != Error::Error_Success) {
        return error;
    }

    if (0 == size) {
        return Error::Error_CantNotBeSspkgBinaryPacked;
    }

    auto payload = ep::make_shared_object<ep::BufferSegment>(ep::make_shared_array<unsigned char>(size), 0, size);
    if (!payload) {
        return Error::Error_UnableToDynamicAllocateBufferSegment;
    }

    if (!payload->Buffer) {
        return Error::Error_UnableToAllocateBinaryBuffer;
    }
    else {
        memcpy(payload->UnsafeAddrOfPinnedArray(), packet, size);
    }

    std::shared_ptr<ep::net::Transitroute> transitroute = ep::make_shared_object<ep::net::Transitroute>();
    if (!transitroute) {
        return Error::Error_UnableToDynamicAllocateTransitroute;
    }

    transitroute->Payload = payload;
    transitroute->CommandId = cmd;
    transitroute->SequenceNo = ep::net::Message::NewId();

    if (0 != busAddr) {
        transitroute->ServerNo = bus.ServerNo;
        transitroute->LinkType = bus.LinkType;
        transitroute->Platform = bus.Platform;
    }
    else {
        transitroute->LinkType = linkType;
        transitroute->ServerNo = channel->GetServerNo();
        transitroute->Platform = channel->GetPlatform();
    }

    boost::asio::io_service& context = hosting->GetContextObject();
    context.post([channel, transitroute] {
        channel->Send(*transitroute, NULL);
    });
    return Error::Error_Success;
}

bool ServerExchanger::GetVirtualBus(unsigned int buskey, VirtualBus& vbus)
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);

    VirtualBusTable::iterator il = m_VirtualBusTable.find(buskey);
    VirtualBusTable::iterator el = m_VirtualBusTable.end();
    if (il == el) {
        return false;
    }

    vbus = il->second;
    return true;
}

ServerExchanger::~ServerExchanger()
{

}

ServerExchanger::ServerExchanger(const std::shared_ptr<ep::stage::StageClient>& channel)
    : m_StageClient(channel)
{
    if (!channel) {
        throw std::runtime_error("The provided channel is not allowed to be a Null reference");
    }

    std::shared_ptr<ep::threading::Stranding> stranding = ep::make_shared_object<ep::threading::Stranding>();
    if (!stranding) {
        throw std::runtime_error("Unable to create belong to ServerExchanger stranding object");
    }
    else {
        m_Stranding = stranding;
    }

    AdviseEventSink(channel, stranding); // ¹ÒÔÚÊÂ¼þÕìÌýÆ÷
}

void ServerExchanger::Finalize()
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);

    std::shared_ptr<ep::threading::Stranding> stranding = m_Stranding;
    if (stranding) {
        stranding->Dispose();
    }

    m_Stranding.reset();
    m_StageClient.reset();
}

bool ServerExchanger::AdviseEventSink(const std::shared_ptr<ep::stage::StageClient>& channel, const std::shared_ptr<ep::threading::Stranding>& stranding)
{
    if (!channel) {
        return false;
    }

    if (!stranding) {
        return false;
    }
    
    channel->ServerSocketOpenEvent = [this, stranding](std::shared_ptr<ep::stage::StageServerSocket>& sender, ep::EventArgs& e) {
        if (!sender) {
            return;
        }

        stranding->Post([this, sender] { // ·ÖÅäÐéÄâBUSµØÖ·
            if (sender->IsDisposed()) {
                return false;
            }

            VirtualBus bus;
            return 0 != AddVirtualBus(sender->GetPlatform(), sender->GetLinkType(), sender->GetServerNo(), &bus);
        });
    };
    channel->ServerSocketAbortEvent = [this, stranding](std::shared_ptr<ep::stage::StageServerSocket>& sender, ep::EventArgs& e) {
        if (!sender) {
            return;
        }

        stranding->Post([this, sender] {
            if (sender->IsDisposed()) {
                return false;
            }

            VirtualBus bus;
            return RemoveVirtualBus(sender->GetPlatform(), sender->GetLinkType(), sender->GetServerNo(), &bus);
        });
    };
    channel->ServerSocketMessageEvent = [this, stranding](std::shared_ptr<ep::stage::StageServerSocket>& sender, std::shared_ptr<ep::net::Message>& e) {
        if (!sender) {
            return;
        }

        if (!e) {
            return;
        }

        std::shared_ptr<ep::BufferSegment> payload = e->Payload;
        if (!payload) {
            return;
        }
        else {
            if (payload->Offset < 0 || payload->Length <= 0) {
                return;
            }

            unsigned char* buf = payload->UnsafeAddrOfPinnedArray();
            if (NULL == buf) {
                return;
            }
        }

        if (sender->IsDisposed()) {
            return;
        }

        std::shared_ptr<SSPKG> packet = ep::make_shared_object<SSPKG>();
        if (!packet) {
            return;
        }

        int error = packet->unpack((char*)payload->UnsafeAddrOfPinnedArray(), payload->Length);
        if (TdrError::TDR_NO_ERROR != error) {
            return;
        }

        stranding->Post([this, sender, packet] {
            if (sender->IsDisposed()) {
                return false;
            }

            unsigned int busAddr = FindVirtualBus(sender->GetPlatform(), sender->GetLinkType(), sender->GetServerNo(), NULL);
            if (0 == busAddr) {
                return false;
            }
            else {
                packet->stHead.dwBusid = busAddr;
            }

            return ProcessInput(packet) >= 0;
        });
    };
    return true;
}

unsigned int ServerExchanger::UnknowOpByVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus, int op)
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);

    char sz[65535];
    sprintf(sz, "%s.%d.%d", platform.data(), linkType, serverNo);

    VirtualBusAddressTable::iterator il = m_VirtualBusAddressTable.find(sz);
    VirtualBusAddressTable::iterator el = m_VirtualBusAddressTable.end();

    unsigned int key = 0;
    if (il != el) {
        key = il->second;
        if (op == 1) { // É¾³ý
            VirtualBusTable::iterator il1 = m_VirtualBusTable.find(key);
            if (il1 != m_VirtualBusTable.end()) {
                if (NULL != bus) {
                    *bus = il1->second;
                }
                m_VirtualBusTable.erase(il1);
            }
            m_VirtualBusAddressTable.erase(il);
        }
        return key;
    }
    if (op != 2) { // ²éÑ¯
        return key;
    }
    else {
        VirtualBusTable::iterator il1 = m_VirtualBusTable.end();
        VirtualBusTable::iterator el1 = m_VirtualBusTable.end();
        do {
            key = ep::net::Message::NewId();
            if (key) {
                il1 = m_VirtualBusTable.find(key);
                if (il1 == el1) {
                    break;
                }
            }
            key = 0;
        } while (0 == key);
    }
    VirtualBus& vbus = m_VirtualBusTable[key];
    {
        vbus.BusAddr = key;
        vbus.LinkType = linkType;
        vbus.Platform = platform;
        vbus.ServerNo = serverNo;
    }
    m_VirtualBusAddressTable[sz] = key;
    if (NULL != bus) {
        *bus = vbus;
    }
    return key;
}

bool ServerExchanger::RemoveVirtualBus(unsigned int buskey, VirtualBus& vbus)
{
    std::unique_lock<SynchronizingObject> scope(m_Syncobj);

    VirtualBusTable::iterator il = m_VirtualBusTable.find(buskey);
    VirtualBusTable::iterator el = m_VirtualBusTable.end();
    if (il == el) {
        return false;
    }

    vbus = il->second;
    m_VirtualBusTable.erase(il);

    char sz[65535];
    sprintf(sz, "%s.%d.%d", vbus.Platform.data(), vbus.LinkType, vbus.ServerNo);

    VirtualBusAddressTable::iterator il1 = m_VirtualBusAddressTable.find(sz);
    if (il1 != m_VirtualBusAddressTable.end()) {
        m_VirtualBusAddressTable.erase(il1);
    }

    return true;
}

bool ServerExchanger::RemoveVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus)
{
    return 0 != UnknowOpByVirtualBus(platform, linkType, serverNo, bus, 1);
}

unsigned int ServerExchanger::FindVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus)
{
    return UnknowOpByVirtualBus(platform, linkType, serverNo, bus, 0);
}

unsigned int ServerExchanger::AddVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus)
{
    return UnknowOpByVirtualBus(platform, linkType, serverNo, bus, 2);
}

int ServerExchanger::ProcessInput(std::shared_ptr<SSPKG> packet)
{
    if (!packet) {
        return ~0;
    }

    HandlerTable::iterator il = m_HandlerTable.find(packet->stHead.dwCmd);
    HandlerTable::iterator el = m_HandlerTable.end();
    if (il == el) {
        return ~1;
    }

    Handler handler = il->second;
    if (NULL == handler) {
        return ~2;
    }

    return handler(*packet);
}

std::shared_ptr<ep::threading::Stranding> ServerExchanger::GetStranding()
{
    return m_Stranding;
}

std::shared_ptr<ep::stage::StageClient> ServerExchanger::GetChannel()
{
    return m_StageClient;
}

ServerExchanger::SynchronizingObject& ServerExchanger::GetSynchronizingObject()
{
    return m_Syncobj;
}

int ServerExchanger::DoEvents(int concurrent)
{
    std::shared_ptr<ep::threading::Stranding> stranding = m_Stranding;
    if (!stranding) {
        return 0;
    }
    return stranding->DoEvents(concurrent);
}

int ServerExchanger::SendByLinkType(SSPKG& pkg, int cmd, unsigned char linkType)
{
    std::shared_ptr<ep::stage::StageClient> channel = m_StageClient;
    if (!channel) {
        return Error::Error_ChannelHasBeenAbandonedToHoldOrDisposed;
    }

    unsigned int busAddr = FindVirtualBus(channel->GetPlatform(), linkType, channel->GetServerNo(), NULL);
    if (0 != busAddr) {
        return Send(pkg, cmd, busAddr);
    }

    return SendJit(pkg, cmd, 0, linkType);
}

int ServerExchanger::Send(SSPKG& pkg, int cmd, unsigned int busAddr)
{
    return SendJit(pkg, cmd, busAddr, 0);
}

void ServerExchanger::VirtualBus::Clear()
{
    Platform = "";
    ServerNo = 0;
    LinkType = 0;
    BusAddr = 0;
}

ServerExchanger::VirtualBus::VirtualBus()
{
    Clear();
}
