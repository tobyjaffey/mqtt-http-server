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
#include <common/b64.h>
#include <common/tdate_parse.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ev.h>
#include "ebb.h"
#include <uriparser/Uri.h>

#include "httpd.h"
#include "mqtthttpd.h"
#include "httpdconnlist.h"
#include "streamlist.h"

static void on_uri_complete(ebb_request *request, const char *at, size_t length);
static void on_body_complete(ebb_request *request, const char *at, size_t length);
static void on_path_complete(ebb_request *request, const char *at, size_t length);
static ebb_connection* on_new_connection(ebb_server *server, struct sockaddr_in *addr);
static void on_close(ebb_connection *conn);
static void on_request_complete(ebb_request *request);
static void on_write_complete(ebb_connection *conn);
static void on_header_field(ebb_request*, const char *at, size_t length, int header_index);
static void on_header_value(ebb_request*, const char *at, size_t length, int header_index);

extern server_context_t g_srvctx;

extern void httpd_timeout_conn(uint32_t connid)
{
    ebb_connection_info *conninfo;
    if (NULL != (conninfo = httpd_get_conninfo(connid)))
    {
        conninfo->http_status = 408;    // timeout
        httpd_close(connid);
    }
}

ebb_connection_info *httpd_get_conninfo(uint32_t connid)
{
    ebb_connection *conn;
    if (NULL == (conn = httpdconnlist_find(connid)))
        return NULL;
    return (ebb_connection_info *)conn->data;
}


int httpd_query_iterate(const char *uri_str, query_iterate_func f, void *userdata)
{
    UriParserStateA state;
    UriUriA uri;
    UriQueryListA *queryList, *p;
    int itemCount;

    state.uri = &uri;
    if (uriParseUriA(&state, uri_str) != URI_SUCCESS)
    {
        uriFreeUriMembersA(&uri);
        return 1;
    }

    if (uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first, uri.query.afterLast) != URI_SUCCESS)
    {
        uriFreeUriMembersA(&uri);
        return 1;
    }

    p = queryList;
    while(NULL != p)
    {
        if (0 == f(p->key, p->value, userdata))
            break;
        p = p->next;
    }

    uriFreeQueryListA(queryList);
    uriFreeUriMembersA(&uri);
    return 0;
}


// rsp_q deque holds blocks
// each block is {size_t, data}

static void connection_flush_write(ebb_connection *conn)
{
    ebb_connection_info *conninfo = (ebb_connection_info *)(conn->data);
    if (!conninfo->writing)
    {
        uint8_t *blockp;
        uint8_t *data;
        size_t len = 0;

        blockp = tailq_pop_head(conninfo->rsp_q);
/*
LOG_INFO("flush %p", blockp);
if (NULL != blockp)
dump(blockp, 16);
*/
        if (NULL != blockp)
        {
            len = *((size_t *)blockp);
            data = blockp + sizeof(size_t);

            conninfo->writing = true;
            LOG_DEBUG("conn_write");
            if (len > sizeof(conninfo->writebuf))
                LOG_CRITICAL("block too big %u of %u", len, sizeof(conninfo->writebuf));
            memcpy(conninfo->writebuf, data, len);
            conninfo->push_len -= len;
            free(blockp);
            if (len)
                ebb_connection_write(conn, conninfo->writebuf, len, on_write_complete);
        }
        else
        {   // reached end of q
            LOG_DEBUG("End of write q");
            if (conninfo->finished)
            {
                if (!conninfo->keepalive)
                    ebb_connection_schedule_close(conn);
            }
        }
    }
}

static void on_write_complete(ebb_connection *conn)
{
    ebb_connection_info *conninfo = (ebb_connection_info *)(conn->data);
    conninfo->writing = false;
    connection_flush_write(conn);    // try and send some more
}

static int ebb_connection_pushbin(ebb_connection *conn, const uint8_t *msg, size_t len, bool flush)
{
    ebb_connection_info *conninfo = (ebb_connection_info *)(conn->data);
    uint8_t *block;

    block = (uint8_t *)malloc(sizeof(size_t) + len);

    LOG_DEBUG("********* ebb_connection_pushbin str=%p", msg);
    if (block == NULL)
    {
        LOG_ERROR("strdup failed, silently closing conn");
        ebb_connection_schedule_close(conn);
        return 1;
    }
    else
    {
        memcpy(block, &len, sizeof(size_t));
        memcpy(block + sizeof(size_t), msg, len);

        //LOG_INFO("Pushed block %u (+%u) bytes", len, sizeof(size_t));
        //dump(block, len + sizeof(size_t));
        tailq_push_tail(conninfo->rsp_q, block);    // FIXME, error check
        conninfo->push_len += len;
        if (flush)
            connection_flush_write(conn);
        return 0;
    }
}

