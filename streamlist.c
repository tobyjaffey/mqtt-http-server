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
#include "stream.h"
#include "streamlist.h"
#include <common/tree.h>

struct stream_node
{
    RB_ENTRY(stream_node) entry;
    uint32_t streamid;
    stream_t *stream;
};

static int stream_cmp(struct stream_node *e1, struct stream_node *e2)
{
    return (e1->streamid < e2->streamid ? -1 : e1->streamid > e2->streamid);
}

RB_HEAD(streamlist, stream_node) stream_head = RB_INITIALIZER(&stream_head);
RB_PROTOTYPE(streamlist, stream_node, entry, stream_cmp);
RB_GENERATE(streamlist, stream_node, entry, stream_cmp);

int streamlist_insert(uint32_t streamid, stream_t *stream)
{
    struct stream_node *data;

    if (NULL == (data = malloc(sizeof(struct stream_node))))
        return 1;

    data->streamid = streamid;
    data->stream = stream;

    if (NULL == RB_INSERT(streamlist, &stream_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("streamlist_insert streamid collision");  // can't happen
        return 1;
    }

}

stream_t *streamlist_remove(uint32_t streamid)
{
    struct stream_node key;
    struct stream_node *res;
    stream_t *stream;

    key.streamid = streamid;
    res = RB_FIND(streamlist, &stream_head, &key);
    if (NULL == res)
        return NULL;
    RB_REMOVE(streamlist, &stream_head, res);
    stream = res->stream;
    free(res);

    return stream;
}

stream_t *streamlist_find(uint32_t streamid)
{
    struct stream_node key;
    struct stream_node *res;

    key.streamid = streamid;
    res = RB_FIND(streamlist, &stream_head, &key);
    if (NULL == res)
        return NULL;
    return res->stream;
}

void streamlist_foreach(streamlist_iterate_func f, void *userdata)
{
    struct stream_node *var, *nxt;
    // can't use foreach if we want to allow deletion
    for (var = RB_MIN(streamlist, &stream_head); var != NULL; var = nxt)
    {
        nxt = RB_NEXT(streamlist, &stream_head, var);
        if (f(var->streamid, var->stream, userdata))
        {
            RB_REMOVE(streamlist, &stream_head, var);
            if (0 != streamlist_remove(var->streamid))
                stream_destroy(var->stream);
            free(var);
        }
    }
}


