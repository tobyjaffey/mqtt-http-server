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
#ifndef mqtthttpd_H
#define mqtthttpd_H 1

#include <ev.h>

#include "mqtt.h"
#include "httpd.h"

struct server_context
{
    struct ev_loop *loop;

    ev_timer *timer_1hz;
    ev_timer *timer_10hz;
    struct ev_idle *idle_watcher;

    mqtt_context_t mqttctx;
    char mqtt_server[512];
    uint16_t mqtt_port;
    char mqtt_name[256];

    ebb_server httpd;
    char *httpd_root;
    httpd_virt vhttpd[MAX_VHOST];
    uint32_t num_vhost;

    uint32_t httpdconnid;   // global ids
    uint32_t streamid;      // global ids
};
typedef struct server_context server_context_t;


int mqtthttpd_listen(int port);
void mqtthttpd_start_sleep(int t);
int mqtthttpd_run_script(char *filename);

#endif

