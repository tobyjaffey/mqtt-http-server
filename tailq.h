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
#ifndef TAILQ_H
#define TAILQ_H 1

#include <common/queue.h>

struct tailq_node
{
    TAILQ_ENTRY(tailq_node) entry;
    void *userdata;
};

typedef struct
{
    TAILQ_HEAD(tailq, tailq_node) root;
    bool magic;
} tailq_t;

extern tailq_t *tailq_create(void);
extern void tailq_destroy(tailq_t *tq);

extern int tailq_push_head(tailq_t *tq, void *userdata);
extern int tailq_push_tail(tailq_t *tq, void *userdata);

extern void *tailq_pop_head(tailq_t *tq);
extern void *tailq_pop_tail(tailq_t *tq);

extern bool tailq_empty(tailq_t *tq);
extern void tailq_dump(tailq_t *tq);

#endif