static int ebb_connection_push(ebb_connection *conn, const char *msg, bool flush)
{
    return ebb_connection_pushbin(conn, (const uint8_t *)msg, strlen(msg), flush);
}


static int ebb_connection_push_header(ebb_connection *conn, const char *msg)
{
    ebb_connection_info *conninfo = (ebb_connection_info *)(conn->data);
    uint8_t *block;
    size_t msglen;

    msglen = strlen(msg);

    block = (uint8_t *)malloc(sizeof(size_t) + msglen);

LOG_DEBUG("pushhdr, block = %p", block);

    LOG_DEBUG("********* ebb_connection_push_header str=%p", block);
    if (block == NULL)
    {
        LOG_ERROR("strdup failed, silently closing conn");
        ebb_connection_schedule_close(conn);
        return 1;
    }
    else
    {
        memcpy(block, &msglen, sizeof(size_t));
        memcpy(block + sizeof(size_t), msg, msglen);

        LOG_DEBUG("Pushed hdr %s", msg);
        tailq_push_head(conninfo->rsp_q, block);
        conninfo->push_len += msglen;
        connection_flush_write(conn);
        return 0;
    }
}


////////////////////////



static void ebb_request_destroy(ebb_request *request)
{
    ebb_request_info *request_info;

    LOG_DEBUG("*********** ebb_request_destroy %p", request);

    if (request != NULL)
    {
        request_info = (ebb_request_info *)request->data;
        if (NULL != request_info)
        {
            if (request_info->body != NULL)
                free(request_info->body);
            if (request_info->path != NULL)
                free(request_info->path);
            if (request_info->uri != NULL)
                free(request_info->uri);
            if (request_info->auth != NULL)
                free(request_info->auth);

            free(request_info);
        }
        free(request);
    }
}

static ebb_request *ebb_request_create(ebb_connection *conn)
{
    ebb_request *request;
    ebb_request_info *request_info;

    if (NULL == (request = calloc(1, sizeof(ebb_request))))
        goto fail;

    if (NULL == (request_info = calloc(1, sizeof(ebb_request_info))))
        goto fail;

    ebb_request_init(request);
    request->data = (void *)request_info;
    request->on_complete = on_request_complete;
    request->on_uri = on_uri_complete;
    request->on_body = on_body_complete;
    request->on_path = on_path_complete;
    request->on_header_field = on_header_field;
    request->on_header_value = on_header_value;

    request_info->conn = conn;

    ebb_connection_reset_timeout(conn);

    return request;
fail:
    ebb_request_destroy(request);
    ebb_connection_schedule_close(conn);
    return NULL;
}

static void ebb_connection_info_rsp_q_destroyall(tailq_t *tq)
{
    if (NULL != tq)
    {
        while(!tailq_empty(tq))
            free(tailq_pop_head(tq));
    }
}

static void ebb_connection_info_destroy(ebb_connection_info *conninfo)
{
    if (conninfo != NULL)
    {
        ebb_connection_info_rsp_q_destroyall(conninfo->rsp_q);
        if (conninfo->errorbody != NULL)
            free(conninfo->errorbody);
        if (conninfo->managed_str != NULL)
            free(conninfo->managed_str);
        if (conninfo->streamids != NULL)
            idset_destroy(conninfo->streamids);
        if (conninfo->rsp_q != NULL)
            tailq_destroy(conninfo->rsp_q);
        free(conninfo);
    }
}

static ebb_connection_info *ebb_connection_info_create(void)
{
    ebb_connection_info *conninfo;

    if (NULL == (conninfo = (ebb_connection_info *)calloc(1, sizeof(ebb_connection_info))))
        goto fail;
    if (NULL == (conninfo->streamids = idset_create()))
        goto fail;
    if (NULL == (conninfo->rsp_q = tailq_create()))
        goto fail;
    conninfo->rsp_q->magic = true;

    return conninfo;
fail:
    ebb_connection_info_destroy(conninfo);
    return NULL;
}


static void ebb_connection_destroy(ebb_connection *conn)
{
    if (NULL != conn)
    {
        ebb_connection_info_destroy((ebb_connection_info *)conn->data);
        free(conn);
    }
}


