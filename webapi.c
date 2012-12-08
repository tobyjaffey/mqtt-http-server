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

// FIXME
//on unsubscribe, walk msgcache for all matching matching messages and remove them



#include <common/common.h>
#include <common/logging.h>
#include <common/util.h>
#include <common/cJSON.h>
#include "mqtthttpd.h"
#include "httpd.h"
#include "webapi.h"
#include "webapi_serve.h"
#include "midconnlist.h"
#include "streamlist.h"
#include "stream.h"
#include "sublist.h"
#include "msgcache.h"
#include "accesslog.h"

#define MAX_PATH_ELEMS 256
const int MAX_STREAMS = 256;

extern server_context_t g_srvctx;

static void inject_retained(const char *topic, const char *msg, void *userdata);


static char *json_escape(const char *in)
{
    char *out;
    size_t len = strlen(in);
    char *outp;

    if (NULL == (outp = out = malloc(len*2+3))) // worst case
        return NULL;

    *outp++ = '"';

    while(*in)
    {
        switch(*in)
        {
            case '"':
                *outp++ = '\\';
                *outp++ = '"';
            break;
            case '\\':
                *outp++ = '\\';
                *outp++ = '\\';
            break;
            case '\r':
                *outp++ = '\\';
                *outp++ = 'r';
            break;
            case '\n':
                *outp++ = '\\';
                *outp++ = 'n';
            break;
            case '\t':
                *outp++ = '\\';
                *outp++ = 't';
            break;

            /*
            case '\'':
                *outp++ = '\\';
                *outp++ = '\'';
            break;
            */
            /*
            case '/':
                *outp++ = '\\';
                *outp++ = '/';
            break;
            */

            default:
                *outp++ = *in;
        }
        in++;
    }
    *outp++ = '"';
    *outp = 0;

    return out;
}

static void stream_drainer(stream_t *stream, const char *str, bool first, void *userdata)
{
    uint32_t connid = (uint32_t)(uintptr_t)userdata;
    LOG_DEBUG("drainer: %s first=%d", str, first);
    httpd_printf(connid, "%s%s", first ? "" : ",", str);
}

static bool strm_clr(idset_t *ids, uint32_t streamid, void *userdata)
{
    stream_t *stream;
    if (NULL == (stream = streamlist_find(streamid)))
    {
        LOG_ERROR("streamid %u not in list!", streamid);
        return true;
    }
    stream_clear_connection(stream);
    return true;
}


void httpd_request_complete(uint32_t connid, ebb_connection_info *conninfo)
{
    LOG_DEBUG("req complete connid=%u", connid);
}

static void fail_conn(int mid, uint32_t connid, void *userdata)
{
    ebb_connection_info *conninfo;
    if (NULL != (conninfo = httpd_get_conninfo(connid)))
    {
        conninfo->http_status = 503;
        httpd_close(connid);
    }
}

void webapi_mqtt_failall_cb(void)
{
    midconn_foreach(fail_conn, NULL);
}

void webapi_publish_cb(int mid)
{
    uint32_t connid;
    if (0 == midconn_find(mid, &connid))
    {
        httpd_close(connid);
    }
    else
    {
        LOG_ERROR("webapi_publish_cb mid not found %d", mid);
    }
    midconn_remove(mid);
}

struct topic_message
{
    const char *topic;
    const char *msg;
};

