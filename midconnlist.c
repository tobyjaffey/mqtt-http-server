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
#include <common/tree.h>
#include "midconnlist.h"

struct midconn_node
{
    RB_ENTRY(midconn_node) entry;
    int mid;
    uint32_t connid;
};

static int midconn_cmp(struct midconn_node *e1, struct midconn_node *e2)
{
    return (e1->mid < e2->mid ? -1 : e1->mid > e2->mid);
}

RB_HEAD(midconnlist, midconn_node) midconn_head = RB_INITIALIZER(&midconn_head);
RB_PROTOTYPE(midconnlist, midconn_node, entry, midconn_cmp);
RB_GENERATE(midconnlist, midconn_node, entry, midconn_cmp);

int midconn_find(int mid, uint32_t *connid)
{
    struct midconn_node key;
    struct midconn_node *res;
    key.mid = mid;
    res = RB_FIND(midconnlist, &midconn_head, &key);
    if (NULL == res)
        return 1;
    *connid = res->connid;
    return 0;
}

int midconn_insert(int mid, uint32_t connid)
{
    struct midconn_node *data;

    if (NULL == (data = malloc(sizeof(struct midconn_node))))
        return 1;

    data->mid = mid;
    data->connid = connid;

    if (NULL == RB_INSERT(midconnlist, &midconn_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("middconn_insert mid collision");  // can't happen
        return 1;
    }
}

int midconn_remove(int mid)
{
    struct midconn_node key;
    struct midconn_node *res;
    key.mid = mid;
    res = RB_FIND(midconnlist, &midconn_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(midconnlist, &midconn_head, res);
    free(res);
    return 0;
}

void midconn_print(void)
{
    struct midconn_node *i;
    RB_FOREACH(i, midconnlist, &midconn_head)
    {
        LOG_INFO("mid=%d -> connid=%d\n", i->mid, i->connid);
    }
}

void midconn_foreach(midconn_iterate_func f, void *userdata)
{
    struct midconn_node *i;
    RB_FOREACH(i, midconnlist, &midconn_head)
    {
        f(i->mid, i->connid, userdata);
    }
}

#if 0
void midconn_test(void)
{
    uint32_t connid;

    midconn_insert(10, 100);
    midconn_insert(20, 200);
    midconn_insert(30, 300);

    midconn_print();

    if (midconn_find(20, &connid) < 0)
        LOG_CRITICAL("Sanity failure");
    LOG_INFO("mid 20 found on -> connid %d", connid);
}
#endif

