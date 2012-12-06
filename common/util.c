/*
* Copyright (c) 2012, Toby Jaffey <toby@sensemote.com>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
//#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <common/logging.h>
#include <common/util.h>

void crash(void)
{
    // crash for benefit of debugger
    char *p = NULL;
    *p = 0;
}

time_t getTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

static uint8_t tolowercase(uint8_t ch)
{
    if ((ch >= 'A') && (ch <= 'Z'))
        return ch + 0x20;       // Convert uppercase to lowercase
    return ch;      // Simply return original character if it doesn't require any adjustments
}

static int8_t parseHexDigit(uint8_t digit)
{
    digit = tolowercase(digit);
    if (isdigit(digit))
        return (int8_t)digit - '0';
    if ((digit >= 'a') && (digit <= 'f'))
        return (int8_t)digit + 0xA - 'a';
    return -1;      // Error case - input wasn't a valid hex digit
}

int hexstring_parse(const char *hexstr, uint8_t *buf, size_t *buflen)
{
    size_t hexstrlen = strlen(hexstr);
    size_t i;

    if (hexstrlen & 0x1)
    {
        LOG_DEBUG("hexstring_parse: not even");
        return 1;
    }

    if (*buflen < hexstrlen/2)
    {
        LOG_DEBUG("hexstring_parse: buffer too small %d < %d", *buflen, hexstrlen/2);
        return 1;
    }

    for (i=0;i<hexstrlen;i+=2)
    {
        int8_t a, b;
        if (-1 == (a = parseHexDigit(hexstr[i])))
        {
            LOG_DEBUG("hexstring_parse: bad digit 0x%02X", hexstr[i]);
            return 1;
        }
        if (-1 == (b = parseHexDigit(hexstr[i+1])))
        {
            LOG_DEBUG("hexstring_parse: bad digit 0x%02X", hexstr[i+1]);
            return 1;
        }
        *buf++ = (a << 4) | b;
    }

    *buflen = hexstrlen/2;

    return 0;
}


