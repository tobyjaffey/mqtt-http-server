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
#include "msgcache.h"
#include <common/tree.h>

struct mc_node
{
    RB_ENTRY(mc_node) entry;
    char *topic;
    char *msg;
};

static int mc_cmp(struct mc_node *e1, struct mc_node *e2)
{
    return strcmp(e1->topic, e2->topic);
}

RB_HEAD(msgcache, mc_node) mc_head = RB_INITIALIZER(&mc_head);
RB_PROTOTYPE(msgcache, mc_node, entry, mc_cmp);
RB_GENERATE(msgcache, mc_node, entry, mc_cmp);

const char *msgcache_lookup(char *topic)
{
    struct mc_node key;
    key.topic = topic;
    if (NULL == RB_FIND(msgcache, &mc_head, &key))
        return NULL;
    return key.msg;
}

int msgcache_insert(char *topic, char *msg)
{
    struct mc_node *found;
    struct mc_node *data = NULL;
    struct mc_node key;

    key.topic = topic;
    if (NULL != (found = RB_FIND(msgcache, &mc_head, &key)))
    {
        free(found->msg);
        if (NULL == (found->msg = strdup(msg)))
            return 1;
        else
            return 0;
    }
    else
    {
        if (NULL == (data = calloc(sizeof(struct mc_node), 1)))
            goto fail;

        if (NULL == (data->topic = strdup(topic)))
            goto fail;
        if (NULL == (data->msg = strdup(msg)))
            goto fail;

        if (NULL != RB_INSERT(msgcache, &mc_head, data))    // NULL means OK
        {
            LOG_CRITICAL("msgcache_insert mc collision");  // can't happen
            return 1;
        }
    }
    return 0;

fail:
    if (NULL != data)
    {
        if (NULL != data->topic)
            free(topic);
        if (NULL != data->msg)
            free(msg);
        free(data);
    }
    return 1;
}

bool msgcache_remove(char *topic)
{
    struct mc_node key;
    struct mc_node *res;

    key.topic = topic;
    res = RB_FIND(msgcache, &mc_head, &key);
    if (NULL == res)
        return false;
    
    RB_REMOVE(msgcache, &mc_head, res);
    free(res->topic);
    free(res->msg);
    free(res);
    return true;
}


void msgcache_foreach(msgcache_iterate_func f, void *userdata)
{
    struct mc_node *i;
    RB_FOREACH(i, msgcache, &mc_head)
    {
        f(i->topic, i->msg, userdata);
    }
}