bool message_to_stream(uint32_t streamid, stream_t *stream, void *userdata)
{
    struct topic_message *tm = (struct topic_message *)userdata;
    char *s = NULL;

    LOG_DEBUG("rx check: %s %s", stream->topic_patt, tm->topic);

    if (topic_match_string(stream->topic_patt, tm->topic))
    {
        size_t len;

        LOG_DEBUG("%s:%s -> stream %u", tm->topic, tm->msg, streamid);

        len = strlen(tm->topic)*2 + strlen(tm->msg)*2 + 8;  // {"":""} + null // FIXME

        if (NULL == (s = (char *)malloc(len)))
            LOG_ERROR("out of mem");
        else
        {
            uint32_t connid;
            ebb_connection_info *conninfo = NULL;
            bool connected;

            connected = stream_get_connection(stream, &connid);

            if (connected)
            {
                if (NULL == (conninfo = httpd_get_conninfo(connid)))
                {
                    LOG_ERROR("bad connid");
                    return false;
                }
                if (conninfo->finished) // if conn is in process of closing, 
                    connected = false;
            }

            if (conninfo && conninfo->rawmode)
            {
                // just the data
                memcpy(s, tm->msg, strlen(tm->msg)+1);
            }
            else
            {
                // wrapped in JSON obj
                cJSON *mj = NULL;
                char *msgesc;
                char *topicesc;

                if (NULL == (topicesc = json_escape(tm->topic)))
                {
                    conninfo->http_status = 503;
                    httpd_close(connid);
                    return false;
                }

                if (tm->msg[0] == 0)
                {
                    snprintf(s, len, "{%s:\"\"}", topicesc);
                }
                else
                if (0==strcmp(tm->msg, "inf"))
                {
                    snprintf(s, len, "{%s:\"inf\"}", topicesc);
                }
                else
                if (NULL != (mj = cJSON_Parse(tm->msg)))
                {
                    snprintf(s, len, "{%s:%s}", topicesc, cJSON_PrintUnformatted(mj));
                    cJSON_Delete(mj);
                }
                else
                {
                    if (NULL == (msgesc = json_escape(tm->msg)))
                    {
                        conninfo->http_status = 503;
                        httpd_close(connid);
                        free(topicesc);
                        return false;
                    }
                    else
                    {
                        snprintf(s, len, "{%s:%s}", topicesc, msgesc);
                        free(msgesc);
                    }
                }
                free(topicesc);
            }
            if (0 != stream_push(stream, s))
                LOG_ERROR("stream_push %u failed", streamid);
            else
            {
                if (connected)
                {
                    if (!conninfo->rawmode)
                        httpd_printf(connid, "[");
                    stream_drain(stream, stream_drainer, (void *)(uintptr_t)connid);
                    if (!conninfo->rawmode)
                        httpd_printf(connid, "]");
                    stream_clear_connection(stream);
                    httpd_close(connid);
                }
                else
                {
                    LOG_DEBUG("streamid %u not connected", stream->streamid);
                }
            }
        }
    }
    if (NULL != s)
        free(s);
    return false;
}

void webapi_message_cb(int mid, char *topic, char *msg)
{
    struct topic_message tm = {topic, msg};
    LOG_DEBUG("webapi_message_cb %s %s", topic, msg);

    msgcache_insert(topic, msg);
    streamlist_foreach(message_to_stream, (void *)&tm);

//    if (topic[0] == 0)  // FIXME, for us, empty == null
//        msgcache_remove(topic);
}

static stream_t *webapi_subscribe_response(ebb_connection_info *conninfo, const char *topic)
{
    stream_t *stream = NULL;

    if (NULL == (stream = stream_create(topic)))
    {
        LOG_ERROR("failed to create stream for topic patt %s", topic);
        conninfo->http_status = 503;
    }
    else
    {
        char seckey_str[STREAM_SECKEY_SIZE * 2 + 1];    // hex print
        int i;

        if (0 != streamlist_insert(stream->streamid, stream))
        {
            LOG_ERROR("streamlist_insert failed");
            conninfo->http_status = 503;
        }

        // print hex string
        for (i=0;i<STREAM_SECKEY_SIZE;i++)
            sprintf(seckey_str+i*2, "%02X", stream->seckey[i]);

        if (!conninfo->rawmode)
        {
            httpd_printf(conninfo->connid, "{\"url\":\"/stream/%u\",\"id\":%u,\"seckey\":\"%s\"}", stream->streamid, stream->streamid, seckey_str);
        }
        else
        {   // attach conn to stream
            stream_set_connection(stream, conninfo->connid);
            idset_insert(conninfo->streamids, stream->streamid);
            stream_refresh_timer(stream);   // stop from timing out
        }
    }
    if (!conninfo->rawmode)
        httpd_close(conninfo->connid);

    return stream;
}

