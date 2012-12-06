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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>

#include "logging.h"
#include "util.h"

static int loglevel = LOGLEVEL_INFO;

void log_init(void)
{
}

void log_setlevel(int level)
{
    loglevel = level;
}

void log_msg(int level, const char *func, const char *format, ...)
{
    va_list args;
    const char *timeformat = "%Y-%m-%dT%H:%M:%S";
    char timestr[256];
    time_t t;

    t = time(NULL);
    strftime(timestr, sizeof(timestr), timeformat, gmtime(&t));


    if (loglevel >= level || level == LOGLEVEL_CRITICAL)
    {
        if (level == LOGLEVEL_DEBUG)
            fprintf(stderr, "[%s()] ", func);
        va_start(args, format);
        fprintf(stderr, "[%s] ", timestr);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\r\n");
        va_end(args);
    }

    if (level == LOGLEVEL_CRITICAL)
        exit(1);
}

void log_msg_noln(int level, const char *func, const char *format, ...)
{
    va_list args;

    if (loglevel >= level || loglevel == LOGLEVEL_CRITICAL)
    {
        if (level == LOGLEVEL_DEBUG)
            fprintf(stderr, "[%s()] ", func);
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }

    if (level == LOGLEVEL_CRITICAL)
        exit(1);
}

