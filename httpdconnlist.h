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
#ifndef HTTPDCONNLIST_H
#define HTTPDCONNLIST_H 1

#include "httpd.h"

typedef void(*httpdconnlist_iterate_func)(uint32_t connid, ebb_connection *conn, void *userdata);

extern int httpdconnlist_insert(uint32_t connid, ebb_connection *conn);
extern int httpdconnlist_remove(uint32_t connid);
extern struct ebb_connection *httpdconnlist_find(uint32_t connid);
extern void httpdconnlist_foreach(httpdconnlist_iterate_func f, void *data);

#endif