void webapi_subscribe_cb(int mid)
{
    uint32_t connid;
    ebb_connection_info *conninfo;

    if (0 == midconn_find(mid, &connid))
    {
        if (NULL == (conninfo = httpd_get_conninfo(connid)))
        {
            LOG_ERROR("bad connid");
            return;
        }

        LOG_INFO("webapi_subscribe_cb: managed_str=%s", conninfo->managed_str);
        webapi_subscribe_response(conninfo, conninfo->managed_str);
    }
    else
    {
        LOG_ERROR("webapi_subscribe_cb mid not found %d", mid);
    }
    midconn_remove(mid);
}

static int parse_query_qos(const char *key, const char *val, void *userdata)
{
    int *qos = (int *)userdata;
    if (0==strcmp(key, "qos"))
    {
        *qos = atoi(val);
        return 0;
    }
    return 1;
}

static int json_fetch_keyval(const char *jsonstr, char **key, char **val)
{
    cJSON *root = NULL;
    cJSON *e;

    *key = NULL;
    *val = NULL;

    if (NULL == (root = cJSON_Parse(jsonstr)))
        goto fail;
    e = root->child;
    if (NULL == e || NULL == e->string || NULL == e->valuestring)
        goto fail;
    if (NULL == (*key = strdup(e->string)))
        goto fail;
    if (NULL == (*val = strdup(e->valuestring)))
        goto fail;
    if (NULL != root)
        cJSON_Delete(root);
    return 0;
fail:
    if (NULL != root)
        cJSON_Delete(root);
    if (NULL != *key)
        free(*key);
    if (NULL != *val)
        free(*val);
    return 1;
}

static int json_fetch_key(const char *jsonstr, char **key)
{
    cJSON *root = NULL;
    cJSON *e;

    *key = NULL;

    if (NULL == (root = cJSON_Parse(jsonstr)))
        goto fail;
    e = root->child;
    if (NULL == e || NULL == e->valuestring)
        goto fail;
    if (NULL == (*key = strdup(e->valuestring)))
        goto fail;

    if (NULL != root)
        cJSON_Delete(root);
    return 0;
fail:
    if (NULL != root)
        cJSON_Delete(root);
    if (NULL != *key)
        free(key);
    return 1;
}

static int json_parse_array_uint32_seckey(const char *jsonstr, uint32_t *arr, uint8_t seckeys[MAX_STREAMS][STREAM_SECKEY_SIZE], size_t *len, size_t max_len)
{
    cJSON *root = NULL;
    cJSON *e;
    size_t i = 0;
    size_t hlen;

     if (NULL == (root = cJSON_Parse(jsonstr)))
        goto fail;
    e = root->child;
    if (NULL == e)
        goto fail;

    while(NULL != e)
    {
        if (i > max_len)
            goto fail;
        if (NULL == e->string)
        {
            LOG_INFO("no seckey");
            goto fail;
        }
        arr[i] = atoi(e->string);
        hlen = STREAM_SECKEY_SIZE;
        if (0 != hexstring_parse(e->valuestring, seckeys[i], &hlen) || hlen != STREAM_SECKEY_SIZE)
        {
            LOG_INFO("bad seckey %s", e->valuestring);
            goto fail;
        }
        i++;
        e = e->next;
    }
    *len = i;
    if (NULL != root)
        cJSON_Delete(root);
    return 0;
fail:
    if (NULL != root)
        cJSON_Delete(root);
    return 1;
}

