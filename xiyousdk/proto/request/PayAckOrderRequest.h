#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class PayAckOrderRequest;

class PayAckOrderRequest {
private:
    const char* ProductName;
    unsigned short ProductName_len;
    const char* OrderNo;
    unsigned short OrderNo_len;
    const char* ChannelOrderNo;
    unsigned short ChannelOrderNo_len;
    const char* UserID;
    unsigned short UserID_len;
    int Money;
    const char* Currency;
    unsigned short Currency_len;
    const char* Extension;
    unsigned short Extension_len;
    int BuyType;
    const char* Platform;
    unsigned short Platform_len;

public:
    PayAckOrderRequest() {
        this->ProductName = NULL;
        this->ProductName_len = -1;
        this->OrderNo = NULL;
        this->OrderNo_len = -1;
        this->ChannelOrderNo = NULL;
        this->ChannelOrderNo_len = -1;
        this->UserID = NULL;
        this->UserID_len = -1;
        this->Money = 0;
        this->Currency = NULL;
        this->Currency_len = -1;
        this->Extension = NULL;
        this->Extension_len = -1;
        this->BuyType = 0;
        this->Platform = NULL;
        this->Platform_len = -1;
    }
    ~PayAckOrderRequest() {
        const char* sz = NULL;
        sz = this->ProductName;
        this->ProductName = NULL;
        this->ProductName_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->OrderNo;
        this->OrderNo = NULL;
        this->OrderNo_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->ChannelOrderNo;
        this->ChannelOrderNo = NULL;
        this->ChannelOrderNo_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->UserID;
        this->UserID = NULL;
        this->UserID_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->Currency;
        this->Currency = NULL;
        this->Currency_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->Extension;
        this->Extension = NULL;
        this->Extension_len = -1;
        if ( sz != NULL ) delete sz;
        sz = this->Platform;
        this->Platform = NULL;
        this->Platform_len = -1;
        if ( sz != NULL ) delete sz;
    }

public:
    const char* GetProductName() {
        if ( this->ProductName_len == 0xffff ) return NULL;
        else if ( this->ProductName_len == 0 ) return "";
        return this->ProductName;

    };
    int GetProductNameLength() {
        return ( ( this->ProductName_len == 0xffff ) ? -1 : this->ProductName_len );
    };
    const char* GetOrderNo() {
        if ( this->OrderNo_len == 0xffff ) return NULL;
        else if ( this->OrderNo_len == 0 ) return "";
        return this->OrderNo;

    };
    int GetOrderNoLength() {
        return ( ( this->OrderNo_len == 0xffff ) ? -1 : this->OrderNo_len );
    };
    const char* GetChannelOrderNo() {
        if ( this->ChannelOrderNo_len == 0xffff ) return NULL;
        else if ( this->ChannelOrderNo_len == 0 ) return "";
        return this->ChannelOrderNo;

    };
    int GetChannelOrderNoLength() {
        return ( ( this->ChannelOrderNo_len == 0xffff ) ? -1 : this->ChannelOrderNo_len );
    };
    const char* GetUserID() {
        if ( this->UserID_len == 0xffff ) return NULL;
        else if ( this->UserID_len == 0 ) return "";
        return this->UserID;

    };
    int GetUserIDLength() {
        return ( ( this->UserID_len == 0xffff ) ? -1 : this->UserID_len );
    };
    int GetMoney() {
        return this->Money;
    };
    const char* GetCurrency() {
        if ( this->Currency_len == 0xffff ) return NULL;
        else if ( this->Currency_len == 0 ) return "";
        return this->Currency;

    };
    int GetCurrencyLength() {
        return ( ( this->Currency_len == 0xffff ) ? -1 : this->Currency_len );
    };
    const char* GetExtension() {
        if ( this->Extension_len == 0xffff ) return NULL;
        else if ( this->Extension_len == 0 ) return "";
        return this->Extension;

    };
    int GetExtensionLength() {
        return ( ( this->Extension_len == 0xffff ) ? -1 : this->Extension_len );
    };
    int GetBuyType() {
        return this->BuyType;
    };
    const char* GetPlatform() {
        if ( this->Platform_len == 0xffff ) return NULL;
        else if ( this->Platform_len == 0 ) return "";
        return this->Platform;

    };
    int GetPlatformLength() {
        return ( ( this->Platform_len == 0xffff ) ? -1 : this->Platform_len );
    };

public:
    void SetProductName( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->ProductName_len == 0 && len == 0 ) return;
        if ( this->ProductName_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->ProductName ) delete[] this->ProductName;
        this->ProductName = NULL;
        this->ProductName_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->ProductName = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->ProductName;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->ProductName_len = len;
    };
    void SetOrderNo( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->OrderNo_len == 0 && len == 0 ) return;
        if ( this->OrderNo_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->OrderNo ) delete[] this->OrderNo;
        this->OrderNo = NULL;
        this->OrderNo_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->OrderNo = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->OrderNo;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->OrderNo_len = len;
    };
    void SetChannelOrderNo( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->ChannelOrderNo_len == 0 && len == 0 ) return;
        if ( this->ChannelOrderNo_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->ChannelOrderNo ) delete[] this->ChannelOrderNo;
        this->ChannelOrderNo = NULL;
        this->ChannelOrderNo_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->ChannelOrderNo = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->ChannelOrderNo;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->ChannelOrderNo_len = len;
    };
    void SetUserID( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->UserID_len == 0 && len == 0 ) return;
        if ( this->UserID_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->UserID ) delete[] this->UserID;
        this->UserID = NULL;
        this->UserID_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->UserID = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->UserID;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->UserID_len = len;
    };
    void SetMoney( int value ) {
        this->Money = value;
    };
    void SetCurrency( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->Currency_len == 0 && len == 0 ) return;
        if ( this->Currency_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->Currency ) delete[] this->Currency;
        this->Currency = NULL;
        this->Currency_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->Currency = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->Currency;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->Currency_len = len;
    };
    void SetExtension( const char* s, unsigned short len = 0xffff ) {
        if ( len == 0xffff && s != NULL ) len = ( unsigned short )strlen( s );
        if ( this->Extension_len == 0 && len == 0 ) return;
        if ( this->Extension_len == 0xffff && len == 0xffff ) return;
        if ( NULL != this->Extension ) delete[] this->Extension;
        this->Extension = NULL;
        this->Extension_len = 0xffff;
        if ( len > 0 && len != 0xffff ) this->Extension = new char[ len + 1 ];
        if ( NULL != s && ( len > 0 && len != 0xffff ) ) {
            char* sz = ( char* )this->Extension;
            memcpy( sz, ( char* )s, len );
            sz[ len ] = '\x0';
        }
        this->Extension_len = len;
    };
    void SetBuyType( int value ) {
        this->BuyType = value;
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

public:
    bool Deserialize( unsigned char*& buf, int& count ) {
        if ( ( count -= 1 ) < 0 ) return false;
        if ( *buf++ != 0 ) return false;
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetProductName( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetProductName( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetProductName( ( char* )buf, len_ );
                buf += len_;
            }
        }
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetOrderNo( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetOrderNo( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetOrderNo( ( char* )buf, len_ );
                buf += len_;
            }
        }
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetChannelOrderNo( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetChannelOrderNo( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetChannelOrderNo( ( char* )buf, len_ );
                buf += len_;
            }
        }
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetUserID( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetUserID( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetUserID( ( char* )buf, len_ );
                buf += len_;
            }
        }
        if ( ( count -= 4 ) < 0 ) return false;
        this->Money = *( int* )buf;
        buf += 4;
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetCurrency( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetCurrency( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetCurrency( ( char* )buf, len_ );
                buf += len_;
            }
        }
        {
            if ( ( count -= 2 ) < 0 ) return false;
            unsigned short len_ = *( unsigned short* )buf;
            buf += 2;
            if ( len_ == 0xffff ) this->SetExtension( NULL, 0xffff );
            else if ( len_ == 0 ) this->SetExtension( "", 0 );
            else {
                if ( ( count -= len_ ) < 0 ) return false;
                this->SetExtension( ( char* )buf, len_ );
                buf += len_;
            }
        }
        if ( ( count -= 4 ) < 0 ) return false;
        this->BuyType = *( int* )buf;
        buf += 4;
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
        return true;
    }
    void Serialize( std::strstream& s ) {
        s.put( NULL == this );
        s.write( ( char* )&this->ProductName_len, 2 );
        if ( this->ProductName_len > 0 && this->ProductName_len != 0xffff )
            s.write( this->ProductName, this->ProductName_len );
        s.write( ( char* )&this->OrderNo_len, 2 );
        if ( this->OrderNo_len > 0 && this->OrderNo_len != 0xffff )
            s.write( this->OrderNo, this->OrderNo_len );
        s.write( ( char* )&this->ChannelOrderNo_len, 2 );
        if ( this->ChannelOrderNo_len > 0 && this->ChannelOrderNo_len != 0xffff )
            s.write( this->ChannelOrderNo, this->ChannelOrderNo_len );
        s.write( ( char* )&this->UserID_len, 2 );
        if ( this->UserID_len > 0 && this->UserID_len != 0xffff )
            s.write( this->UserID, this->UserID_len );
        s.write( ( char* )&this->Money, 4 );
        s.write( ( char* )&this->Currency_len, 2 );
        if ( this->Currency_len > 0 && this->Currency_len != 0xffff )
            s.write( this->Currency, this->Currency_len );
        s.write( ( char* )&this->Extension_len, 2 );
        if ( this->Extension_len > 0 && this->Extension_len != 0xffff )
            s.write( this->Extension, this->Extension_len );
        s.write( ( char* )&this->BuyType, 4 );
        s.write( ( char* )&this->Platform_len, 2 );
        if ( this->Platform_len > 0 && this->Platform_len != 0xffff )
            s.write( this->Platform, this->Platform_len );
    }
};
