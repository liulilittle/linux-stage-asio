#include "DateTime.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <string>
#include <string.h>

DateTime DateTime::ToUtc()
{
    return this->AddSeconds(-GetGMTOffset());
}

DateTime DateTime::ToLocal()
{
    return this->AddSeconds(GetGMTOffset());
}

static double DateTimeTicksRound(double number, unsigned int bits = 0)
{
    long long integerPart = number;
    number -= integerPart;
    for (unsigned int i = 0; i < bits; ++i)
        number *= 10;
    number = (long long)(number + 0.5);
    for (unsigned int i = 0; i < bits; ++i)
        number /= 10;
    return integerPart + number;
}

DateTime DateTime::Now()
{
    int gmtoff = GetGMTOffset();

    DateTime dateTime = UtcNow();
    if (0 != gmtoff)
    {
        dateTime = dateTime.AddSeconds(gmtoff);
    }

    return dateTime;
}

DateTime DateTime::UtcNow()
{
    struct timespec ts = { 0, 0 };

    clock_gettime(CLOCK_REALTIME, &ts);

    long long sec = (long long)ts.tv_sec;
    long long nsec = (long long)ts.tv_nsec;
    long long nsec_100 = DateTimeTicksRound((double)nsec / 100);

    DateTime dateTime = DateTime(1970, 1, 1, 0, 0, 0);

    long long ticks = sec * 10000000;
    ticks += nsec_100;
    ticks += dateTime.Ticks();

    return DateTime(ticks);
}

int DateTime::GetGMTOffset(bool abs)
{
    static int gmtoff = 0;
    static char geted = 0;
    if (abs || 0 == geted)
    {
        time_t rawtime;
        struct tm* ptm;

        time(&rawtime);

        ptm = gmtime(&rawtime);
        gmtoff = ptm->tm_gmtoff;

        if (0 == gmtoff) // 获取当前时区与中心时区之间（TimeZoneInfo）的时差；例：中国GMT/CST（东八区+ UTC-8）
        {
            ptm = localtime(&rawtime);
            gmtoff = ptm->tm_gmtoff;
        }

        geted = 1;
    }

    return gmtoff;
}

bool DateTime::TryParse(const char* s, int len, DateTime& out)
{
    out = MinValue();
    if (s == NULL && len != 0)
    {
        return false;
    }
    if (s != NULL && len == 0)
    {
        return false;
    }
    if (len < 0)
    {
        len = (int)strlen(s);
    }
    if (len <= 0)
    {
        return false;
    }
    static const int max_segments_length = 7;
    std::string segments[max_segments_length + 1];

    const char* p = s;
    unsigned int length = 0;
    while (p < (s + len) && *p != '\x0')
    {
        char ch = *p;
        if (ch >= '0' && ch <= '9')
        {
            char buf[2] = { ch, '\x0' };
            segments[length] += buf;
        }
        else
        {
            if (!segments[length].empty())
            {
                length++;
                if (length >= max_segments_length)
                {
                    break;
                }
                segments[length].empty();
            }
        }
        p++;
    }

    struct
    {
        int y;
        int M;
        int d;
        int H;
        int m;
        int s;
        int f;
    } tm;

    if (0 == length)
    {
        return false;
    }
    else
    {
        int* t = (int*)&tm;
        for (unsigned int i = 1; i <= max_segments_length; i++)
        {
            if (i > (length + 1))
            {
                t[i - 1] = 0;
            }
            else
            {
                std::string& sx = segments[i - 1];
                if (sx.empty())
                {
                    t[i - 1] = 0;
                }
                else
                {
                    t[i - 1] = atoi(sx.c_str());
                }
            }
        }
        out = DateTime(tm.y, tm.M, tm.d, tm.H, tm.m, tm.s, tm.f);
    }
    return length > 0;
}

std::string DateTime::ToString(const char* format, int len)
{
    if (format == NULL && len != 0)
    {
        return "";
    }
    if (format != NULL && len == 0)
    {
        return "";
    }
    if (len < 0)
    {
        len = (int)strlen(format);
    }
    if (len <= 0)
    {
        return "";
    }

#define DATETIME_FORMAT_PROPERTY_TOSTRING(k, v) \
{ \
        { \
            int n = 0; \
            while ( (p + n) < e && p[n] == *(#k) ) n++; \
            if ( n > 0 ) \
            { \
                char buf[ 255 ]; \
                char fmt[ 50 ]; \
                fmt[ 0 ] = '%'; \
                fmt[ 1 ] = '0'; \
                sprintf( fmt + 2, "%dlld", n ); \
                sprintf( buf, fmt, (long long int)v ); \
                s += buf; \
                p += n; \
            } \
        } \
    DATETIME_FORMAT_DIVIDER_TOSTRING(); \
}

#define DATETIME_FORMAT_DIVIDER_TOSTRING() \
    { \
            const char* fb = "yMdHmsfu"; \
            char buf[ 50 ]; \
            int n = 0; \
            while ( ( p + n ) < e && n < (int)(sizeof(buf) - 1)) { \
                char ch = p[ n ]; \
                if ( ch == '\x0' ) break; \
                bool fx = false; \
                for ( int i = 0; i < 8; i++ ) { \
                    if ( ch == fb[ i ] ) { \
                        fx = true; \
                        break; \
                    } \
                } \
                if ( fx ) break; else buf[n++] = ch; \
            } \
            buf[ n ] = '\x0'; \
            if ( buf[ 0 ] != '\x0' ) { \
                s += buf; \
                p += n; \
            } \
    }

    std::string s = "";

    const char* p = format;
    const char* e = p + len;
    while (p < e && *p != '\x0')
    {
        DATETIME_FORMAT_PROPERTY_TOSTRING(y, Year());
        DATETIME_FORMAT_PROPERTY_TOSTRING(M, Month());
        DATETIME_FORMAT_PROPERTY_TOSTRING(d, Day());
        DATETIME_FORMAT_PROPERTY_TOSTRING(H, Hour());
        DATETIME_FORMAT_PROPERTY_TOSTRING(m, Minute());
        DATETIME_FORMAT_PROPERTY_TOSTRING(s, Second());
        DATETIME_FORMAT_PROPERTY_TOSTRING(f, Millisecond());
        DATETIME_FORMAT_PROPERTY_TOSTRING(u, Microseconds());

        p++;
    }

#undef DATETIME_FORMAT_DIVIDER_TOSTRING
#undef DATETIME_FORMAT_PROPERTY_TOSTRING

    return s;
}
