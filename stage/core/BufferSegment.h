#pragma once

#include "Object.h"

namespace ep
{
    template<typename T>
    class TBufferSegment
    {
    public:
        inline TBufferSegment()
            : Offset(0)
            , Length(0) {
            Buffer = make_shared_null<T>();
        }
        inline TBufferSegment(const std::shared_ptr<T>& buffer, int offset, int length)
            : Offset(0)
            , Length(0) {
            Reset(buffer, offset, length);
        }

    public:
        std::shared_ptr<T>												                                Buffer;
        int																	                            Offset;
        int																	                            Length;

    public:
        inline TBufferSegment*                                                                          GetNativePointer() { return this; }
        inline T*														                                UnsafeAddrOfPinnedArray() {
            if (NULL == this) {
                return NULL;
            }

            if (this->Length <= 0) {
                return NULL;
            }

            if (this->Offset < 0) {
                return NULL;
            }

            if (Object::IsNulll(this->Buffer)) {
                return NULL;
            }

            T* buffer = Object::addressof(this->Buffer);
            if (NULL == buffer) {
                return NULL;
            }

            T* stream_ptr = buffer + this->Offset;
            return stream_ptr;
        }
        inline int																	                    CopyTo(const T* buffer, int length) {
            if (NULL == this) {
                return ~0;
            }

            if (Offset < 0) {
                return ~1;
            }

            if (length < 0) {
                return ~2;
            }

            if (buffer == NULL && length != 0) {
                return ~3;
            }

            T* dst = const_cast<T*>(buffer);
            T* src = UnsafeAddrOfPinnedArray();
       
            int max = Length;
            if (length < max) {
                max = length;
            }

            if (max < 0) {
                return ~4;
            }

            if (src == NULL && max != 0) {
                return ~5;
            }

            for (int i = 0; i < max; i++) {
                dst[i] = src[i];
            }
            return max;
        }
        inline bool                                                                                     Clear() {
            if (NULL == this) {
                return false;
            }

            Offset = 0;
            Length = 0;
            Buffer = make_shared_null<T>();
            return true;
        }
        inline bool																		                Reset(const std::shared_ptr<T>& buffer, int offset, int length) {
            if (NULL == this) {
                return false;
            }

            if (offset < 0) {
                throw std::runtime_error("offset is not allowed to be less than 0");
            }

            if (length < 0) {
                throw std::runtime_error("length is not allowed to be less than 0");
            }

            const_cast<int&>(Offset) = offset;
            const_cast<int&>(Length) = length;
            if (0 != length) {
                Buffer = buffer;
            }
            else {
                Buffer = make_shared_null<T>();
            }
            return true;
        }
    };

    class BufferSegment : public TBufferSegment<unsigned char>
    {
    public:
        inline BufferSegment()
            : TBufferSegment() {
        }
        inline BufferSegment(const std::shared_ptr<unsigned char>& buffer, int offset, int length)
            : TBufferSegment(buffer, offset, length) {
        }
    };
}