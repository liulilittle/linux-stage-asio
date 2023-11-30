#pragma once

#include <stdio.h>

namespace ep
{
    namespace cipher
    {
        void rc4_crypt(int mod, unsigned char* key, int keylen, unsigned char* data, int datalen, int subtract, int E);
    }
}