static int webapi_publish(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, const char *uri_str, const char *auth)
{
    ebb_connection_info *conninfo;
    int mid;
    int qos = 2;
    char *topic = NULL;
    char *msg = NULL;

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid");
        return 1;
    }

    if (method != EBB_PUT && method != EBB_POST)
    {
        conninfo->http_status = 405;    // method not allowed
        return 1;
    }

    httpd_query_iterate(uri_str, parse_query_qos, (void *)&qos);
    if (qos < 0 || qos > 2)
    {
        conninfo->http_status = 400;
        return 1;
    }

    if (0 != json_fetch_keyval(body, &topic, &msg))
    {
        LOG_INFO("malformed json %s", body);
        conninfo->http_status = 400;
        return 1;
    }

accesslog_write("%s publish %s", conninfo->ipaddr, topic);

    if (mqtt_publish(&g_srvctx.mqttctx, topic, msg, qos, &mid) != 0)
    {
        conninfo->http_status = 503;
        free(topic);
        free(msg);
        return 1;
    }
    else
    {
#ifdef LOCAL_ECHO
        // feed self
        webapi_message_cb(mid, topic, msg);
#endif
        free(topic);
        free(msg);
        if (qos == 0)
        {
            httpd_close(connid);
            return 0;
        }
        else
        {
            LOG_DEBUG("midconn_insert mid=%d conn=%d", mid, connid);
            midconn_insert(mid, connid);
            conninfo->have_mid = true;
            conninfo->mid = mid;
            return 0;
        }
    }

    return 1;
}


static int webapi_stream(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, const char *uri_str, const char *auth)
{
    ebb_connection_info *conninfo;

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid");
        return 1;
    }

    if (method == EBB_POST && argc == 0)
    {
        uint32_t streamids[MAX_STREAMS];
        stream_t *streams[MAX_STREAMS];
        uint8_t seckeys[MAX_STREAMS][STREAM_SECKEY_SIZE];

        size_t num_streams;
        size_t i;
        bool close_conn = false;

        if (0 != json_parse_array_uint32_seckey(body, streamids, seckeys, &num_streams, MAX_STREAMS) || num_streams == 0)
        {
            conninfo->http_status = 400;    // malformed
            return 1;
        }

        for (i=0;i<num_streams;i++) // reject request if any of the streams don't exist or wrong seckey
        {
//LOG_INFO("webapi_stream %u for connid=%u", streamids[i], connid);
            if (NULL == (streams[i] = streamlist_find(streamids[i])))
            {
                LOG_DEBUG("no such stream %u", streamids[i]);
                conninfo->http_status = 404;    // no such
                return 1;
            }
            else
            {
                if (0!=memcmp(seckeys[i], streams[i]->seckey, STREAM_SECKEY_SIZE))
                {
                    LOG_INFO("invalid seckey for streamid %u", streamids[i]);
                    conninfo->http_status = 403;    // verboten
                    return 1;
                }
                LOG_DEBUG("adding stream %u", streamids[i]);
            }
        }

        for (i=0;i<num_streams;i++)
        {
            if (!stream_isempty(streams[i]))
            {
                if (close_conn)
                    httpd_printf(connid, "\r\n");   // FIXME, should this be a single JSON doc?
                httpd_printf(connid, "[");
                stream_drain(streams[i], stream_drainer, (void *)(uintptr_t)connid);
                httpd_printf(connid, "]");
                close_conn = true;  // defer close till all available streams have been drained
            }
            else
            {
                uint32_t old_connid;
                if (stream_get_connection(streams[i], &old_connid))
                {
                    ebb_connection_info *old_conninfo;
                    LOG_DEBUG("closing old_connid %u replacing with %u for stream %u", old_connid, connid, streamids[i]);
                    if (NULL != (old_conninfo = httpd_get_conninfo(old_connid)))
                        idset_foreach(old_conninfo->streamids, strm_clr, NULL);
                    httpd_close(old_connid);
                }

                stream_set_connection(streams[i], connid);
                // associate the stream to this connection
                if (0 != idset_insert(conninfo->streamids, streamids[i]))
                {
                    conninfo->http_status = 503;
                    return 1;
                }
                stream_refresh_timer(streams[i]);   // stop from timing out
            }
        }

        if (close_conn)
        {
            LOG_DEBUG("closing");
            httpd_close(connid);
        }

        return 0;
    }
    else
    {
        conninfo->http_status = 405;    // method not allowed
        return 1;
    }

    return 0;
}

