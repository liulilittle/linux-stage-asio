#pragma once

#include <strstream>
#ifdef WIN32
#include <hash_map>
#include <hash_set>
#else

#include "util/Util.h"
#endif

#include "../net/XiYouSdkClient.h"

class XiYouSdkClient;
class XiYouSdkInvokeEventArgs;
class SocketMessage;

template<class TEventArgs>
class EventHandler;

enum XiYouSdkCommands;

enum ApiInvokeError
{
    ApiInvoke_kError,
    ApiInvoke_kSuccess,
    ApiInvoke_kTimeout
};

template<typename TResult>
class ApiInvokeResultEventArgs : EventArgs
{
private:
    XiYouSdkInvokeEventArgs *   m_EventArgs;
    TResult*                    m_Result;
    ApiInvokeError              m_Error;

public:
    ApiInvokeResultEventArgs() :
        m_EventArgs(NULL),
        m_Result(NULL),
        m_Error(ApiInvoke_kError)
    {
    }

public:
    XiYouSdkInvokeEventArgs*&   GetEventArgs() { return this->m_EventArgs; }
    TResult*&                   GetResult() { return this->m_Result; }
    ApiInvokeError&             GetError() { return this->m_Error; }
};

class ApiController
{
private:
    typedef struct 
    {
        EventHandler<ApiInvokeResultEventArgs<void*>/**/>           delegate_;
    } DelegateInfo;
#ifdef WIN32
    typedef stdext::hash_map<unsigned short, DelegateInfo>          DelegateContainer;
#else
    typedef HASHMAPDEFINE(unsigned short, DelegateInfo)             DelegateContainer;
#endif

private:
    XiYouSdkClient&             m_sdkClient;
    DelegateContainer           m_delegates;

public:
    ApiController(XiYouSdkClient& sdkClient)
        : m_sdkClient(sdkClient)
    {
    }
    virtual ~ApiController()
    {
        m_delegates.clear();
    }

protected:
    template<typename TRequest, typename TResult>
    bool InvokeAsync(XiYouSdkCommands command, TRequest& request, EventHandler<ApiInvokeResultEventArgs<TResult>/**/>* handler)
    {
        std::strstream stream;
        request.Serialize(stream);

        DelegateContainer::iterator it = m_delegates.find(command);

        EventHandler<XiYouSdkInvokeEventArgs>* callback = NULL;
        DelegateInfo* info = NULL;

        do
        {
            if (it == m_delegates.end())
            {
                DelegateInfo di;

                m_delegates.insert(DelegateContainer::value_type(command, di));
                it = m_delegates.find(command);
            }

            if (it == m_delegates.end())
            {
                return false;
            }
        } while (false);

        info = static_cast<DelegateInfo*>(reinterpret_cast<void*>(&reinterpret_cast<char&>(it->second)));
        callback = (EventHandler<XiYouSdkInvokeEventArgs>*)&info->delegate_;

        if (callback->GetMethod() == NULL)
        {
            EventHandler<XiYouSdkInvokeEventArgs>* delegate_ = (EventHandler<XiYouSdkInvokeEventArgs>*)&info->delegate_;
            delegate_->Create(delegate_, this, &ApiController::EventInvokeCallback<TResult>);
        }
        return m_sdkClient.InvokeAsync(command, (char*)stream.str(), (int)stream.tellp(), callback, handler);
    };

private:
    template<typename TResult>
    static void EventInvokeCallback(ApiController* target, void* obj, XiYouSdkInvokeEventArgs* e)
    {
        ApiInvokeError error = ApiInvoke_kError;
        TResult model;
        if (e->GetError() == XiYouSdkInvokeEventArgs::XiYouSdkInvoke_Success)
        {
            SocketMessage* message = e->GetMessage();
            if (NULL != message && SocketMessage::Deserialize(model, *message))
            {
                error = ApiInvoke_kSuccess;
            }
        }
        else if (e->GetError() == XiYouSdkInvokeEventArgs::XiYouSdkInvoke_Timeout)
        {
            error = ApiInvoke_kTimeout;
        }
        EventHandler<ApiInvokeResultEventArgs<TResult>/**/>* handler = static_cast<EventHandler<ApiInvokeResultEventArgs<TResult>/**/>*>(e->GetState());
        if (handler != NULL)
        {
            ApiInvokeResultEventArgs<TResult> result;

            result.GetEventArgs() = e;
            result.GetResult() = &model;
            result.GetError() = error;

            handler->Invoke(target, &result);
        }
    }
};