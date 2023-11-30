#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

template<typename T>
class GenericResponse;

template<>
class GenericResponse<const char*> {
private:
    int Code;
    const char* Message;
    unsigned short Message_len;
    const char* Tag;
    unsigned short Tag_len;

public:
    GenericResponse() {
        this->Code = 0;
        this->Message = NULL;
        this->Message_len = -1;
        this->Tag = NULL;
        this->Tag_len = -1;
    }
    ~GenericResponse() {
        const char* sz = NULL;
        sz = this->Message;
        this->Message = NULL;
        this->Message_len = -1;
        if (sz != NULL) delete sz;
        sz = this->Tag;
        this->Tag = NULL;
        this->Tag_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    int GetCode() {
        return this->Code;
    };
    const char* GetMessage() {
        if (this->Message_len == 0xffff) return NULL;
        else if (this->Message_len == 0) return "";
        return this->Message;

    };
    int GetMessageLength() {
        return ((this->Message_len == 0xffff) ? -1 : this->Message_len);
    };
    const char* GetTag() {
        if (this->Tag_len == 0xffff) return NULL;
        else if (this->Tag_len == 0) return "";
        return this->Tag;

    };
    int GetTagLength() {
        return ((this->Tag_len == 0xffff) ? -1 : this->Tag_len);
    };

public:
    void SetCode(int value) {
        this->Code = value;
    };
    void SetMessage(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->Message_len == 0 && len == 0) return;
        if (this->Message_len == 0xffff && len == 0xffff) return;
        if (NULL != this->Message) delete[] this->Message;
        this->Message = NULL;
        this->Message_len = 0xffff;
        if (len > 0 && len != 0xffff) this->Message = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->Message;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->Message_len = len;
    };
    void SetTag(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->Tag_len == 0 && len == 0) return;
        if (this->Tag_len == 0xffff && len == 0xffff) return;
        if (NULL != this->Tag) delete[] this->Tag;
        this->Tag = NULL;
        this->Tag_len = 0xffff;
        if (len > 0 && len != 0xffff) this->Tag = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->Tag;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->Tag_len = len;
    };

public:
    bool Deserialize(unsigned char*& buf, int& count) {
        if ((count -= 1) < 0) return false;
        if (*buf++ != 0) return false;
        if ((count -= 4) < 0) return false;
        this->Code = *(int*)buf;
        buf += 4;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetMessage(NULL, 0xffff);
            else if (len_ == 0) this->SetMessage("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetMessage((char*)buf, len_);
                buf += len_;
            }
        }
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetTag(NULL, 0xffff);
            else if (len_ == 0) this->SetTag("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetTag((char*)buf, len_);
                buf += len_;
            }
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->Code, 4);
        s.write((char*)&this->Message_len, 2);
        if (this->Message_len > 0 && this->Message_len != 0xffff)
            s.write(this->Message, this->Message_len);
        s.write((char*)&this->Tag_len, 2);
        if (this->Tag_len > 0 && this->Tag_len != 0xffff)
            s.write(this->Tag, this->Tag_len);
    }
};