static ebb_connection *ebb_connection_create(const struct sockaddr_in *addr)
{
    ebb_connection *conn;
    ebb_connection_info *conninfo;

    if (NULL == (conn = (ebb_connection *)calloc(1, sizeof(ebb_connection))))
        goto fail;
    if (NULL == (conninfo = ebb_connection_info_create()))
        goto fail;

    ebb_connection_init(conn);
    conn->data = (void *)conninfo;
    conninfo->conn = conn;

    strncpy(conninfo->ipaddr, inet_ntoa(addr->sin_addr), sizeof(conninfo->ipaddr));

    // don't reuse connids
    do
    {
        conninfo->connid = g_srvctx.httpdconnid++;
    }   // this won't spin forever as there will never be > 2^32 connections
    while(httpdconnlist_find(conninfo->connid) != NULL);

    conn->new_request = ebb_request_create;
    conn->on_close = on_close;
    httpd_open_cb(conninfo->connid);
    return conn;
fail:
    ebb_connection_destroy(conn);
    return NULL;
}


//////////////////////////////////////////////

static void on_close(ebb_connection *conn)
{
    ebb_connection_info *info = (ebb_connection_info *)(conn->data);
    httpd_close_cb(info->connid);
    httpdconnlist_remove(info->connid);
    ebb_connection_destroy(conn);
}

static void on_request_complete(ebb_request *request)
{
    ebb_request_info *reqinfo = (ebb_request_info *)request->data;
    ebb_connection *conn = reqinfo->conn;
    ebb_connection_info *conninfo = (ebb_connection_info *)conn->data;
    char hdr[512];

    LOG_DEBUG("request complete: method=%d transfer_encoding=%d, expect_cont=%d, ver_maj=%d, ver_min=%d, num_hdr=%d, len=%d, body_read=%d", request->method, request->transfer_encoding, request->expect_continue, request->version_major, request->version_minor, request->number_of_headers, request->content_length, request->body_read);

    strcpy(conninfo->mimetype, "text/plain");

    conninfo->finished = false;
    conninfo->http_status = 200;    // success by default
    conninfo->last_modified[0] = 0;

    if (NULL != conninfo->errorbody)
    {
        free(conninfo->errorbody);
        conninfo->errorbody = NULL;
    }

    if (request->version_major >= 1 && request->version_minor >= 1)
        conninfo->keepalive = true; // keepalive for HTTP 1.1+

    conninfo->version_major = request->version_major;
    conninfo->version_minor = request->version_minor;

    if (0 != httpd_request_cb(conninfo->connid, reqinfo->host, request->method, reqinfo->uri, reqinfo->path, reqinfo->body, reqinfo->body_len, reqinfo->auth))
    {
        conninfo->finished = true;
        if (NULL != conninfo->errorbody)
        {
            snprintf(hdr, sizeof(hdr), "HTTP/%d.%d %d\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n", request->version_major, request->version_minor, conninfo->http_status, strlen(conninfo->errorbody));
            ebb_connection_push_header(conn, hdr);
            ebb_connection_push_header(conn, conninfo->errorbody);
        }
        else
        {   // empty JSON response
            snprintf(hdr, sizeof(hdr), "HTTP/%d.%d %d\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\n{}", request->version_major, request->version_minor, conninfo->http_status);
            ebb_connection_push_header(conn, hdr);
        }
    }

    ebb_request_destroy(request);

    if (conninfo->push_len > 0 && conninfo->finished)
        connection_flush_write(conn);

    httpd_request_complete(conninfo->connid, conninfo);
}

static void on_header_field(ebb_request* request, const char *at, size_t length, int header_index)
{
    ebb_request_info *reqinfo = (ebb_request_info *)(request->data);

    if (length == strlen("Host"))
    {
        if (0 == memcmp(at, "Host", length))
        {
            reqinfo->parsing_host_header = true;
            return;
        }
    }

    if (length == strlen("Authorization"))
    {
        if (0 == memcmp(at, "Authorization", length))
        {
            reqinfo->parsing_auth_header = true;
            return;
        }
    }
    if (length == strlen("If-Modified-Since"))
    {
        if (0 == memcmp(at, "If-Modified-Since", length))
        {
            reqinfo->parsing_ifmodifiedsince_header = true;
            return;
        }
    }

    reqinfo->parsing_auth_header = false;
    reqinfo->parsing_ifmodifiedsince_header = false;
    reqinfo->parsing_host_header = false;
}

