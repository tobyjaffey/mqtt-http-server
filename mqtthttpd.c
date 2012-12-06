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

#include "mqtthttpd.h"
#include <common/logging.h>
#include <common/util.h>
#include <common/jitter.h>
#include <common/pidfile.h>
#include <common/random.h>

#include "mqtt.h"
#include "httpd.h"
#include "stream.h"
#include "streamlist.h"
#include "accesslog.h"
#include "opt.h"

server_context_t g_srvctx;

void exitfunc(void)
{
}

void idle_cb(struct ev_loop *loop, struct ev_idle *w, int revents)
{
}

void timeout_1hz_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    if (EV_TIMEOUT & revents)
    {
        streamlist_foreach(stream_1hz, NULL);
    }
}

void timeout_10hz_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    if (EV_TIMEOUT & revents)
    {
    }
}


int main(int argc, char *argv[])
{
    if (0 == getuid())
        LOG_CRITICAL("Don't run as root!");

    memset(&g_srvctx, 0, sizeof(g_srvctx));

    signal(SIGPIPE, SIG_IGN);    // ignore sigpipe
    g_srvctx.loop = ev_default_loop(0);

    log_init();
    random_init();

    if (0 != accesslog_init())
        LOG_CRITICAL("Failed to open access log");
    if (NULL == (g_srvctx.timer_1hz = (struct ev_timer *)malloc(sizeof(struct ev_timer))))
        LOG_CRITICAL("out of mem");
    if (NULL == (g_srvctx.timer_10hz = (struct ev_timer *)malloc(sizeof(struct ev_timer))))
        LOG_CRITICAL("out of mem");
    if (NULL == (g_srvctx.idle_watcher = (struct ev_idle *)malloc(sizeof(struct ev_idle))))
        LOG_CRITICAL("out of mem");
    if (0 != pidfile_write("mqtthttpd.pid"))
        LOG_CRITICAL("pidfile write mqtthttpd.pid failed");

    ev_timer_init(g_srvctx.timer_1hz, (void *)timeout_1hz_cb, 0, 1);
    ev_set_priority(g_srvctx.timer_1hz, EV_MAXPRI);
    ev_timer_start(g_srvctx.loop, g_srvctx.timer_1hz);
    ev_timer_init(g_srvctx.timer_10hz, (void *)timeout_10hz_cb, 0, 0.1);
    ev_set_priority(g_srvctx.timer_10hz, EV_MAXPRI);
    ev_timer_start(g_srvctx.loop, g_srvctx.timer_10hz);
    ev_idle_init(g_srvctx.idle_watcher, idle_cb);

    if (0 != httpd_init(&(g_srvctx.httpd), g_srvctx.loop))
        LOG_CRITICAL("httpd_init failed");
    if (0 != mqtt_init(g_srvctx.loop, &(g_srvctx.mqttctx)))
        LOG_CRITICAL("mqtt_init failed");

    atexit(exitfunc);

    if (0 != parse_options(argc, argv))
    {
        usage();
        return 1;
    }

    // Start infinite loop
    while (1)
    {
        ev_loop(g_srvctx.loop, 0);
    }

    return 0;
}

