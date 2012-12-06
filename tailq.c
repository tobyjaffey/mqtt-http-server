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
#include <common/util.h>
#include <common/queue.h>

#include "tailq.h"

void tailq_destroy(tailq_t *tq)
{
    if (NULL != tq)
    {
        struct tailq_node *n1;
        struct tailq_node *n2;
       
        n1 = TAILQ_FIRST(&tq->root);
        while (n1 != NULL)
        {
            n2 = TAILQ_NEXT(n1, entry);
            free(n1);
            n1 = n2;
        }

        free(tq);
    }
}

tailq_t *tailq_create(void)
{
    tailq_t *tq;

    if (NULL == (tq = (tailq_t *)calloc(sizeof(tailq_t), 1)))
        goto fail;

    TAILQ_INIT(&tq->root);

    return tq;
fail:
    tailq_destroy(tq);
    return NULL;
}

int tailq_push_head(tailq_t *tq, void *userdata)
{
    struct tailq_node *n1;
LOG_DEBUG("tailq_push_head %p %p", tq, userdata);
    if (NULL == (n1 = calloc(sizeof(struct tailq_node), 1)))
        return 1;
    n1->userdata = userdata;
    TAILQ_INSERT_HEAD(&tq->root, n1, entry);
    return 0;
}

int tailq_push_tail(tailq_t *tq, void *userdata)
{
    struct tailq_node *n1;
LOG_DEBUG("tailq_push_tail %p %p", tq, userdata);
    if (NULL == (n1 = calloc(sizeof(struct tailq_node), 1)))
        return 1;
    n1->userdata = userdata;
    TAILQ_INSERT_TAIL(&tq->root, n1, entry);
    return 0;
}

void *tailq_pop_head(tailq_t *tq)
{
    struct tailq_node *n1;
    void *userdata;
    if (NULL == (n1 = TAILQ_FIRST(&tq->root)))
        return NULL;
    userdata = n1->userdata;
LOG_DEBUG("tailq_pop_head %p %p", tq, userdata);
    TAILQ_REMOVE(&tq->root, n1, entry);
    free(n1);
    return userdata;
}

void *tailq_pop_tail(tailq_t *tq)
{
    struct tailq_node *n1;
    void *userdata;
    if (NULL == (n1 = TAILQ_LAST(&tq->root, tailq)))
        return NULL;
    userdata = n1->userdata;
LOG_DEBUG("tailq_pop_tail %p %p", tq, userdata);
    TAILQ_REMOVE(&tq->root, n1, entry);
    free(n1);
    return userdata;
}

bool tailq_empty(tailq_t *tq)
{
    return TAILQ_EMPTY(&tq->root);
}

#if 0
void tailq_dump(tailq_t *tq)
{
    struct tailq_node *item;
    size_t sz;

    TAILQ_FOREACH(item, &tq->root, entry)
    {
        memcpy(&sz, item->userdata, sizeof(size_t));
        LOG_DEBUG("%p %u", item->userdata, sz);
        if (sz > 1024 && tq->magic)
        {
            crash();
        }
    }
}
#endif