static void on_header_value(ebb_request* request, const char *at, size_t length, int header_index)
{
    ebb_request_info *reqinfo = (ebb_request_info *)(request->data);
    ebb_connection *conn = reqinfo->conn;
    ebb_connection_info *conninfo = (ebb_connection_info *)(conn->data);

    if (reqinfo->parsing_host_header)
    {
        if (length+1 > sizeof(reqinfo->host))
        {
            reqinfo->host[0] = 0;
        }
        else
        {
            memcpy(reqinfo->host, at, length);
            reqinfo->host[length] = 0;
        }
        reqinfo->parsing_host_header = false;
    }

    if (reqinfo->parsing_auth_header)
    {
        if (length > 6)
        {
            if (0 == memcmp(at, "Basic ", 6))
            {
                if (NULL != (reqinfo->auth = calloc(1, (length-6)+1)))
                {
                    b64_decode(reqinfo->auth, at + 6, length-6);
                    LOG_DEBUG("auth='%s'", reqinfo->auth);
                }
            }
        }
        reqinfo->parsing_auth_header = false;
    }

    if (reqinfo->parsing_ifmodifiedsince_header)
    {
        char str[512];
        if (length < sizeof(str))
        {
            memcpy(str, at, length);
            str[length] = 0;
            conninfo->ifmodifiedsince = tdate_parse(str);
            LOG_DEBUG("tdate_parse %s = %u", str, conninfo->ifmodifiedsince);
        }
        reqinfo->parsing_ifmodifiedsince_header = false;
    }
}

static void on_uri_complete(ebb_request *request, const char *at, size_t length)
{
    ebb_request_info *reqinfo = (ebb_request_info *)(request->data);
    if (NULL != reqinfo->uri)
        free(reqinfo->uri);

    reqinfo->uri = malloc(length+1);
    if (NULL != reqinfo->uri)
    {
        memcpy(reqinfo->uri, at, length);
        reqinfo->uri[length] = 0;
    }
}

static void on_body_complete(ebb_request *request, const char *at, size_t length)
{
    ebb_request_info *reqinfo = (ebb_request_info *)(request->data);
    char *newbuf;

    if (NULL == (newbuf = (char *)malloc(length + reqinfo->body_len)))
        goto fail;

    if (NULL != reqinfo->body)
        memcpy(newbuf, reqinfo->body, reqinfo->body_len);
    memcpy(newbuf+reqinfo->body_len, at, length);

    if (NULL != reqinfo->body)
        free(reqinfo->body);

    reqinfo->body = newbuf;
    reqinfo->body_len += length;

    return;
fail:
    LOG_ERROR("out of mem on_body");
}

static void on_path_complete(ebb_request *request, const char *at, size_t length)
{
    ebb_request_info *reqinfo = (ebb_request_info *)(request->data);

    if (NULL != reqinfo->path)
        free(reqinfo->path);

    reqinfo->path = malloc(length+1);
    if (NULL != reqinfo->path)
    {
        memcpy(reqinfo->path, at, length);
        reqinfo->path[length] = 0;
    }
}

static ebb_connection* on_new_connection(ebb_server *server, struct sockaddr_in *addr)
{
    ebb_connection *conn;
    conn = ebb_connection_create(addr);
    if (NULL == conn)
        return NULL;
    httpdconnlist_insert(((ebb_connection_info *)(conn->data))->connid, conn);
    return conn;
}

/////////////////////////////////////////

int httpd_init(ebb_server *server, struct ev_loop *loop)
{
    ebb_server_init(server, loop);
    server->new_connection = on_new_connection;
    return 0;
}

int httpd_listen(ebb_server *server, const char *rootdir, uint16_t port, const char *cert, const char *key)
{
    if (NULL != g_srvctx.httpd_root)
        free(g_srvctx.httpd_root);

    g_srvctx.httpd_root = strdup(rootdir);
    if (NULL == g_srvctx.httpd_root)
        return 1;

    if (NULL != cert && NULL != key)
        ebb_server_set_secure(server, cert, key);
    if (ebb_server_listen_on_port(server, port)<0)
        return 1;
    return 0;
}

