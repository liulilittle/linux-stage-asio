#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class LoginAccountRequest;

class LoginAccountRequest {
private:
    const char* UserToken;
    unsigned short UserToken_len;
    unsigned int ClientIP;
    const char* ClientInfo;
    unsigned short ClientInfo_len;

public:
    LoginAccountRequest() {
        this->UserToken = NULL;
        this->UserToken_len = -1;
        this->ClientIP = 0;
        this->ClientInfo = NULL;
        this->ClientInfo_len = -1;
    }
    ~LoginAccountRequest() {
        const char* sz = NULL;
        sz = this->UserToken;
        this->UserToken = NULL;
        this->UserToken_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ClientInfo;
        this->ClientInfo = NULL;
        this->ClientInfo_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    const char* GetUserToken() {
        if (this->UserToken_len == 0xffff) return NULL;
        else if (this->UserToken_len == 0) return "";
        return this->UserToken;

    };
    int GetUserTokenLength() {
        return ((this->UserToken_len == 0xffff) ? -1 : this->UserToken_len);
    };
    unsigned int GetClientIP() {
        return this->ClientIP;
    };
    const char* GetClientInfo() {
        if (this->ClientInfo_len == 0xffff) return NULL;
        else if (this->ClientInfo_len == 0) return "";
        return this->ClientInfo;

    };
    int GetClientInfoLength() {
        return ((this->ClientInfo_len == 0xffff) ? -1 : this->ClientInfo_len);
    };

public:
    void SetUserToken(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->UserToken_len == 0 && len == 0) return;
        if (this->UserToken_len == 0xffff && len == 0xffff) return;
        if (NULL != this->UserToken) delete[] this->UserToken;
        this->UserToken = NULL;
        this->UserToken_len = 0xffff;
        if (len > 0 && len != 0xffff) this->UserToken = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->UserToken;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->UserToken_len = len;
    };
    void SetClientIP(unsigned int value) {
        this->ClientIP = value;
    };
    void SetClientInfo(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ClientInfo_len == 0 && len == 0) return;
        if (this->ClientInfo_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ClientInfo) delete[] this->ClientInfo;
        this->ClientInfo = NULL;
        this->ClientInfo_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ClientInfo = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ClientInfo;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ClientInfo_len = len;
    };

public:
    bool Deserialize(unsigned char*& buf, int& count) {
        if ((count -= 1) < 0) return false;
        if (*buf++ != 0) return false;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetUserToken(NULL, 0xffff);
            else if (len_ == 0) this->SetUserToken("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetUserToken((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 4) < 0) return false;
        this->ClientIP = *(unsigned int*)buf;
        buf += 4;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetClientInfo(NULL, 0xffff);
            else if (len_ == 0) this->SetClientInfo("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetClientInfo((char*)buf, len_);
                buf += len_;
            }
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->UserToken_len, 2);
        if (this->UserToken_len > 0 && this->UserToken_len != 0xffff)
            s.write(this->UserToken, this->UserToken_len);
        s.write((char*)&this->ClientIP, 4);
        s.write((char*)&this->ClientInfo_len, 2);
        if (this->ClientInfo_len > 0 && this->ClientInfo_len != 0xffff)
            s.write(this->ClientInfo, this->ClientInfo_len);
    }
};