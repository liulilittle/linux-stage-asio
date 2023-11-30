#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class LoginAccountResponse;

class XiYouGameServer;

class LoginAccountResponse {
private:
    const char* UserId;
    unsigned short UserId_len;
    const char* UserName;
    unsigned short UserName_len;
    const char* ChannelId;
    unsigned short ChannelId_len;
    const char* ChannelUserId;
    unsigned short ChannelUserId_len;
    XiYouGameServer* DefaultServer;
    std::vector<XiYouGameServer*>* LoggedServers;

public:
    LoginAccountResponse() {
        this->UserId = NULL;
        this->UserId_len = -1;
        this->UserName = NULL;
        this->UserName_len = -1;
        this->ChannelId = NULL;
        this->ChannelId_len = -1;
        this->ChannelUserId = NULL;
        this->ChannelUserId_len = -1;
        this->DefaultServer = NULL;
        this->LoggedServers = NULL;
    }
    ~LoginAccountResponse() {
        const char* sz = NULL;
        sz = this->UserId;
        this->UserId = NULL;
        this->UserId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->UserName;
        this->UserName = NULL;
        this->UserName_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ChannelId;
        this->ChannelId = NULL;
        this->ChannelId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ChannelUserId;
        this->ChannelUserId = NULL;
        this->ChannelUserId_len = -1;
        if (sz != NULL) delete sz;
        this->SetDefaultServer(NULL);
        this->SetLoggedServers(NULL);
    }

public:
    const char* GetUserId() {
        if (this->UserId_len == 0xffff) return NULL;
        else if (this->UserId_len == 0) return "";
        return this->UserId;

    };
    int GetUserIdLength() {
        return ((this->UserId_len == 0xffff) ? -1 : this->UserId_len);
    };
    const char* GetUserName() {
        if (this->UserName_len == 0xffff) return NULL;
        else if (this->UserName_len == 0) return "";
        return this->UserName;

    };
    int GetUserNameLength() {
        return ((this->UserName_len == 0xffff) ? -1 : this->UserName_len);
    };
    const char* GetChannelId() {
        if (this->ChannelId_len == 0xffff) return NULL;
        else if (this->ChannelId_len == 0) return "";
        return this->ChannelId;

    };
    int GetChannelIdLength() {
        return ((this->ChannelId_len == 0xffff) ? -1 : this->ChannelId_len);
    };
    const char* GetChannelUserId() {
        if (this->ChannelUserId_len == 0xffff) return NULL;
        else if (this->ChannelUserId_len == 0) return "";
        return this->ChannelUserId;

    };
    int GetChannelUserIdLength() {
        return ((this->ChannelUserId_len == 0xffff) ? -1 : this->ChannelUserId_len);
    };
    XiYouGameServer* GetDefaultServer() {
        return this->DefaultServer;
    }
    XiYouGameServer* MutableDefaultServer() {
        if (this->DefaultServer == NULL) this->DefaultServer = new XiYouGameServer();
        return this->DefaultServer;
    }
    std::vector<XiYouGameServer*>* GetLoggedServers() {
        return this->LoggedServers;
    }
    std::vector<XiYouGameServer*>* MutableLoggedServers() {
        if (this->LoggedServers == NULL) this->LoggedServers = new std::vector<XiYouGameServer*>();
        return this->LoggedServers;
    }

public:
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
    void SetUserName(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->UserName_len == 0 && len == 0) return;
        if (this->UserName_len == 0xffff && len == 0xffff) return;
        if (NULL != this->UserName) delete[] this->UserName;
        this->UserName = NULL;
        this->UserName_len = 0xffff;
        if (len > 0 && len != 0xffff) this->UserName = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->UserName;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->UserName_len = len;
    };
    void SetChannelId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ChannelId_len == 0 && len == 0) return;
        if (this->ChannelId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ChannelId) delete[] this->ChannelId;
        this->ChannelId = NULL;
        this->ChannelId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ChannelId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ChannelId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ChannelId_len = len;
    };
    void SetChannelUserId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ChannelUserId_len == 0 && len == 0) return;
        if (this->ChannelUserId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ChannelUserId) delete[] this->ChannelUserId;
        this->ChannelUserId = NULL;
        this->ChannelUserId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ChannelUserId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ChannelUserId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ChannelUserId_len = len;
    };
    void SetDefaultServer(XiYouGameServer* value) {
        XiYouGameServer* DefaultServer_ptr = this->DefaultServer;
        if (value == DefaultServer_ptr) return;
        if (DefaultServer_ptr != NULL) {
            this->DefaultServer = NULL;
            delete DefaultServer_ptr;
        }
        this->DefaultServer = value;
    }
    void SetLoggedServers(std::vector<XiYouGameServer*>* value) {
        std::vector<XiYouGameServer*>* LoggedServers_ptr = this->LoggedServers;
        if (value == LoggedServers_ptr) return;
        if (LoggedServers_ptr != NULL) {
            this->LoggedServers = NULL;
            unsigned int len_ = 0;
            if (LoggedServers_ptr != NULL) len_ = (unsigned int)LoggedServers_ptr->size();
            for (unsigned int idx_ = 0; idx_ < len_; idx_++) {
                XiYouGameServer* item_x_ptr = (*LoggedServers_ptr)[idx_];
                if (NULL != item_x_ptr) delete item_x_ptr;
            }
            delete LoggedServers_ptr;
        }
        this->LoggedServers = value;
    }

