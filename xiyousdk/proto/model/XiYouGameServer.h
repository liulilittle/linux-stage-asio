#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class XiYouGameServer;

class XiYouGameServer {
private:
    const char* ServerName;
    unsigned short ServerName_len;
    unsigned char IsCommend;
    unsigned char Type;
    const char* ServerId;
    unsigned short ServerId_len;
    unsigned char Status;
    int ActiveTime;
    int LoginTime;
    const char* Platform;
    unsigned short Platform_len;

public:
    XiYouGameServer() {
        this->ServerName = NULL;
        this->ServerName_len = -1;
        this->IsCommend = 0;
        this->Type = 0;
        this->ServerId = NULL;
        this->ServerId_len = -1;
        this->Status = 0;
        this->ActiveTime = 0;
        this->LoginTime = 0;
        this->Platform = NULL;
        this->Platform_len = -1;
    }
    ~XiYouGameServer() {
        const char* sz = NULL;
        sz = this->ServerName;
        this->ServerName = NULL;
        this->ServerName_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ServerId;
        this->ServerId = NULL;
        this->ServerId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->Platform;
        this->Platform = NULL;
        this->Platform_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    const char* GetServerName() {
        if (this->ServerName_len == 0xffff) return NULL;
        else if (this->ServerName_len == 0) return "";
        return this->ServerName;

    };
    int GetServerNameLength() {
        return ((this->ServerName_len == 0xffff) ? -1 : this->ServerName_len);
    };
    unsigned char GetIsCommend() {
        return this->IsCommend;
    };
    unsigned char GetType() {
        return this->Type;
    };
    const char* GetServerId() {
        if (this->ServerId_len == 0xffff) return NULL;
        else if (this->ServerId_len == 0) return "";
        return this->ServerId;

    };
    int GetServerIdLength() {
        return ((this->ServerId_len == 0xffff) ? -1 : this->ServerId_len);
    };
    unsigned char GetStatus() {
        return this->Status;
    };
    int GetActiveTime() {
        return this->ActiveTime;
    };
    int GetLoginTime() {
        return this->LoginTime;
    };
    const char* GetPlatform() {
        if (this->Platform_len == 0xffff) return NULL;
        else if (this->Platform_len == 0) return "";
        return this->Platform;

    };
    int GetPlatformLength() {
        return ((this->Platform_len == 0xffff) ? -1 : this->Platform_len);
    };

public:
    void SetServerName(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ServerName_len == 0 && len == 0) return;
        if (this->ServerName_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ServerName) delete[] this->ServerName;
        this->ServerName = NULL;
        this->ServerName_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ServerName = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ServerName;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ServerName_len = len;
    };
    void SetIsCommend(unsigned char value) {
        this->IsCommend = value;
    };
    void SetType(unsigned char value) {
        this->Type = value;
    };
    void SetServerId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ServerId_len == 0 && len == 0) return;
        if (this->ServerId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ServerId) delete[] this->ServerId;
        this->ServerId = NULL;
        this->ServerId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ServerId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ServerId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ServerId_len = len;
    };
    void SetStatus(unsigned char value) {
        this->Status = value;
    };
    void SetActiveTime(int value) {
        this->ActiveTime = value;
    };
    void SetLoginTime(int value) {
        this->LoginTime = value;
    };
    void SetPlatform(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->Platform_len == 0 && len == 0) return;
        if (this->Platform_len == 0xffff && len == 0xffff) return;
        if (NULL != this->Platform) delete[] this->Platform;
        this->Platform = NULL;
        this->Platform_len = 0xffff;
        if (len > 0 && len != 0xffff) this->Platform = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->Platform;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->Platform_len = len;
    };

public:
    bool Deserialize(unsigned char*& buf, int& count) {
        if ((count -= 1) < 0) return false;
        if (*buf++ != 0) return false;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetServerName(NULL, 0xffff);
            else if (len_ == 0) this->SetServerName("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetServerName((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 1) < 0) return false;
        this->IsCommend = *(unsigned char*)buf;
        buf++;
        if ((count -= 1) < 0) return false;
        this->Type = *(unsigned char*)buf;
        buf++;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetServerId(NULL, 0xffff);
            else if (len_ == 0) this->SetServerId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetServerId((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 1) < 0) return false;
        this->Status = *(unsigned char*)buf;
        buf++;
        if ((count -= 4) < 0) return false;
        this->ActiveTime = *(int*)buf;
        buf += 4;
        if ((count -= 4) < 0) return false;
        this->LoginTime = *(int*)buf;
        buf += 4;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetPlatform(NULL, 0xffff);
            else if (len_ == 0) this->SetPlatform("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetPlatform((char*)buf, len_);
                buf += len_;
            }
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->ServerName_len, 2);
        if (this->ServerName_len > 0 && this->ServerName_len != 0xffff)
            s.write(this->ServerName, this->ServerName_len);
        s.put((char)this->IsCommend);
        s.put((char)this->Type);
        s.write((char*)&this->ServerId_len, 2);
        if (this->ServerId_len > 0 && this->ServerId_len != 0xffff)
            s.write(this->ServerId, this->ServerId_len);
        s.put((char)this->Status);
        s.write((char*)&this->ActiveTime, 4);
        s.write((char*)&this->LoginTime, 4);
        s.write((char*)&this->Platform_len, 2);
        if (this->Platform_len > 0 && this->Platform_len != 0xffff)
            s.write(this->Platform, this->Platform_len);
    }
};
