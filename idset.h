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
#ifndef IDSET_H
#define IDSET_H 1

#include <common/tree.h>

struct idset_node
{
    RB_ENTRY(idset_node) entry;
    uint32_t id;
};

typedef struct
{
    RB_HEAD(idset, idset_node) root;
    size_t count;
} idset_t;

RB_PROTOTYPE(idset, idset_node, entry, id_cmp);

typedef bool(*idset_iterate_func)(idset_t *ids, uint32_t id, void *userdata);

extern idset_t *idset_create(void);
extern void idset_destroy(idset_t *ids);
extern int idset_insert(idset_t *ids, uint32_t id);
extern int idset_remove(idset_t *ids, uint32_t id);
extern bool idset_find(idset_t *ids, uint32_t id);
extern void idset_foreach(idset_t *ids, idset_iterate_func f, void *data);
extern size_t idset_size(idset_t *ids);
extern void idset_print(idset_t *ids);

#endif

