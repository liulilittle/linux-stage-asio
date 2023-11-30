#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class CreateOrderRequest;

class CreateOrderRequest {
private:
    unsigned char AppCategory;
    const char* UserId;
    unsigned short UserId_len;
    int ProductId;
    const char* ProduceName;
    unsigned short ProduceName_len;
    const char* ProduceDesc;
    unsigned short ProduceDesc_len;
    int BuyNum;
    int Money;
    long long RoleId;
    const char* RoleName;
    unsigned short RoleName_len;
    int RoleLevel;
    const char* Extension;
    unsigned short Extension_len;
    const char* PidClientFlags;
    unsigned short PidClientFlags_len;
    unsigned int ClientIP;
    const char* ClientInfo;
    unsigned short ClientInfo_len;

public:
    CreateOrderRequest() {
        this->AppCategory = 0;
        this->UserId = NULL;
        this->UserId_len = -1;
        this->ProductId = 0;
        this->ProduceName = NULL;
        this->ProduceName_len = -1;
        this->ProduceDesc = NULL;
        this->ProduceDesc_len = -1;
        this->BuyNum = 0;
        this->Money = 0;
        this->RoleId = 0;
        this->RoleName = NULL;
        this->RoleName_len = -1;
        this->RoleLevel = 0;
        this->Extension = NULL;
        this->Extension_len = -1;
        this->PidClientFlags = NULL;
        this->PidClientFlags_len = -1;
        this->ClientIP = 0;
        this->ClientInfo = NULL;
        this->ClientInfo_len = -1;
    }
    ~CreateOrderRequest() {
        const char* sz = NULL;
        sz = this->UserId;
        this->UserId = NULL;
        this->UserId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ProduceName;
        this->ProduceName = NULL;
        this->ProduceName_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ProduceDesc;
        this->ProduceDesc = NULL;
        this->ProduceDesc_len = -1;
        if (sz != NULL) delete sz;
        sz = this->RoleName;
        this->RoleName = NULL;
        this->RoleName_len = -1;
        if (sz != NULL) delete sz;
        sz = this->Extension;
        this->Extension = NULL;
        this->Extension_len = -1;
        if (sz != NULL) delete sz;
        sz = this->PidClientFlags;
        this->PidClientFlags = NULL;
        this->PidClientFlags_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ClientInfo;
        this->ClientInfo = NULL;
        this->ClientInfo_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    unsigned char GetAppCategory() {
        return this->AppCategory;
    };
    const char* GetUserId() {
        if (this->UserId_len == 0xffff) return NULL;
        else if (this->UserId_len == 0) return "";
        return this->UserId;

    };
    int GetUserIdLength() {
        return ((this->UserId_len == 0xffff) ? -1 : this->UserId_len);
    };
    int GetProductId() {
        return this->ProductId;
    };
    const char* GetProduceName() {
        if (this->ProduceName_len == 0xffff) return NULL;
        else if (this->ProduceName_len == 0) return "";
        return this->ProduceName;

    };
    int GetProduceNameLength() {
        return ((this->ProduceName_len == 0xffff) ? -1 : this->ProduceName_len);
    };
    const char* GetProduceDesc() {
        if (this->ProduceDesc_len == 0xffff) return NULL;
        else if (this->ProduceDesc_len == 0) return "";
        return this->ProduceDesc;

    };
    int GetProduceDescLength() {
        return ((this->ProduceDesc_len == 0xffff) ? -1 : this->ProduceDesc_len);
    };
    int GetBuyNum() {
        return this->BuyNum;
    };
    int GetMoney() {
        return this->Money;
    };
    long long GetRoleId() {
        return this->RoleId;
    };
    const char* GetRoleName() {
        if (this->RoleName_len == 0xffff) return NULL;
        else if (this->RoleName_len == 0) return "";
        return this->RoleName;

    };
    int GetRoleNameLength() {
        return ((this->RoleName_len == 0xffff) ? -1 : this->RoleName_len);
    };
    int GetRoleLevel() {
        return this->RoleLevel;
    };
    const char* GetExtension() {
        if (this->Extension_len == 0xffff) return NULL;
        else if (this->Extension_len == 0) return "";
        return this->Extension;

    };
    int GetExtensionLength() {
        return ((this->Extension_len == 0xffff) ? -1 : this->Extension_len);
    };
    const char* GetPidClientFlags() {
        if (this->PidClientFlags_len == 0xffff) return NULL;
        else if (this->PidClientFlags_len == 0) return "";
        return this->PidClientFlags;

    };
    int GetPidClientFlagsLength() {
        return ((this->PidClientFlags_len == 0xffff) ? -1 : this->PidClientFlags_len);
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
    void SetAppCategory(unsigned char value) {
        this->AppCategory = value;
    };
    void SetUserId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->UserId_len == 0 && len == 0) return;
        if (this->UserId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->UserId) delete[] this->UserId;
        this->UserId = NULL;
        this->UserId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->UserId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->UserId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->UserId_len = len;
    };
    void SetProductId(int value) {
        this->ProductId = value;
    };
    void SetProduceName(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ProduceName_len == 0 && len == 0) return;
        if (this->ProduceName_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ProduceName) delete[] this->ProduceName;
        this->ProduceName = NULL;
        this->ProduceName_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ProduceName = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ProduceName;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ProduceName_len = len;
    };
    void SetProduceDesc(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ProduceDesc_len == 0 && len == 0) return;
        if (this->ProduceDesc_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ProduceDesc) delete[] this->ProduceDesc;
        this->ProduceDesc = NULL;
        this->ProduceDesc_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ProduceDesc = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ProduceDesc;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ProduceDesc_len = len;
    };
    void SetBuyNum(int value) {
        this->BuyNum = value;
    };
    void SetMoney(int value) {
        this->Money = value;
    };
    void SetRoleId(long long value) {
        this->RoleId = value;
    };
    void SetRoleName(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->RoleName_len == 0 && len == 0) return;
        if (this->RoleName_len == 0xffff && len == 0xffff) return;
        if (NULL != this->RoleName) delete[] this->RoleName;
        this->RoleName = NULL;
        this->RoleName_len = 0xffff;
        if (len > 0 && len != 0xffff) this->RoleName = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->RoleName;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->RoleName_len = len;
    };
    void SetRoleLevel(int value) {
        this->RoleLevel = value;
    };
    void SetExtension(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->Extension_len == 0 && len == 0) return;
        if (this->Extension_len == 0xffff && len == 0xffff) return;
        if (NULL != this->Extension) delete[] this->Extension;
        this->Extension = NULL;
        this->Extension_len = 0xffff;
        if (len > 0 && len != 0xffff) this->Extension = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->Extension;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->Extension_len = len;
    };
    void SetPidClientFlags(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->PidClientFlags_len == 0 && len == 0) return;
        if (this->PidClientFlags_len == 0xffff && len == 0xffff) return;
        if (NULL != this->PidClientFlags) delete[] this->PidClientFlags;
        this->PidClientFlags = NULL;
        this->PidClientFlags_len = 0xffff;
        if (len > 0 && len != 0xffff) this->PidClientFlags = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->PidClientFlags;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->PidClientFlags_len = len;
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
        if ((count -= 1) < 0) return false;
        this->AppCategory = *(unsigned char*)buf;
        buf++;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetUserId(NULL, 0xffff);
            else if (len_ == 0) this->SetUserId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetUserId((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 4) < 0) return false;
        this->ProductId = *(int*)buf;
        buf += 4;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetProduceName(NULL, 0xffff);
            else if (len_ == 0) this->SetProduceName("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetProduceName((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetProduceDesc(NULL, 0xffff);
            else if (len_ == 0) this->SetProduceDesc("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetProduceDesc((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 4) < 0) return false;
        this->BuyNum = *(int*)buf;
        buf += 4;
        if ((count -= 4) < 0) return false;
        this->Money = *(int*)buf;
        buf += 4;
        if ((count -= 8) < 0) return false;
        this->RoleId = *(long long*)buf;
        buf += 8;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetRoleName(NULL, 0xffff);
            else if (len_ == 0) this->SetRoleName("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetRoleName((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count -= 4) < 0) return false;
        this->RoleLevel = *(int*)buf;
        buf += 4;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetExtension(NULL, 0xffff);
            else if (len_ == 0) this->SetExtension("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetExtension((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetPidClientFlags(NULL, 0xffff);
            else if (len_ == 0) this->SetPidClientFlags("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetPidClientFlags((char*)buf, len_);
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
        s.put((char)this->AppCategory);
        s.write((char*)&this->UserId_len, 2);
        if (this->UserId_len > 0 && this->UserId_len != 0xffff)
            s.write(this->UserId, this->UserId_len);
        s.write((char*)&this->ProductId, 4);
        s.write((char*)&this->ProduceName_len, 2);
        if (this->ProduceName_len > 0 && this->ProduceName_len != 0xffff)
            s.write(this->ProduceName, this->ProduceName_len);
        s.write((char*)&this->ProduceDesc_len, 2);
        if (this->ProduceDesc_len > 0 && this->ProduceDesc_len != 0xffff)
            s.write(this->ProduceDesc, this->ProduceDesc_len);
        s.write((char*)&this->BuyNum, 4);
        s.write((char*)&this->Money, 4);
        s.write((char*)&this->RoleId, 8);
        s.write((char*)&this->RoleName_len, 2);
        if (this->RoleName_len > 0 && this->RoleName_len != 0xffff)
            s.write(this->RoleName, this->RoleName_len);
        s.write((char*)&this->RoleLevel, 4);
        s.write((char*)&this->Extension_len, 2);
        if (this->Extension_len > 0 && this->Extension_len != 0xffff)
            s.write(this->Extension, this->Extension_len);
        s.write((char*)&this->PidClientFlags_len, 2);
        if (this->PidClientFlags_len > 0 && this->PidClientFlags_len != 0xffff)
            s.write(this->PidClientFlags, this->PidClientFlags_len);
        s.write((char*)&this->ClientIP, 4);
        s.write((char*)&this->ClientInfo_len, 2);
        if (this->ClientInfo_len > 0 && this->ClientInfo_len != 0xffff)
            s.write(this->ClientInfo, this->ClientInfo_len);
    }
};