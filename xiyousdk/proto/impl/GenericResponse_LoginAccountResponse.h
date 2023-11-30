#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

template<typename T>
class GenericResponse;

class LoginAccountResponse;

template<>
class GenericResponse<LoginAccountResponse> {
private:
    int Code;
    const char* Message;
    unsigned short Message_len;
    LoginAccountResponse* Tag;

public:
    GenericResponse() {
        this->Code = 0;
        this->Message = NULL;
        this->Message_len = -1;
        this->Tag = NULL;
    }
    ~GenericResponse() {
        const char* sz = NULL;
        sz = this->Message;
        this->Message = NULL;
        this->Message_len = -1;
        if (sz != NULL) delete sz;
        this->SetTag(NULL);
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
    LoginAccountResponse* GetTag() {
        return this->Tag;
    }
    LoginAccountResponse* MutableTag() {
        if (this->Tag == NULL) this->Tag = new LoginAccountResponse();
        return this->Tag;
    }

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
    void SetTag(LoginAccountResponse* value) {
        LoginAccountResponse* Tag_ptr = this->Tag;
        if (value == Tag_ptr) return;
        if (Tag_ptr != NULL) {
            this->Tag = NULL;
            delete Tag_ptr;
        }
        this->Tag = value;
    }

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
        if ((count - 1) < 0) return false;
        if (*buf != 0) {
            buf++;
            if (NULL != this->Tag) {
                delete this->Tag;
            }
            this->Tag = NULL;
        }
        else {
            if (NULL == this->Tag) this->Tag = new LoginAccountResponse();
            this->Tag->Deserialize(buf, count);
        }
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->Code, 4);
        s.write((char*)&this->Message_len, 2);
        if (this->Message_len > 0 && this->Message_len != 0xffff)
            s.write(this->Message, this->Message_len);
        if (NULL == this->Tag)
            s.put(1);
        else
            this->Tag->Serialize(s);
    }
};
