#pragma once

#include "ApiController.h"
#include "../net/XiYouSdkClient.h"
#include "../proto/inc/XiYouOrder.h"

class XiYouOrderApi : public ApiController
{
public:
    XiYouOrderApi(XiYouSdkClient& sdkClient) : ApiController(sdkClient)
    {

    }
    virtual ~XiYouOrderApi()
    {

    }

public:
    virtual bool CreateAsync(CreateOrderRequest& order, EventHandler<ApiInvokeResultEventArgs<GenericResponse<CreateOrderResponse>/**/>/**/>* callback)
    {
        if (order.GetRoleNameLength() <= 0)
        {
            return false;
        }
        if (order.GetProduceNameLength() <= 0)
        {
            return false;
        }
        if (order.GetProduceDescLength() <= 0)
        {
            return false;
        }
        if (order.GetExtensionLength() <= 0)
        {
            return false;
        }
        if (order.GetPidClientFlagsLength() <= 0)
        {
            return false;
        }
        if (order.GetUserIdLength() <= 0)
        {
            return false;
        }
        return ApiController::InvokeAsync<CreateOrderRequest, GenericResponse<CreateOrderResponse>/**/>(XiYouSdkCommands_CreateOrder, order, callback);
    }
};