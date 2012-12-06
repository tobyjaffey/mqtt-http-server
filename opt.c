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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <common/logging.h>
#include <common/util.h>
#include <common/jitter.h>
#include <common/pidfile.h>
#include <common/random.h>

#include "mqtthttpd.h"
#include "mqtt.h"
#include "httpd.h"
#include "stream.h"
#include "streamlist.h"
#include "accesslog.h"
#include "opt.h"

static char *mqtt_server = NULL;
static int mqtt_port = -1;
static int httpd_port = -1;
static char *httpd_root = NULL;
static int verbose = 0;
static char *client_name = NULL;
static char *cert_file = NULL;
static char *key_file = NULL;

static struct option long_options[] =
{
    {"help",    no_argument, 0, 'h'},
    {"device",     required_argument, 0, 'd'},
    {"server",     required_argument, 0, 's'},
    {"port",       required_argument, 0, 'p'},
    {"verbose",    no_argument, 0, 'v'},
    {"name",      required_argument, 0, 'n'},
    {"root",      required_argument, 0, 'r'},
    {"listen",      required_argument, 0, 'l'},
    {"sslcert",      required_argument, 0, 'c'},
    {"sslkey",      required_argument, 0, 'k'},
    {0, 0, 0, 0}
};

extern server_context_t g_srvctx;

void usage(void)
{
    fprintf(stderr, "mqtthttpd\n");
    fprintf(stderr, "mqtthttpd -v -r htdocs -l 8080 -n foo -s test.mosquitto.org -p 1883\n");
    fprintf(stderr, "  --verbose                    -v     Chatty mode, -vv for more\n");
    fprintf(stderr, "  --server=test.mosquitto.org  -s     MQTT broker host\n");
    fprintf(stderr, "  --port=1883                  -p     MQTT broker port\n");
    fprintf(stderr, "  --name=mqttname              -n     MQTT client name\n");
    fprintf(stderr, "  --listen=8080                -l     HTTP server port\n");
    fprintf(stderr, "  --root=htdocs                -r     HTTP server root\n");
    fprintf(stderr, "  --sslcert=ca-cert.pem        -c     SSL certificate\n");
    fprintf(stderr, "  --sslkey=ca-key.pem          -k     SSL key\n");
}

int parse_options(int argc, char **argv)
{
    int c;
    int option_index;

    while(1)
    {
        c = getopt_long(argc, argv, "n:vhs:p:l:r:c:k:", long_options, &option_index);
        if (c == -1)
            break;
        switch(c)
        {
            case 'h':
                return 1;
            break;
            case 'c':
                cert_file = strdup(optarg);
            break;
            case 'k':
                key_file = strdup(optarg);
            break;
            case 'n':
                client_name = strdup(optarg);
            break;
            case 's':
                mqtt_server = strdup(optarg);
            break;
            case 'r':
                httpd_root = strdup(optarg);
            break;
            case 'p':
                mqtt_port = atoi(optarg);
            break;
            case 'l':
                httpd_port = atoi(optarg);
            break;
            case 'v':
                verbose++;
            break;
            default:
                return 1;
            break;
        }
    }

    if (NULL == client_name || NULL == mqtt_server || -1 == mqtt_port || -1 == httpd_port || NULL == httpd_root)
        return 1;

    if (verbose == 0)
        log_setlevel(LOGLEVEL_WARN);
    else
    if (verbose == 1)
        log_setlevel(LOGLEVEL_INFO);
    else
    if (verbose == 2)
        log_setlevel(LOGLEVEL_DEBUG);

    if (0 != mqtt_connect(&(g_srvctx.mqttctx), client_name, mqtt_server, mqtt_port))
        LOG_CRITICAL("Failed to connect to MQTT broker %s:%d", mqtt_server, mqtt_port);

    if (0 != httpd_listen(&(g_srvctx.httpd), httpd_root, httpd_port, cert_file, key_file))
        LOG_CRITICAL("Failed to start HTTP server on port %d, serving %s", httpd_port, httpd_root);
    return 0;
}



