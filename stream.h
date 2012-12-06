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
#ifndef STREAM_H
#define STREAM_H 1

#include "topic.h"
#include "tailq.h"

#define DEFAULT_STREAM_TIMEOUT_S 30
#define STREAM_SECKEY_SIZE 16

typedef struct
{
    uint32_t streamid;
    topic_node_t *topic_patt;
    char *topicstr;
    tailq_t *rsp_q;
    bool have_connid;
    uint32_t connid;
    uint32_t timeoutS;
    uint8_t seckey[STREAM_SECKEY_SIZE];
} stream_t;


typedef void(*stream_drain_func)(stream_t *stream, const char *str, bool first, void *userdata);

extern stream_t *stream_create(const char *topic_patt_str);
extern void stream_destroy(stream_t *stream);
extern int stream_push(stream_t *stream, char *str);
extern void stream_drain(stream_t *stream, stream_drain_func f, void *userdata);
extern bool stream_isempty(stream_t *stream);
extern void stream_set_connection(stream_t *stream, uint32_t connid);
extern bool stream_get_connection(stream_t *stream, uint32_t *connid);
extern void stream_clear_connection(stream_t *stream);
extern bool stream_1hz(uint32_t streamid, stream_t *s, void *userdata); // return true to destroy
extern void stream_refresh_timer(stream_t *stream);


#endif
