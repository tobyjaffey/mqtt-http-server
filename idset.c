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
#include "idset.h"
#include <common/tree.h>

static int id_cmp(struct idset_node *e1, struct idset_node *e2)
{
    return (e1->id < e2->id ? -1 : e1->id > e2->id);
}

RB_GENERATE(idset, idset_node, entry, id_cmp);

void idset_destroy(idset_t *ids)
{
    if (NULL != ids)
    {
        struct idset_node *var;
        struct idset_node *nxt;
       
        for (var = RB_MIN(idset, &ids->root); var != NULL; var = nxt)
        {
            nxt = RB_NEXT(idset, &ids->root, var);
            RB_REMOVE(idset, &ids->root, var);
            free(var);
        }

        free(ids);
    }
}

idset_t *idset_create(void)
{
    idset_t *ids;

    if (NULL == (ids = (idset_t *)calloc(sizeof(idset_t), 1)))
        goto fail;

    RB_INIT(&ids->root);

    return ids;
fail:
    idset_destroy(ids);
    return NULL;
}

int idset_insert(idset_t *ids, uint32_t id)
{
    struct idset_node *data;

    if (NULL == (data = malloc(sizeof(struct idset_node))))
        return 1;

    data->id = id;

    if (NULL == RB_INSERT(idset, &ids->root, data))    // NULL means OK
    {
        ids->count++;
        return 0;
    }
    else
    {
        LOG_ERROR("idset_insert id collision");
        crash();
        free(data);
        return 1;
    }
}

int idset_remove(idset_t *ids, uint32_t id)
{
    struct idset_node key;
    struct idset_node *res;

    key.id = id;
    res = RB_FIND(idset, &ids->root, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(idset, &ids->root, res);
    free(res);
    ids->count--;
    return 0;
}

bool idset_find(idset_t *ids, uint32_t id)
{
    struct idset_node key;
    struct idset_node *res;

    key.id = id;
    res = RB_FIND(idset, &ids->root, &key);
    if (NULL == res)
        return false;
    return true;
}

void idset_foreach(idset_t *ids, idset_iterate_func f, void *userdata)
{
    struct idset_node *var, *nxt;
    // can't use foreach if we want to allow deletion
    for (var = RB_MIN(idset, &ids->root); var != NULL; var = nxt)
    {
        nxt = RB_NEXT(idset, &ids->root, var);
        if (f(ids, var->id, userdata))
        {
            RB_REMOVE(idset, &ids->root, var);
            free(var);
        }
    }
}


void idset_print(idset_t *ids)
{
    struct idset_node *i;
    RB_FOREACH(i, idset, &ids->root)
    {
        LOG_INFO("%u", i->id);
    }
}


size_t idset_size(idset_t *ids)
{
    return ids->count;
}

