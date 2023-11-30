#pragma once

namespace ep
{
    namespace stage
    {
        namespace model
        {
            enum Commands
            {
                Commands_LCriticalSI = 0x7e8d,      // �ٽ�ָ��(L)
                Commands_AuthenticationAAA,         // ��ȨAAA
                Commands_Heartbeat,                 // ����
                Commands_AcceptClient,              // ���ܿͻ���
                Commands_AbortClient,               // �жϿͻ���
                Commands_RoutingTransparent,        // �ͻ��˽���·��
                Commands_Transitroute,              // ����������·��
                Commands_AbortAllClient,            // �ж�ȫ���ͻ���(�߼�ָ��)
                Commands_PermittedAccept,           // �������(�߼�ָ��)
                Commands_PreventAccept,             // ��ֹ����(�߼�ָ��)
                Commands_RCriticalSI,               // �ٽ�ָ��(R)
            };
        }
    }
}
