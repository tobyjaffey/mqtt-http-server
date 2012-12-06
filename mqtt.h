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
#ifndef MQTT_H
#define MQTT_H 1

#include "evhelper.h"

struct mqtt_context
{
    struct mosquitto *mosq;
    ev_io_helper_t read_watcher;
    ev_io_helper_t write_watcher;
    ev_timer_helper_t timer;
    bool connected;
};
typedef struct mqtt_context mqtt_context_t;

extern int mqtt_init(struct ev_loop *loop, mqtt_context_t *mqctx);
extern int mqtt_connect(mqtt_context_t *mqctx, const char *name, const char *host, uint16_t port);
extern int mqtt_disconnect(mqtt_context_t *mqctx);
extern int mqtt_publish(mqtt_context_t *mqctx, const char *topic, const char *msg, int qos, int *mid);
extern int mqtt_subscribe(mqtt_context_t *mqttctx, const char *topic, int qos, int *mid);
extern int mqtt_unsubscribe(mqtt_context_t *mqttctx, const char *topic, int *mid);

#endif

