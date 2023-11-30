#pragma once

namespace ep
{
    namespace stage
    {
        namespace model
        {
            enum Error
            {
                Error_Success,
                Error_PendingAuthentication,
                Error_TheObjectHasBeenFreed,
                Error_UnableToFindThisServer,
                Error_SvrAreaNoItIsIllegal,
                Error_NoLinkTypeIsHostedOnTheStage,
                Error_TheInternalServerIsNotOnline,
                Error_LimitTheNumberOfInternalServerClients,
                Error_TheLinkNoCouldNotBeAssigned,
                Error_UnableToAllocSocketVectorObject,
                Error_UnableToAllocServerObject,
                Error_UnableTryGetValueSocketVectorObject,
                Error_UnableToPostEstablishLinkToInternalServer,
                Error_SeriousServerInternalError,
                Error_UnableToPushAllAcceptClient,
                Error_UnableToPushToAllServers,
                Error_UnableToCreateAcceptServerIL,
                Error_NotAllowedToProvideSocketNullReferences,
                Error_UnableToGetSocketCommunication,
                Error_UnableToGetSendbackController,
                Error_InternalServerActivelyRefused,
            };
        }
    }
}