int httpd_addvirt(ebb_server *server, const char *rootdir, const char *hostname, const char *name)
{
    if (g_srvctx.num_vhost >= MAX_VHOST-1)
    {
        LOG_ERROR("Too many vhost, increase MAX_VHOST");
        return 1;
    }
    
    if (NULL == (g_srvctx.vhttpd[g_srvctx.num_vhost].rootdir = strdup(rootdir)))
        goto fail;
    if (NULL == (g_srvctx.vhttpd[g_srvctx.num_vhost].hostname = strdup(hostname)))
        goto fail;
    if (NULL == (g_srvctx.vhttpd[g_srvctx.num_vhost].name = strdup(name)))
        goto fail;

    g_srvctx.num_vhost++;
    return 0;

fail:
    if (NULL != g_srvctx.vhttpd[g_srvctx.num_vhost].rootdir)
        free(g_srvctx.vhttpd[g_srvctx.num_vhost].rootdir);
    if (NULL != g_srvctx.vhttpd[g_srvctx.num_vhost].hostname)
        free(g_srvctx.vhttpd[g_srvctx.num_vhost].hostname);
    if (NULL != g_srvctx.vhttpd[g_srvctx.num_vhost].name)
        free(g_srvctx.vhttpd[g_srvctx.num_vhost].name);
    return 1;
}

int httpd_lookupvirt(const char *host)
{
    int i;
    for (i=0;i<g_srvctx.num_vhost;i++)
    {
        if (0==strcmp(host, g_srvctx.vhttpd[i].hostname))
            return i;
    }
    return -1;
}

int httpd_printf(uint32_t connid, const char *format, ...)
{
    va_list args;
    char buf[65536];    // FIXME

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

//LOG_INFO("httpd push connid=%u %s", connid, buf);

    return httpd_push(connid, buf, false);
}


int httpd_push(uint32_t connid, const char *msg, bool flush)
{
    ebb_connection *conn;
    conn = httpdconnlist_find(connid);
    if (NULL == conn)
    {
        LOG_WARN("httpd_push no such connid %d", connid);
        return 1;
    }
    return ebb_connection_push(conn, msg, flush);
}

int httpd_pushbin(uint32_t connid, const uint8_t *msg, size_t len, bool flush)
{
    ebb_connection *conn;
    conn = httpdconnlist_find(connid);
    if (NULL == conn)
    {
        LOG_WARN("httpd_push no such connid %d", connid);
        return 1;
    }
    return ebb_connection_pushbin(conn, msg, len, flush);
}

int httpd_close(uint32_t connid)
{
    ebb_connection *conn;
    ebb_connection_info *conninfo;
    char hdr[1024];
    const char *rfc1123fmt = "%a, %d %b %Y %H:%M:%S GMT";
    char datestr[512];
    time_t t;

    LOG_DEBUG("httpd_close (we closed) %u", connid);

    conn = httpdconnlist_find(connid);
    if (NULL == conn)
    {
        LOG_WARN("httpd_push no such connid %d", connid);
        return 1;
    }

    conninfo = (ebb_connection_info *)(conn->data);

    if (conninfo->finished)
    {
        // this is not meant to happen
        LOG_ERROR("**** httpd_close already finished connid=%u!", connid);
    }

    conninfo->finished = true;

    // create date string
    t = getTime();
    strftime(datestr, sizeof(datestr), rfc1123fmt, localtime(&t));

    // write out fixed size header
    if (conninfo->last_modified[0] != 0)
    {
        snprintf(hdr, sizeof(hdr), "HTTP/%d.%d %d\r\nLast-Modified: %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nDate: %s\r\n\r\n", conninfo->version_major, conninfo->version_minor, conninfo->http_status, conninfo->last_modified, conninfo->mimetype, conninfo->push_len, datestr);
    }
    else
    {
        snprintf(hdr, sizeof(hdr), "HTTP/%d.%d %d\r\nContent-Type: %s\r\nContent-Length: %zu\r\nDate: %s\r\n\r\n", conninfo->version_major, conninfo->version_minor, conninfo->http_status, conninfo->mimetype, conninfo->push_len, datestr);
    }

    ebb_connection_push_header(conn, hdr);
    connection_flush_write(conn);
    return 0;
}

int httpd_flush_stale(uint32_t connid)
{
    ebb_connection *conn;
    ebb_connection_info *conninfo;

    conn = httpdconnlist_find(connid);
    if (NULL == conn)
    {
        LOG_WARN("httpd_push no such connid %d", connid);
        return 1;
    }
    conninfo = (ebb_connection_info *)(conn->data);
    
    ebb_connection_info_rsp_q_destroyall(conninfo->rsp_q);
    conninfo->push_len = 0;
    return 0;
}

