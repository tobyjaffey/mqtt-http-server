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
#include "httpd.h"
#include "httpdconnlist.h"
#include <common/tree.h>

struct httpdconn_node
{
    RB_ENTRY(httpdconn_node) entry;
    uint32_t connid;
    ebb_connection *conn;
};

static int httpdconn_cmp(struct httpdconn_node *e1, struct httpdconn_node *e2)
{
    return (e1->connid < e2->connid ? -1 : e1->connid > e2->connid);
}

RB_HEAD(httpdconnlist, httpdconn_node) httpdconn_head = RB_INITIALIZER(&httpdconn_head);
RB_PROTOTYPE(httpdconnlist, httpdconn_node, entry, httpdconn_cmp);
RB_GENERATE(httpdconnlist, httpdconn_node, entry, httpdconn_cmp);

int httpdconnlist_insert(uint32_t connid, ebb_connection *conn)
{
    struct httpdconn_node *data;

    if (NULL == (data = malloc(sizeof(struct httpdconn_node))))
        return 1;

    data->connid = connid;
    data->conn = conn;

    if (NULL == RB_INSERT(httpdconnlist, &httpdconn_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("httpdconnlist_insert connid collision");  // can't happen
        return 1;
    }

}

int httpdconnlist_remove(uint32_t connid)
{
    struct httpdconn_node key;
    struct httpdconn_node *res;
    key.connid = connid;
    res = RB_FIND(httpdconnlist, &httpdconn_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(httpdconnlist, &httpdconn_head, res);
    free(res);
    return 0;
}

struct ebb_connection *httpdconnlist_find(uint32_t connid)
{
    struct httpdconn_node key;
    struct httpdconn_node *res;
    key.connid = connid;
    res = RB_FIND(httpdconnlist, &httpdconn_head, &key);
    if (NULL == res)
        return NULL;
    return res->conn;
}

void httpdconnlist_foreach(httpdconnlist_iterate_func f, void *userdata)
{
    struct httpdconn_node *i;
    RB_FOREACH(i, httpdconnlist, &httpdconn_head)
    {
        f(i->connid, i->conn, userdata);
    }
}


