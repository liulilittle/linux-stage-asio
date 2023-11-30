#pragma once

#include "../proto/inc/XiYouGMWeb.h"
#include "../net/SocketClient.h"

#include <map>

class GMWebInstructionDefaultHandler : public SocketHandler
{
protected:
    GMWebInstructionDefaultHandler() { } 

public:
    virtual unsigned short                          GetCommandId()
    {
        return XiYouSdkCommands_GMWebInstruction;
    }
    virtual void                                    ProcessRequest( SocketMessage& message )
    {
        GenericResponse<const char*> response;
        GMWebInstruction request;
        std::string interfaces = "";
        if ( SocketMessage::Deserialize( request, message ) )
        {
            if ( request.GetInterfaceLength() > 0 )
            {
                interfaces = std::string( request.GetInterface(), request.GetInterfaceLength() );
            }
            std::map<std::string, std::string> parameters;
            if ( !AcquireRequest( request, parameters ) )
            {
                ProcessError( interfaces, response );
            }
            else
            {
                ProcessRequest( interfaces, parameters, response );
            }
        }
        else
        {
            ProcessError( interfaces, response );
        }
        ResponseWrite( message.GetClient(), message.GetSequenceNo(), response );
    };

public:
    inline static int                               BufferTransform( char* in, int inlen, char* out, int outlen )
    {
        if ( in == NULL || inlen <= 0 )
        {
            return 0;
        }
        if ( out == NULL || outlen <= 0 )
        {
            return inlen;
        }
        int transformcount = outlen / 2;
        for ( register int i = 0; i < transformcount; i++ )
        {
            char b1 = in[ i * 2 ];
            char b2 = in[ i * 2 + 1 ];

            if ( b1 >= '0' && b1 <= '9' )
                b1 -= '0';
            else if ( b1 >= 'A' && b1 <= 'F' )
                b1 = ( b1 - 'A' ) + 10;
            else if ( b1 >= 'a' && b1 <= 'f' )
                b1 = ( b1 - 'a' ) + 10;
            else
                return -1;

            if ( b2 >= '0' && b2 <= '9' )
                b2 -= '0';
            else if ( b2 >= 'A' && b2 <= 'F' )
                b2 = ( b2 - 'A' ) + 10;
            else if ( b2 >= 'a' && b2 <= 'f' )
                b2 = ( b2 - 'a' ) + 10;
            else
                return -1;

            out[ i ] = ( char )( b1 * 16 + b2 );
        }
        return transformcount;
    }

protected:
    //template<typename TString>
    typedef std::string TString;
    inline bool                                     AcquireRequest( GMWebInstruction& instruction, std::map<TString, TString>& parameters ) 
    {
        const char* contents = instruction.GetRequest();
        int len = instruction.GetRequestLength();
        if ( (contents == NULL && len > 0) || (len < 0) )
        {
            return false;
        }
        if ( contents[ len ] != '\x0' )
        {
            return false;
        }
        char* parameter = strtok( ( char* )contents, "&" );
        while ( parameter != NULL )
        {
            char* kv = strstr( parameter, "=" );
            if ( kv == NULL )
            {
                continue;
            }
            else
            {
                *kv = '\x0';
            }
            char* key = parameter;
            char* value = kv + 1;
            if ( *key == '\x0' )
            {
                continue;
            }
            else
            {
                int count = ( int )strlen( value );
                count = BufferTransform( value, count, value, count );
                if ( count < 0 )
                {
                    return false;
                }
                value[ count ] = '\x0';
            }
            parameters.insert( std::map<TString, TString>::value_type( key, value ) );
            parameter = strtok( NULL, "&" );
        }
        return true;
    }

public:
    virtual void                                    ProcessError( std::string& interfaces, GenericResponse<const char*>& response )
    {
        response.SetCode( 412 );
        response.SetTag( NULL );
        response.SetMessage( interfaces.data(), (unsigned short)interfaces.length() );
    }
    virtual void                                    ProcessRequest( std::string& interfaces, std::map<std::string, std::string>& args, GenericResponse<const char*>& response )
    {

    }
};