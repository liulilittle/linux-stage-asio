#pragma once

namespace ep
{
    namespace stage
    {
        namespace model
        {
            enum Commands
            {
                Commands_LCriticalSI = 0x7e8d,      // 临界指令(L)
                Commands_AuthenticationAAA,         // 鉴权AAA
                Commands_Heartbeat,                 // 心跳
                Commands_AcceptClient,              // 接受客户端
                Commands_AbortClient,               // 中断客户端
                Commands_RoutingTransparent,        // 客户端交换路由
                Commands_Transitroute,              // 服务器交换路由
                Commands_AbortAllClient,            // 中断全部客户端(高级指令)
                Commands_PermittedAccept,           // 允许接受(高级指令)
                Commands_PreventAccept,             // 阻止接受(高级指令)
                Commands_RCriticalSI,               // 临界指令(R)
            };
        }
    }
}
