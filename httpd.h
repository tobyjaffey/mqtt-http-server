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
#ifndef HTTPD_H
#define HTTPD_H 1

#include <ev.h>
#include "ebb.h"
#include "idset.h"
#include "tailq.h"

typedef int (*query_iterate_func)(const char *key, const char *val, void *userdata);

#define MAX_VHOST 32

typedef struct
{
    char *rootdir;
    char *hostname;
    char *name;
} httpd_virt;

typedef struct
{
    ebb_connection *conn;
    uint32_t connid;
    char *body;   // request body
    size_t body_len;
    char *uri;    // request uri (including query str)
    char *path;   // request path part of uri
    char *auth;
    char host[512];
    bool parsing_auth_header;
    bool parsing_ifmodifiedsince_header;
    bool parsing_host_header;
} ebb_request_info;

typedef struct
{
    ebb_connection *conn;
    uint32_t connid;    // unique id for this connection
    bool keepalive;     // should this connection be kept alive after request (>= HTTP1.1)
    tailq_t *rsp_q;
    bool writing;       // busy writing an item from rsp_q
    bool finished;      // user sets this to say, after flushing queue, hangup.
    size_t push_len;    // total bytes pending

    bool have_mid;
    int mid;

    int version_major;
    int version_minor;

    bool reset;

    char writebuf[16384];
    char mimetype[512];
    int http_status;
    char *errorbody;
    time_t ifmodifiedsince;
    char last_modified[512];

    char *managed_str;  // userdata, but owned by this struct and freed by destructor

    idset_t *streamids; // set of streams associated with this connection
    bool rawmode;   // immediate response wanted

    char ipaddr[32];

} ebb_connection_info;

extern int httpd_init(ebb_server *server, struct ev_loop *loop);
extern int httpd_listen(ebb_server *server, const char *rootdir, uint16_t port, const char *cert, const char *key);
extern int httpd_push(uint32_t connid, const char *msg, bool flush);
extern int httpd_pushbin(uint32_t connid, const uint8_t *data, size_t len, bool flush);
extern int httpd_close(uint32_t connid);
extern void httpd_open_cb(uint32_t connid);
extern void httpd_close_cb(uint32_t connid);
extern int httpd_request_cb(uint32_t connid, const char *host, uint32_t method, const char *uri, const char *path, const char *body_data, size_t body_len, const char *auth);
extern int httpd_flush_stale(uint32_t connid);
extern int httpd_printf(uint32_t connid, const char *format, ...);
extern int httpd_query_iterate(const char *uri_str, query_iterate_func f, void *userdata);
extern ebb_connection_info *httpd_get_conninfo(uint32_t connid);
extern void httpd_timeout_conn(uint32_t connid);
extern void httpd_request_complete(uint32_t connid, ebb_connection_info *conninfo);

extern int httpd_addvirt(ebb_server *server, const char *rootdir, const char *hostname, const char *name);
extern int httpd_lookupvirt(const char *host);

#endif

