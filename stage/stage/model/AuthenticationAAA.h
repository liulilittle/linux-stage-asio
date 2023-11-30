#pragma once /*SSX: BinaryFormatter 1.0.1*/ 

#include <string>
#include <vector>
#include <strstream>

namespace ep
{
    namespace stage
    {
        namespace model
        {
            class AuthenticationAAAResponse;
            class AuthenticationAAARequest;

            enum Error;

            class AuthenticationAAARequest {
            private:
                unsigned short _svrAreaNo;
                unsigned short _serverNo;
                const char* _platform;
                unsigned short _platform_len;
                unsigned char _linkType;
                unsigned short _maxFD;

            public:
                AuthenticationAAARequest() {
                    this->_svrAreaNo = 0;
                    this->_serverNo = 0;
                    this->_platform = NULL;
                    this->_platform_len = -1;
                    this->_linkType = 0;
                    this->_maxFD = 0;
                }
                ~AuthenticationAAARequest() {
                    const char* sz = NULL;
                    sz = this->_platform;
                    this->_platform = NULL;
                    this->_platform_len = -1;
                    if (sz != NULL) delete sz;
                }

            public:
                unsigned short GetSvrAreaNo() {
                    return this->_svrAreaNo;
                };
                unsigned short GetServerNo() {
                    return this->_serverNo;
                };
                const char* GetPlatform() {
                    if (this->_platform_len == 0xffff) return NULL;
                    else if (this->_platform_len == 0) return "";
                    return this->_platform;

                };
                int GetPlatformLength() {
                    return ((this->_platform_len == 0xffff) ? -1 : this->_platform_len);
                };
                unsigned char GetLinkType() {
                    return this->_linkType;
                };
                unsigned short GetMaxFD() {
                    return this->_maxFD;
                };

            public:
                void SetSvrAreaNo(unsigned short value) {
                    this->_svrAreaNo = value;
                };
                void SetServerNo(unsigned short value) {
                    this->_serverNo = value;
                };
                void SetPlatform(const char* s, unsigned short len = 0xffff) {
                    if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
                    if (this->_platform_len == 0 && len == 0) return;
                    if (this->_platform_len == 0xffff && len == 0xffff) return;
                    if (NULL != this->_platform) delete[] this->_platform;
                    this->_platform = NULL;
                    this->_platform_len = 0xffff;
                    if (len > 0 && len != 0xffff) this->_platform = new char[len + 1];
                    if (NULL != s && (len > 0 && len != 0xffff)) {
                        char* sz = (char*)this->_platform;
                        memcpy(sz, (char*)s, len);
                        sz[len] = '\x0';
                    }
                    this->_platform_len = len;
                };
                void SetLinkType(unsigned char value) {
                    this->_linkType = value;
                };
                void SetMaxFD(unsigned short value) {
                    this->_maxFD = value;
                };

            public:
                bool Deserialize(unsigned char*& buf, int& count) {
                    if ((count -= 1) < 0) return false;
                    if (*buf++ != 0) return false;
                    if ((count -= 2) < 0) return false;
                    this->_svrAreaNo = *(unsigned short*)buf;
                    buf += 2;
                    if ((count -= 2) < 0) return false;
                    this->_serverNo = *(unsigned short*)buf;
                    buf += 2;
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
                    if ((count -= 1) < 0) return false;
                    this->_linkType = *(unsigned char*)buf;
                    buf++;
                    if ((count -= 2) < 0) return false;
                    this->_maxFD = *(unsigned short*)buf;
                    buf += 2;
                    return true;
                }
                void Serialize(std::strstream& s) {
                    s.put(NULL == this);
                    s.write((char*)&this->_svrAreaNo, 2);
                    s.write((char*)&this->_serverNo, 2);
                    s.write((char*)&this->_platform_len, 2);
                    if (this->_platform_len > 0 && this->_platform_len != 0xffff)
                        s.write(this->_platform, this->_platform_len);
                    s.put((char)this->_linkType);
                    s.write((char*)&this->_maxFD, 2);
                }
            };

            class AuthenticationAAAResponse {
            private:
                unsigned short _code;
                unsigned char _subtract;
                unsigned char _modVt;
                const char* _key;
                unsigned short _key_len;
                unsigned int _linkNo;
                unsigned char _linkType;

