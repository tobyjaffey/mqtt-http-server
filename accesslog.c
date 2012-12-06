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
#include <time.h>

#include "accesslog.h"

#define ACCESSLOG_FILENAME "access.log"
static FILE *fp;

int accesslog_init(void)
{
    if (NULL == (fp = fopen(ACCESSLOG_FILENAME, "a")))
        return 1;
    return 0;
}

void accesslog_write(const char *format, ...)
{
    va_list args;
    const char *timeformat = "%Y-%m-%dT%H:%M:%S";
    char timestr[256];
    time_t t;

    t = time(NULL);
    strftime(timestr, sizeof(timestr), timeformat, gmtime(&t));

    va_start(args, format);
    fprintf(fp, "[%s] ", timestr);
    vfprintf(fp, format, args);
    fprintf(fp, "\r\n");
    va_end(args);

    fflush(fp);
}

