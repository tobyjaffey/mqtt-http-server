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
#include "sublist.h"
#include <common/tree.h>

struct sub_node
{
    RB_ENTRY(sub_node) entry;
    char *topic;
    uint32_t count;
};

static int sub_cmp(struct sub_node *e1, struct sub_node *e2)
{
    return strcmp(e1->topic, e2->topic);
}

RB_HEAD(sublist, sub_node) sub_head = RB_INITIALIZER(&sub_head);
RB_PROTOTYPE(sublist, sub_node, entry, sub_cmp);
RB_GENERATE(sublist, sub_node, entry, sub_cmp);

static struct sub_node *sublist_find(char *topic)
{
    struct sub_node key;
    key.topic = topic;
    return RB_FIND(sublist, &sub_head, &key);
}

bool sublist_insert(char *topic)
{
    struct sub_node *found;

    if (NULL != (found = sublist_find(topic)))
    {
        found->count++;
        return false;
    }
    else
    {
        struct sub_node *data;

        if (NULL == (data = malloc(sizeof(struct sub_node))))
            return false;

        if (NULL == (data->topic = strdup(topic)))
        {
            free(data);
            return false;
        }
        data->count = 1;

        if (NULL == RB_INSERT(sublist, &sub_head, data))    // NULL means OK
            return true;
        else
        {
            LOG_CRITICAL("sublist_insert sub collision");  // can't happen
            return false;
        }
    }
}

bool sublist_remove(char *topic)
{
    struct sub_node key;
    struct sub_node *res;

    key.topic = topic;
    res = RB_FIND(sublist, &sub_head, &key);
    if (NULL == res)
        return false;
    
    if (--res->count == 0)
    {
        RB_REMOVE(sublist, &sub_head, res);
        free(res->topic);
        free(res);
        return true;
    }

    return false;
}


void sublist_foreach(sublist_iterate_func f, void *userdata)
{
    struct sub_node *i;
    RB_FOREACH(i, sublist, &sub_head)
    {
        f(i->topic, i->count, userdata);
    }
}


