#pragma once

#include "ApiController.h"
#include "../net/XiYouSdkClient.h"
#include "../proto/inc/XiYouAccount.h"

class XiYouAccountApi : public ApiController
{
public:
    XiYouAccountApi(XiYouSdkClient& sdkClient) : ApiController(sdkClient)
    {
        
    }
    virtual ~XiYouAccountApi()
    {
        
    }

public:
    virtual bool LoginAsync(const char* szUserToken, unsigned int szClientIP, EventHandler<ApiInvokeResultEventArgs<GenericResponse<LoginAccountResponse>/**/>/**/>* callback)
    {
        if (szUserToken == NULL || (szClientIP == 0 || szClientIP == 0xffffffff))
        {
            return false;
        }
        LoginAccountRequest request;

        request.SetClientIP(szClientIP);
        request.SetUserToken(szUserToken);

        return ApiController::InvokeAsync<LoginAccountRequest, GenericResponse<LoginAccountResponse>/**/>(XiYouSdkCommands_LoginAccount, request, callback);
    }

    virtual bool FeedbackAsync(unsigned char bAppCategory, const char* szAccountID, const char* szRoleID, const char* szReportContent,
        EventHandler<ApiInvokeResultEventArgs<GenericResponse<char>/**/>/**/>* callback)
    {
        if (szAccountID == NULL || szRoleID == NULL || szReportContent == NULL)
        {
            return false;
        }
        FeedbackAccountRequest request;
        
        request.SetAccountId(szAccountID);
        request.SetReportContent(szReportContent);
        request.SetRoleId(szRoleID);
        request.SetAppCategory(bAppCategory);

        return ApiController::InvokeAsync<FeedbackAccountRequest, GenericResponse<char>/**/>(XiYouSdkCommands_FeedbackAccount, request, callback);
    }
};