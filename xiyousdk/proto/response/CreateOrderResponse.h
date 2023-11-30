#pragma once /*SSX: BinaryFormatter*/ 

#include <string>
#include <vector>
#include <strstream>

class CreateOrderResponse;

class CreateOrderResponse {
private:
    const char* OrderNo;
    unsigned short OrderNo_len;
    const char* Extension;
    unsigned short Extension_len;
    int Flag;

public:
    CreateOrderResponse() {
        this->OrderNo = NULL;
        this->OrderNo_len = -1;
        this->Extension = NULL;
        this->Extension_len = -1;
        this->Flag = 0;
    }
    ~CreateOrderResponse() {
        const char* sz = NULL;
        sz = this->OrderNo;
        this->OrderNo = NULL;
        this->OrderNo_len = -1;
        if (sz != NULL) delete sz;
        sz = this->Extension;
        this->Extension = NULL;
        this->Extension_len = -1;
        if (sz != NULL) delete sz;
    }

public:
    const char* GetOrderNo() {
        if (this->OrderNo_len == 0xffff) return NULL;
        else if (this->OrderNo_len == 0) return "";
        return this->OrderNo;

    };
    int GetOrderNoLength() {
        return ((this->OrderNo_len == 0xffff) ? -1 : this->OrderNo_len);
    };
    const char* GetExtension() {
        if (this->Extension_len == 0xffff) return NULL;
        else if (this->Extension_len == 0) return "";
        return this->Extension;

    };
    int GetExtensionLength() {
        return ((this->Extension_len == 0xffff) ? -1 : this->Extension_len);
    };
    int GetFlag() {
        return this->Flag;
    };

public:
    void SetOrderNo(const char* s, unsigned short len = 0xffff) {
        if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
        if (this->OrderNo_len == 0 && len == 0) return;
        if (this->OrderNo_len == 0xffff && len == 0xffff) return;
        if (NULL != this->OrderNo) delete[] this->OrderNo;
        this->OrderNo = NULL;
        this->OrderNo_len = 0xffff;
        if (len > 0 && len != 0xffff) this->OrderNo = new char[len + 1];
        if (NULL != s && (len > 0 && len != 0xffff)) {
            char* sz = (char*)this->OrderNo;
            memcpy(sz, (char*)s, len);
            sz[len] = '\x0';
        }
        this->OrderNo_len = len;
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
    void SetFlag(int value) {
        this->Flag = value;
    };

public:
    bool Deserialize(unsigned char*& buf, int& count) {
        if ((count -= 1) < 0) return false;
        if (*buf++ != 0) return false;
        {
            if ((count -= 2) < 0) return false;
            unsigned short len_ = *(unsigned short*)buf;
            buf += 2;
            if (len_ == 0xffff) this->SetOrderNo(NULL, 0xffff);
            else if (len_ == 0) this->SetOrderNo("", 0);
            else {
                if ((count -= len_) < 0) return false;
                this->SetOrderNo((char*)buf, len_);
                buf += len_;
            }
        }
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
        if ((count -= 4) < 0) return false;
        this->Flag = *(int*)buf;
        buf += 4;
        return true;
    }
    void Serialize(std::strstream& s) {
        s.put(NULL == this);
        s.write((char*)&this->OrderNo_len, 2);
        if (this->OrderNo_len > 0 && this->OrderNo_len != 0xffff)
            s.write(this->OrderNo, this->OrderNo_len);
        s.write((char*)&this->Extension_len, 2);
        if (this->Extension_len > 0 && this->Extension_len != 0xffff)
            s.write(this->Extension, this->Extension_len);
        s.write((char*)&this->Flag, 4);
    }
};
