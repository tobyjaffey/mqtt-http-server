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
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <common/logging.h>
#include "jitter.h"

static bool jitter_enabled;
static struct timeval last;
static double last_delay_us = 0;

void jitter_enable(bool enabled)
{
    jitter_enabled = enabled;
}

void jitter_check(void)
{
    double jitter_us;
    double delay_us;
    struct timeval now;

    if (jitter_enabled)
    {
        gettimeofday(&now, NULL);

        delay_us = (now.tv_sec - last.tv_sec) * 1000000.0;
        delay_us += now.tv_usec - last.tv_usec;

        jitter_us = delay_us - last_delay_us;

        if (jitter_us >= 0)
            LOG_USER("jitter: +%5.4fms", jitter_us / 1000.0);
        else
            LOG_USER("jitter: %5.4fms", jitter_us / 1000.0);

        memcpy(&last, &now, sizeof(struct timeval));
        last_delay_us = delay_us;
    }
}
