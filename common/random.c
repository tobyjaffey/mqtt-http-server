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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ev.h>
#include <stddef.h>

#include <common/logging.h>
#include <common/util.h>
#include <common/random.h>

static int random_fd = -1;

int random_init(void)
{
    random_fd = open("/dev/urandom", O_RDONLY);
    if (random_fd < 0)
        return 1;
    return 0;
}

void random_read(uint8_t *buf, size_t len)
{
#if 1
    if (random_fd < 0)
        LOG_CRITICAL("random_init failed/not called");

    if (len != read(random_fd, buf, len))
        LOG_CRITICAL("random_read failed");
#else
#warning BUST RANDOM
    while(len--)
        *buf++ = 4;
#endif
}

