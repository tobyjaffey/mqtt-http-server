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
#include <common/cJSON.h>
#include <common/path.h>
#include "mqtthttpd.h"
#include "httpd.h"
#include "webapi.h"
#include "webapi_serve.h"
#include "httpdconnlist.h"
#include "accesslog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/limits.h>

#include <uriparser/Uri.h>

#define DEFAULT_FILENAME "index.html"

extern server_context_t g_srvctx;

/*************** mimetype handling */
typedef struct
{
    const char *ext;
    const char *mime;
} filext_t;

static filext_t extensions[] =
{
    {".html", "text/html"},
    {".js", "application/x-javascript"},
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".mpg", "video/mpeg"},
    {".c", "text/plain"},
    {".h", "text/plain"},
    {".gz", "text/plain"},
    {".zip", "text/plain"},
    {NULL, "text/plain"}
};

static const char *lookup_mimetype(const char *filename)
{
    filext_t *p = extensions;
    while(p->ext != NULL)
    {
        if (NULL != strcasestr(filename, p->ext))
            break;
        p++;
    }
    return p->mime;
}
/************************************/

int webapi_serve(uint32_t connid, uint32_t method, int argc, char **argv, const char *body, const char *reqpath, int vhost)
{
    int fd = -1;
    size_t count;
    ebb_connection *conn;
    ebb_connection_info *conninfo = NULL;
    struct stat st;
    char path[PATH_MAX];
    uint8_t buf[512];
    const char *rfc1123fmt = "%a, %d %b %Y %H:%M:%S GMT";
    char reqpath_fname[4096];

    LOG_DEBUG("webapi_serve %s", reqpath);

    if (strlen(reqpath) > sizeof(reqpath_fname)/2)  // FIXME
    {
        LOG_INFO("path too long");
        return 1;
    }
    if (0 != uriUriStringToUnixFilenameA(reqpath, reqpath_fname))
    {
        LOG_INFO("bad req %s", reqpath);
        return 1;
    }

    reqpath = reqpath_fname;

    if (NULL == (conn = httpdconnlist_find(connid)))
    {
        LOG_WARN("no such connid %d", connid);
        goto fail;
    }
    conninfo = (ebb_connection_info *)(conn->data);
    if (NULL == conninfo)
        goto fail;

    if (method != EBB_GET)
        goto fail;

    if (NULL == reqpath || reqpath[0] == 0)
        reqpath = "/";

    if (vhost < 0)
    {
        snprintf(path, sizeof(path), "%s%s", g_srvctx.httpd_root, reqpath);
        if (!path_in_dir(g_srvctx.httpd_root, path))
        {
            LOG_DEBUG("request for %s out of root (%s)", path, g_srvctx.httpd_root);
            goto fail;
        }
    }
    else
    {
        snprintf(path, sizeof(path), "%s%s", g_srvctx.vhttpd[vhost].rootdir, reqpath);
        if (!path_in_dir(g_srvctx.vhttpd[vhost].rootdir, path))
        {
            LOG_DEBUG("request for %s out of root (%s)", path, g_srvctx.vhttpd[vhost].rootdir);
            goto fail;
        }
    }

    if ((fd = open(path, O_RDONLY)) < 0)
        goto fail;
    if (fstat(fd, &st) < 0)
        goto fail;

    if (S_ISDIR(st.st_mode))
    {
        char path2[PATH_MAX];
        strcpy(path2, path);
        snprintf(path, sizeof(path), "%s%s", path2, DEFAULT_FILENAME);
        close(fd);
        fd = -1;
        if (vhost < 0)
        {
            if (!path_in_dir(g_srvctx.httpd_root, path))
            {
                LOG_DEBUG("request for %s out of root", path);
                goto fail;
            }
        }
        else
        {
            if (!path_in_dir(g_srvctx.vhttpd[vhost].rootdir, path))
            {
                LOG_DEBUG("request for %s out of root", path);
                goto fail;
            }
        }
        if ((fd = open(path, O_RDONLY)) < 0)
            goto fail;
        if (fstat(fd, &st) < 0)
            goto fail;
    }

    if(!S_ISREG(st.st_mode))
        goto fail;

    strcpy(conninfo->mimetype, lookup_mimetype(path));

    // fill out a last-modified header
    strftime( conninfo->last_modified, sizeof(conninfo->last_modified), rfc1123fmt, localtime( &st.st_mtime ) );

    if (st.st_mtime <= conninfo->ifmodifiedsince && conninfo->ifmodifiedsince != 0)
    {
//        LOG_INFO("not modified since ifms=%u mtime=%u", conninfo->ifmodifiedsince, st.st_mtime);
        conninfo->http_status = 304;    // not modified
        LOG_DEBUG("not modified %s", path);
        httpd_close(connid);
        if (fd != 1)
            close(fd);
        return 0;
    }

    while((count = read(fd, buf, sizeof(buf)-1)) > 0)
        httpd_pushbin(connid, buf, count, false);

    httpd_close(connid);
    LOG_DEBUG("served %s", path);

    accesslog_write("%s %s %s", conninfo->ipaddr, path, reqpath);

    if (fd != -1)
        close(fd);
    return 0;
fail:
    if (fd != -1)
        close(fd);
    if (NULL != conninfo)
        conninfo->http_status = 404;    // not found
    return 1;
}