static void inject_retained(const char *topic, const char *msg, void *userdata)
{
    stream_t *stream = (stream_t *)userdata;
    struct topic_message tm = {topic, msg};

    if (NULL == stream)
        return;

    if (topic_match_string(stream->topic_patt, topic))
    {
        LOG_INFO("inj %s %s", topic, msg);
        message_to_stream(stream->streamid, stream, &tm);
    }
}

static int webapi_subscribe(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, const char *uri_str, const char *auth)
{
    ebb_connection_info *conninfo;
    int mid;
    char *topic = NULL;

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid");
        return 1;
    }

    if (method != EBB_PUT && method != EBB_POST)
    {
        conninfo->http_status = 405;    // method not allowed
        return 1;
    }

    if (0 != json_fetch_key(body, &topic))
    {
        LOG_INFO("malformed sub %s", body);
        conninfo->http_status = 400;
        return 1;
    }

LOG_INFO("webapi_subscribe %s", topic);
accesslog_write("%s subscribe %s", conninfo->ipaddr, topic);
    if (sublist_insert(topic))
    {
        if (mqtt_subscribe(&g_srvctx.mqttctx, topic, 2, &mid) != 0)
        {
            conninfo->http_status = 503;
            free(topic);
            return 1;
        }
        else
        {
            LOG_INFO("midconn_insert mid=%d conn=%d", mid, connid);
            conninfo->managed_str = topic;
            midconn_insert(mid, connid);
            conninfo->have_mid = true;
            conninfo->mid = mid;
            return 0;
        }
    }
    else
    {
        stream_t *stream;
        // we're already subscribed, so no need to send mqtt subscribe req
        stream = webapi_subscribe_response(conninfo, topic);
        // for everything in msgcache, if matches topic, inject into conn
        // FIXME, should walk topic tree
        msgcache_foreach(inject_retained, stream);

        free(topic);
        return 0;
    }

    return 1;
}

static int webapi_raw(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, const char *uri_str, const char *auth)
{
    ebb_connection_info *conninfo;
    int mid;
    char *topic = NULL;

    if (NULL == (topic = strdup(reqpath)))
    {
        LOG_ERROR("out of mem");
        return 1;
    }

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid");
        free(topic);
        return 1;
    }

    if (method != EBB_GET)
    {
        conninfo->http_status = 405;    // method not allowed
        free(topic);
        return 1;
    }

    conninfo->rawmode = true;
    strcpy(conninfo->mimetype, "text/html");

    if (sublist_insert(topic))
    {
        if (mqtt_subscribe(&g_srvctx.mqttctx, topic, 2, &mid) != 0)
        {
            conninfo->http_status = 503;
            free(topic);
            return 1;
        }
        else
        {
            LOG_INFO("midconn_insert mid=%d conn=%d", mid, connid);
            conninfo->managed_str = topic;
            midconn_insert(mid, connid);
            conninfo->have_mid = true;
            conninfo->mid = mid;
            return 0;
        }
    }
    else
    {
        stream_t *stream;
        // we're already subscribed, so no need to send mqtt subscribe req
        stream = webapi_subscribe_response(conninfo, topic);
        // for everything in msgcache, if matches topic, inject into conn
        // FIXME, should walk topic tree
        msgcache_foreach(inject_retained, stream);
        free(topic);
        return 0;
    }
    return 1;
}