public:
    bool Deserialize(unsigned char*& buf, int& count) {
        if ((count -= 1) < 0) return false;
        if (*buf++ != 0) return false;
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
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetUserName(NULL, 0xffff);
            else if (len_ == 0) this->SetUserName("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetUserName((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetChannelId(NULL, 0xffff);
            else if (len_ == 0) this->SetChannelId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetChannelId((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetChannelUserId(NULL, 0xffff);
            else if (len_ == 0) this->SetChannelUserId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetChannelUserId((char*)buf, len_);
                buf += len_;
            }
        }
        if ((count - 1) < 0) return false;
        if (*buf != 0) {
            buf++;
            if (NULL != this->DefaultServer) {
                delete this->DefaultServer;
            }
            this->DefaultServer = NULL;
        }
        else {
            if (NULL == this->DefaultServer) this->DefaultServer = new XiYouGameServer();
            this->DefaultServer->Deserialize(buf, count);
        }
        if ((count -= 2) < 0) return false;
        {
            unsigned short xlen_ = *(unsigned short*)buf;
            buf += 2;
            if (xlen_ == 0xffff) this->SetLoggedServers(NULL);
            else {
                this->SetLoggedServers(new std::vector<XiYouGameServer*>(xlen_));
                for (int idx_ = 0; idx_ < xlen_; idx_++) {
                    if ((count - 1) < 0) return false;
                    if (*buf != 0) {
                        buf++;
                        (*this->LoggedServers)[idx_] = NULL;
                        continue;
                    }
                    XiYouGameServer* LoggedServers_x_ptr = new XiYouGameServer();
                    (*this->LoggedServers)[idx_] = LoggedServers_x_ptr;
                    if (true != LoggedServers_x_ptr->Deserialize(buf, count)) return false;
                }
            }
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->UserId_len, 2);
        if (this->UserId_len > 0 && this->UserId_len != 0xffff)
            s.write(this->UserId, this->UserId_len);
        s.write((char*)&this->UserName_len, 2);
        if (this->UserName_len > 0 && this->UserName_len != 0xffff)
            s.write(this->UserName, this->UserName_len);
        s.write((char*)&this->ChannelId_len, 2);
        if (this->ChannelId_len > 0 && this->ChannelId_len != 0xffff)
            s.write(this->ChannelId, this->ChannelId_len);
        s.write((char*)&this->ChannelUserId_len, 2);
        if (this->ChannelUserId_len > 0 && this->ChannelUserId_len != 0xffff)
            s.write(this->ChannelUserId, this->ChannelUserId_len);
        if (NULL == this->DefaultServer)
            s.put(1);
        else
            this->DefaultServer->Serialize(s);
        {
            unsigned short len_ = 0xffff;
            if (NULL != this->LoggedServers) {
                unsigned int lsize_ = (unsigned int)this->LoggedServers->size();
                if (lsize_ >= 0xffff)
                    lsize_ = 0xfffe;
                len_ = lsize_;
            }
            s.write((char*)&len_, 2);
            if (len_ > 0 && len_ != 0xffff) {
                for (int idx_ = 0; idx_ < len_; idx_++) {
                    XiYouGameServer* item_x_ptr = (*this->LoggedServers)[idx_];
                    if (NULL == item_x_ptr)
                        s.put(1);
                    else
                        item_x_ptr->Serialize(s);
                }
            }
        }
    }
};
