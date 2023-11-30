#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class GMWebInstruction;

class GMWebInstruction {
private:
    const char* Interface;
    unsigned short Interface_len;
    const char* Platform;
    unsigned short Platform_len;
    unsigned int TimeSpan;
    const char* ProcessRequest;
    unsigned short Request_len;

public:
    GMWebInstruction() {
        this->Interface = NULL;
        this->Interface_len = -1;
        this->Platform = NULL;
        this->Platform_len = -1;
        this->TimeSpan = 0;
        this->ProcessRequest = NULL;
        this->Request_len = -1;
    }
    ~GMWebInstruction() {
        const char* sz = NULL;
        sz = this->Interface;
        this->Interface = NULL;
        this->Interface_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->Platform;
        this->Platform = NULL;
        this->Platform_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->ProcessRequest;
        this->ProcessRequest = NULL;
        this->Request_len = -1;
        if ( sz != NULL ) delete sz;
    }

public:
    const char* GetInterface() {
        if ( this->Interface_len == 0xffff ) return NULL;
        else if ( this->Interface_len == 0 ) return "";
        return this->Interface;

    };
    int GetInterfaceLength() {
        return ( ( this->Interface_len == 0xffff ) ? -1 : this->Interface_len );
    };
    const char* GetPlatform() {
        if ( this->Platform_len == 0xffff ) return NULL;
        else if ( this->Platform_len == 0 ) return "";
        return this->Platform;

    };
    int GetPlatformLength() {
        return ( ( this->Platform_len == 0xffff ) ? -1 : this->Platform_len );
    };
    unsigned int GetTimeSpan() {
        return this->TimeSpan;
    };
    const char* GetRequest() {
        if ( this->Request_len == 0xffff ) return NULL;
        else if ( this->Request_len == 0 ) return "";
        return this->ProcessRequest;

    };
    int GetRequestLength() {
        return ( ( this->Request_len == 0xffff ) ? -1 : this->Request_len );
    };

public:
    void SetInterface( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->Interface_len == 0 && len == 0 ) return;
        if ( this->Interface_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->Interface ) delete[] this->Interface;
        this->Interface = NULL;
        this->Interface_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->Interface = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->Interface;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->Interface_len = len;
    };
    void SetPlatform( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->Platform_len == 0 && len == 0 ) return;
        if ( this->Platform_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->Platform ) delete[] this->Platform;
        this->Platform = NULL;
        this->Platform_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->Platform = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->Platform;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->Platform_len = len;
    };
    void SetTimeSpan( unsigned int value ) {
        this->TimeSpan = value;
    };
    void SetRequest( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->Request_len == 0 && len == 0 ) return;
        if ( this->Request_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->ProcessRequest ) delete[] this->ProcessRequest;
        this->ProcessRequest = NULL;
        this->Request_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->ProcessRequest = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->ProcessRequest;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->Request_len = len;
    };

public:
    bool Deserialize( unsigned char*& buf, int& count ) {
        if ( ( count -= 1 ) < 0 ) return false;
        if ( *buf++ != 0 ) return false;
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetInterface( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetInterface( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetInterface( ( char* )buf, len_ );
                buf += len_;
            }
        }
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetPlatform( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetPlatform( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetPlatform( ( char* )buf, len_ );
                buf += len_;
            }
        }
        if ( ( count -= 4 ) < 0 ) return false;
        this->TimeSpan = *( unsigned int* )buf;
        buf += 4;
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetRequest( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetRequest( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetRequest( ( char* )buf, len_ );
                buf += len_;
            }
        }
        return true;
    }
    void Serialize( std::strstream& s ) {
        s.put( NULL == this );
        s.write( ( char* )&this->Interface_len, 2 );
        if ( this->Interface_len > 0 && this->Interface_len != 0xffff )
            s.write( this->Interface, this->Interface_len );
        s.write( ( char* )&this->Platform_len, 2 );
        if ( this->Platform_len > 0 && this->Platform_len != 0xffff )
            s.write( this->Platform, this->Platform_len );
        s.write( ( char* )&this->TimeSpan, 4 );
        s.write( ( char* )&this->Request_len, 2 );
        if ( this->Request_len > 0 && this->Request_len != 0xffff )
            s.write( this->ProcessRequest, this->Request_len );
    }
};
