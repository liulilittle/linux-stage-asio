#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class FeedbackAccountRequest;

class FeedbackAccountRequest {
private:
    unsigned char AppCategory;
    const char* AccountId;
    unsigned short AccountId_len;
    const char* RoleId;
    unsigned short RoleId_len;
    const char* ReportContent;
    unsigned short ReportContent_len;

public:
    FeedbackAccountRequest() {
        this->AppCategory = 0;
        this->AccountId = NULL;
        this->AccountId_len = -1;
        this->RoleId = NULL;
        this->RoleId_len = -1;
        this->ReportContent = NULL;
        this->ReportContent_len = -1;
    }
    ~FeedbackAccountRequest() {
        const char* sz = NULL;
        sz = this->AccountId;
        this->AccountId = NULL;
        this->AccountId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->RoleId;
        this->RoleId = NULL;
        this->RoleId_len = -1;
        if (sz != NULL) delete sz;
        sz = this->ReportContent;
        this->ReportContent = NULL;
        this->ReportContent_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    unsigned char GetAppCategory() {
        return this->AppCategory;
    };
    const char* GetAccountId() {
        if (this->AccountId_len == 0xffff) return NULL;
        else if (this->AccountId_len == 0) return "";
        return this->AccountId;

    };
    int GetAccountIdLength() {
        return ((this->AccountId_len == 0xffff) ? -1 : this->AccountId_len);
    };
    const char* GetRoleId() {
        if (this->RoleId_len == 0xffff) return NULL;
        else if (this->RoleId_len == 0) return "";
        return this->RoleId;

    };
    int GetRoleIdLength() {
        return ((this->RoleId_len == 0xffff) ? -1 : this->RoleId_len);
    };
    const char* GetReportContent() {
        if (this->ReportContent_len == 0xffff) return NULL;
        else if (this->ReportContent_len == 0) return "";
        return this->ReportContent;

    };
    int GetReportContentLength() {
        return ((this->ReportContent_len == 0xffff) ? -1 : this->ReportContent_len);
    };

public:
    void SetAppCategory(unsigned char value) {
        this->AppCategory = value;
    };
    void SetAccountId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->AccountId_len == 0 && len == 0) return;
        if (this->AccountId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->AccountId) delete[] this->AccountId;
        this->AccountId = NULL;
        this->AccountId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->AccountId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->AccountId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->AccountId_len = len;
    };
    void SetRoleId(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->RoleId_len == 0 && len == 0) return;
        if (this->RoleId_len == 0xffff && len == 0xffff) return;
        if (NULL != this->RoleId) delete[] this->RoleId;
        this->RoleId = NULL;
        this->RoleId_len = 0xffff;
        if (len > 0 && len != 0xffff) this->RoleId = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->RoleId;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->RoleId_len = len;
    };
    void SetReportContent(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->ReportContent_len == 0 && len == 0) return;
        if (this->ReportContent_len == 0xffff && len == 0xffff) return;
        if (NULL != this->ReportContent) delete[] this->ReportContent;
        this->ReportContent = NULL;
        this->ReportContent_len = 0xffff;
        if (len > 0 && len != 0xffff) this->ReportContent = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->ReportContent;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->ReportContent_len = len;
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
            if (len_ == 0xffff) this->SetAccountId(NULL, 0xffff);
            else if (len_ == 0) this->SetAccountId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetAccountId((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetRoleId(NULL, 0xffff);
            else if (len_ == 0) this->SetRoleId("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetRoleId((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetReportContent(NULL, 0xffff);
            else if (len_ == 0) this->SetReportContent("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetReportContent((char*)buf, len_);
                buf += len_;
            }
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.put((char)this->AppCategory);
        s.write((char*)&this->AccountId_len, 2);
        if (this->AccountId_len > 0 && this->AccountId_len != 0xffff)
            s.write(this->AccountId, this->AccountId_len);
        s.write((char*)&this->RoleId_len, 2);
        if (this->RoleId_len > 0 && this->RoleId_len != 0xffff)
            s.write(this->RoleId, this->RoleId_len);
        s.write((char*)&this->ReportContent_len, 2);
        if (this->ReportContent_len > 0 && this->ReportContent_len != 0xffff)
            s.write(this->ReportContent, this->ReportContent_len);
    }
};