static int webapi_command(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, const char *uri_str, const char *auth, int vhost)
{
//    if (vhost >=0 && 0 == strcmp(g_srvctx.vhttpd[vhost].name, "sensemote"))
    {
        if (argc > 0)
        {
            if (0 == strcmp(argv[0], "publish"))
                return webapi_publish(connid, method, argc-1, argv+1, body, reqpath + strlen("/publish/"), uri_str, auth);
            else
            if (0 == strcmp(argv[0], "subscribe"))
                return webapi_subscribe(connid, method, argc-1, argv+1, body, reqpath + strlen("/subscribe/"), uri_str, auth);
            else
            if (0 == strcmp(argv[0], "stream"))
                return webapi_stream(connid, method, argc-1, argv+1, body, reqpath + strlen("/stream/"), uri_str, auth);
            else
            if (0 == strcmp(argv[0], "raw"))
                return webapi_raw(connid, method, argc-1, argv+1, body, reqpath + strlen("/raw/"), uri_str, auth);

        }
    }
    if (0 == webapi_serve(connid, method, argc, argv, body, reqpath, vhost))
        return 0;
    else
    {
        ebb_connection_info *conninfo;
        if (NULL != (conninfo = httpd_get_conninfo(connid)))
        {
            conninfo->http_status = 404;
            return 1;
        }
    }
    return 1;
}
void httpd_open_cb(uint32_t connid)
{
    LOG_DEBUG("HTTPDCONN OPEN %d", connid);
}

void httpd_close_cb(uint32_t connid)
{
    ebb_connection_info *conninfo;

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid in httpd_close_cb!");
        return;
    }

    LOG_DEBUG("HTTPDCONN CLOSE %u", connid);

    if (conninfo->have_mid)
    {
        midconn_remove(conninfo->mid);
        conninfo->have_mid = false;
    }

    idset_foreach(conninfo->streamids, strm_clr, NULL);
}

// path will always start with '/', may end with it
int httpd_request_cb(uint32_t connid, const char *host, uint32_t method, const char *uri_str, const char *path, const char *body_data, size_t body_len, const char *auth)
{
    size_t i = 0;
    char *pathstr = NULL;
    char *bodystr = NULL;
    char *path_argv[MAX_PATH_ELEMS];
    int path_argc = 0;
    int rc;
    ebb_connection_info *conninfo;
    int vhost;

    LOG_DEBUG("httpd_request_cb host=%s uri_str=%s, path=%s, auth=%s", host, uri_str, path, auth);

    if ((vhost = httpd_lookupvirt(host)) < 0)
    {
        LOG_DEBUG("VHOST DEFAULT");
    }
    else
    {
        LOG_DEBUG("VHOST = %s", g_srvctx.vhttpd[vhost].name);
    }

    if (NULL == (conninfo = httpd_get_conninfo(connid)))
    {
        LOG_ERROR("bad connid");
        return 1;
    }

    // dissociate conn from streams, left from last req
    idset_foreach(conninfo->streamids, strm_clr, NULL);
    // FIXME, may be other inter-req cleanup to do here also

    if (NULL == (pathstr = strdup(path)))
        goto fail;
    
    if (NULL == (bodystr = (char *)calloc(1, body_len+1)))
    {
        LOG_ERROR("out of mem for bodystr");
        goto fail;
    }
    memcpy(bodystr, body_data, body_len);

    while(i < strlen(path))
    {
        if (pathstr[i] == '/')
        {
            pathstr[i] = 0;
            if (path_argc >= MAX_PATH_ELEMS)
            {
                LOG_ERROR("too many path elems");
                goto fail;
            }
            path_argv[path_argc++] = pathstr + i + 1;
        }
        i++;
    }

    // chop off any empty / element
    if (path_argc > 0)
    {
        if (*path_argv[path_argc-1] == 0 || *path_argv[path_argc-1] == '/')
            path_argc--;
    }

    conninfo->rawmode = false;

    rc = webapi_command(connid, method, path_argc, path_argv, bodystr, path, uri_str, auth, vhost);

    conninfo->ifmodifiedsince = 0;  // FIXME, need to reset this after, should be doing these things more cleanly

    free(pathstr);
    free(bodystr);
    return rc;
fail:
    if (NULL != pathstr)
        free(pathstr);
    if (NULL != bodystr)
        free(bodystr);
    return 1;
}


