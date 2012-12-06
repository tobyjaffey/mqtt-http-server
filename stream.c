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
#include <common/common.h>
#include <common/logging.h>
#include <common/random.h>

#include "mqtthttpd.h"
#include "stream.h"
#include "streamlist.h"
#include "topic.h"
#include "sublist.h"

extern server_context_t g_srvctx;

stream_t *stream_create(const char *topic_patt_str)
{
    stream_t *stream;

    if (NULL == (stream = (stream_t *)calloc(sizeof(stream_t), 1)))
        goto fail;
    if (NULL == (stream->topic_patt = topic_create_from_string(topic_patt_str)))
        goto fail;
    if (NULL == (stream->rsp_q = tailq_create()))
        goto fail;
    if (NULL == (stream->topicstr = strdup(topic_patt_str)))
        goto fail;
    stream->timeoutS = DEFAULT_STREAM_TIMEOUT_S;
    stream->streamid = g_srvctx.streamid++;

    random_read(stream->seckey, STREAM_SECKEY_SIZE);

    return stream;
fail:
    stream_destroy(stream);
    return NULL;
}

void stream_destroy(stream_t *stream)
{
    if (NULL != stream)
    {
        while(!tailq_empty(stream->rsp_q))
            free(tailq_pop_head(stream->rsp_q));
        if (NULL != stream->topic_patt)
            topic_destroy(stream->topic_patt);
        if (stream->have_connid)
            httpd_timeout_conn(stream->connid);
        if (NULL != stream->topicstr)
            free(stream->topicstr);
        if (NULL != stream->rsp_q)
            tailq_destroy(stream->rsp_q);
        free(stream);
    }
}

int stream_push(stream_t *stream, char *str)
{    
    char *s;
    if (NULL == (s = strdup(str)))
        return 1;
    return tailq_push_tail(stream->rsp_q, s);
}

void stream_refresh_timer(stream_t *stream)
{
    stream->timeoutS = DEFAULT_STREAM_TIMEOUT_S;    // refresh timer
}

void stream_drain(stream_t *stream, stream_drain_func f, void *userdata)
{
    bool first = true;
    while(!tailq_empty(stream->rsp_q))
    {
        char *s;
        s = (char *)tailq_pop_head(stream->rsp_q);
        f(stream, s, first, userdata);
        free(s);
        first = false;
    }
    stream_refresh_timer(stream);
}

bool stream_isempty(stream_t *stream)
{
    return tailq_empty(stream->rsp_q);
}

void stream_set_connection(stream_t *stream, uint32_t connid)
{
    if (stream->have_connid)
        LOG_DEBUG("stream hijacked, was connid=%u now %u", stream->connid, connid);
    stream->connid = connid;
    stream->have_connid = true;
}

bool stream_get_connection(stream_t *stream, uint32_t *connid)
{
    if (stream->have_connid)
    {
        *connid = stream->connid;
        return true;
    }
    return false;
}

void stream_clear_connection(stream_t *stream)
{
    stream->have_connid = false;
}

bool stream_1hz(uint32_t streamid, stream_t *stream, void *userdata)
{
    if (0 == stream->timeoutS--)
    {
        LOG_DEBUG("streamid=%u timeout", streamid);
        if (sublist_remove(stream->topicstr))
        {
            LOG_INFO("unsubscribing from %s", stream->topicstr);
            mqtt_unsubscribe(&g_srvctx.mqttctx, stream->topicstr, NULL);
        }
        return true;
    }
    return false;
}