            public:
                AuthenticationAAAResponse() {
                    this->_code = 0;
                    this->_subtract = 0;
                    this->_modVt = 0;
                    this->_key = NULL;
                    this->_key_len = -1;
                    this->_linkNo = 0;
                    this->_linkType = 0;
                }
                ~AuthenticationAAAResponse() {
                    const char* sz = NULL;
                    sz = this->_key;
                    this->_key = NULL;
                    this->_key_len = -1;
                    if (sz != NULL) delete sz;
                }

            public:
                Error GetCode() {
                    return (Error)this->_code;
                };
                unsigned char GetSubtract() {
                    return this->_subtract;
                };
                unsigned char GetModVt() {
                    return this->_modVt;
                };
                const char* GetKey() {
                    if (this->_key_len == 0xffff) return NULL;
                    else if (this->_key_len == 0) return "";
                    return this->_key;

                };
                int GetKeyLength() {
                    return ((this->_key_len == 0xffff) ? -1 : this->_key_len);
                };
                unsigned int GetLinkNo() {
                    return this->_linkNo;
                };
                unsigned char GetLinkType() {
                    return this->_linkType;
                };

            public:
                void SetCode(Error value) {
                    this->_code = (unsigned short)value;
                };
                void SetSubtract(unsigned char value) {
                    this->_subtract = value;
                };
                void SetModVt(unsigned char value) {
                    this->_modVt = value;
                };
                void SetKey(const char* s, unsigned short len = 0xffff) {
                    if (len == 0xffff && s != NULL) len = (unsigned short)strlen(s);
                    if (this->_key_len == 0 && len == 0) return;
                    if (this->_key_len == 0xffff && len == 0xffff) return;
                    if (NULL != this->_key) delete[] this->_key;
                    this->_key = NULL;
                    this->_key_len = 0xffff;
                    if (len > 0 && len != 0xffff) this->_key = new char[len + 1];
                    if (NULL != s && (len > 0 && len != 0xffff)) {
                        char* sz = (char*)this->_key;
                        memcpy(sz, (char*)s, len);
                        sz[len] = '\x0';
                    }
                    this->_key_len = len;
                };
                void SetLinkNo(unsigned int value) {
                    this->_linkNo = value;
                };
                void SetLinkType(unsigned char value) {
                    this->_linkType = value;
                };

            public:
                bool Deserialize(unsigned char*& buf, int& count) {
                    if ((count -= 1) < 0) return false;
                    if (*buf++ != 0) return false;
                    if ((count -= 2) < 0) return false;
                    this->_code = *(unsigned short*)buf;
                    buf += 2;
                    if ((count -= 1) < 0) return false;
                    this->_subtract = *(unsigned char*)buf;
                    buf++;
                    if ((count -= 1) < 0) return false;
                    this->_modVt = *(unsigned char*)buf;
                    buf++;
                    {
                        if ((count -= 2) < 0) return false;
                        unsigned short len_ = *(unsigned short*)buf;
                        buf += 2;
                        if (len_ == 0xffff) this->SetKey(NULL, 0xffff);
                        else if (len_ == 0) this->SetKey("", 0);
                        else {
                            if ((count -= len_) < 0) return false;
                            this->SetKey((char*)buf, len_);
                            buf += len_;
                        }
                    }
                    if ((count -= 4) < 0) return false;
                    this->_linkNo = *(unsigned int*)buf;
                    buf += 4;
                    if ((count -= 1) < 0) return false;
                    this->_linkType = *(unsigned char*)buf;
                    buf++;
                    return true;
                }
                void Serialize(std::strstream& s) {
                    s.put(NULL == this);
                    s.write((char*)&this->_code, 2);
                    s.put((char)this->_subtract);
                    s.put((char)this->_modVt);
                    s.write((char*)&this->_key_len, 2);
                    if (this->_key_len > 0 && this->_key_len != 0xffff)
                        s.write(this->_key, this->_key_len);
                    s.write((char*)&this->_linkNo, 4);
                    s.put((char)this->_linkType);
                }
            };
        }
    }
}