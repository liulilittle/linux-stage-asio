#pragma once

#include "util/Util.h"
#include "util/Singleton.h"
#include "util/busmsghandler/MsgIdTraiter.h"
#include "util/busmsghandler/BusMsgDispatcher.h"
#include "util/busmsghandler/BusMsgResponser.h"
#include "util/netmsgparser/TDRCPPParser.h"
#include "protocol/srcgen/protocol_ss.h"
#include "redprotocol/srcgen/proto/protopkg.pb.h"

#include <mutex>

namespace ep
{
    class Hosting;
    class BufferSegment;

    namespace net
    {
        class Message;
    }

    namespace threading
    {
        class Stranding;
    }

    namespace stage
    {
        class StageClient;
    }
}

using namespace PkgSS;

class ServerExchanger // 服务器交换机
{
public:
    typedef std::recursive_mutex                            SynchronizingObject;
    struct VirtualBus
    {
    public:
        std::string                                         Platform;
        int                                                 ServerNo;
        unsigned char                                       LinkType;
        int                                                 BusAddr;

    public:
        VirtualBus();
        void                                                Clear();
    };
    enum Error
    {
        Error_Success = TdrError::TDR_NO_ERROR,
        Error_CantNotBeSspkgBinaryPacked,
        Error_UnableToDynamicAllocateBufferSegment,
        Error_UnableToAllocateBinaryBuffer,
        Error_UnableToSendTransitrouteMessage,
        Error_UnableToGetVirtualBusObject,
        Error_ChannelHasBeenAbandonedToHoldOrDisposed,
        Error_CouldNotGetLocalhostObject,
        Error_UnableToDynamicAllocateTransitroute,
    };
    typedef int(*Handler)(const SSPKG& ssPkg);
    typedef HASHMAPDEFINE(int, Handler)                     HandlerTable;
    typedef HASHMAPDEFINE(unsigned int, VirtualBus)         VirtualBusTable;
    typedef HASHMAPDEFINE(std::string, unsigned int)        VirtualBusAddressTable;

public:
    ServerExchanger(const std::shared_ptr<ep::stage::StageClient>& channel); // 虚拟服务器与虚拟BUS交换机适配器
    virtual ~ServerExchanger();

public:
    virtual std::shared_ptr<ep::threading::Stranding>       GetStranding();
    virtual std::shared_ptr<ep::stage::StageClient>         GetChannel();
    virtual SynchronizingObject&                            GetSynchronizingObject();
    virtual int                                             DoEvents(int concurrent);

protected:
    virtual int                                             ProcessInput(std::shared_ptr<SSPKG> packet);

protected:
    virtual bool                                            AdviseEventSink(const std::shared_ptr<ep::stage::StageClient>& channel, const std::shared_ptr<ep::threading::Stranding>& stranding);

public:
    // 可允许外部利用接口显示创建虚拟的服务器虚套接字（而无需确保目的服务器必须在线，但这需求本地适配器与STAGE虚拟交换机都需启用“Sendback”机制）
    virtual bool                                            RemoveVirtualBus(unsigned int buskey, VirtualBus& vbus);
    virtual bool                                            RemoveVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus);
    virtual unsigned int                                    FindVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus);
    virtual unsigned int                                    AddVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus);

private:
    unsigned int                                            UnknowOpByVirtualBus(const std::string& platform, unsigned char linkType, int serverNo, VirtualBus* bus, int mode);
    int                                                     SendJit(SSPKG& pkg, int cmd, unsigned int busAddr, unsigned char linkType);

public:
    virtual void                                            Finalize();
    virtual bool                                            RegisterFunc(int cmd, const Handler& handler);
    virtual Handler                                         UnregisterFunc(int cmd);
    virtual bool                                            GetVirtualBus(unsigned int buskey, VirtualBus& vbus);

public:
    virtual int                                             Send(SSPKG& pkg, int cmd, unsigned int busAddr);
    virtual int                                             SendByLinkType(SSPKG& pkg, int cmd, unsigned char linkType);

private:
    HandlerTable                                            m_HandlerTable;
    std::shared_ptr<ep::threading::Stranding>               m_Stranding;
    std::shared_ptr<ep::stage::StageClient>                 m_StageClient;
    VirtualBusTable                                         m_VirtualBusTable;
    VirtualBusAddressTable                                  m_VirtualBusAddressTable;
    SynchronizingObject                                     m_Syncobj;
};