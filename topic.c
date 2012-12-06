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

#include "topic.h"


topic_node_t *topic_create(void)
{
    topic_node_t *tn;
    tn = (topic_node_t *)malloc(sizeof(topic_node_t));
    tn->next = NULL;
    tn->str = NULL;
    return tn;
}

void topic_destroy(topic_node_t *tn)
{
    while(NULL != tn)
    {
        topic_node_t *next;
        next = tn->next;
        if (NULL != tn->str)
            free(tn->str);
        free(tn);
        tn = next;
    }
}

void topic_print(topic_node_t *root)
{
    while(NULL != root)
    {
        switch(root->type)
        {
            case TOPIC_NODE_TYPE_STR:
                LOG_USER_NOLN("%s", root->str);
            break;
            case TOPIC_NODE_TYPE_PLUS:
                LOG_USER_NOLN("+");
            break;
            case TOPIC_NODE_TYPE_HASH:
                LOG_USER_NOLN("#");
            break;
        }
        if (NULL != root->next)
            LOG_USER_NOLN("/");
        root = root->next;
    }
    LOG_USER("");
}

bool topic_match_string(topic_node_t *rule, const char *topicstr)
{
    const char *elem_start;
    size_t len;

    elem_start = topicstr;

    while(1)
    {
        if (NULL == rule)
            return false;

        if (*topicstr == '/' || *topicstr == 0)
        {
            switch(rule->type)
            {
                case TOPIC_NODE_TYPE_STR:
                    len = strlen(rule->str);
                    if (len != (topicstr - elem_start))
                        return false;
                    if (0!=memcmp(elem_start, rule->str, len))
                        return false;
                break;
                case TOPIC_NODE_TYPE_PLUS:
                    // matches
                break;
                case TOPIC_NODE_TYPE_HASH:
                    return true;
                break;
            }

            rule = rule->next;
            elem_start = topicstr+1;
        }

        if (0 == *topicstr++)
            break;
    }

    return true;
}

topic_node_t *topic_create_from_string(const char *topicstr)
{
    topic_node_t *root = NULL;
    topic_node_t *p = NULL;
    bool seen_hash = false;
    const char *elem_start;

    elem_start = topicstr;

    while(1)
    {
        if (*topicstr == '/' || *topicstr == 0)
        {
            if (seen_hash)  // hash must be last item
            {
                LOG_ERROR("Hierarchy should end at #");
                goto fail;
            }

            if (NULL == root)
            {
                if (NULL == (root = topic_create()))
                {
                    LOG_ERROR("create root failed");
                    goto fail;
                }
                p = root;
            }
            else
            {
                if (NULL == (p->next = topic_create()))
                {
                    LOG_ERROR("create child failed");
                    goto fail;
                }
                p = p->next;
            }

            if ((1 == topicstr - elem_start) && *elem_start == '+')
                p->type = TOPIC_NODE_TYPE_PLUS;
            else
            if ((1 == topicstr - elem_start) && *elem_start == '#')
                p->type = TOPIC_NODE_TYPE_HASH;
            else
            {
                p->type = TOPIC_NODE_TYPE_STR;
                if (NULL == (p->str = (char *)malloc((topicstr - elem_start)+1)))
                {
                    LOG_ERROR("string alloc");
                    goto fail;
                }
                memcpy(p->str, elem_start, topicstr - elem_start);
                p->str[topicstr - elem_start] = 0;
            }


            elem_start = topicstr+1;
        }

        if (0 == *topicstr++)
            break;
    }

    return root;
fail:
    topic_destroy(root);
    return NULL;
}

#if 0
int topic_test(void)
{
    topic_node_t *tn;
    const char *topic = "a/b/c/d";
    const char *good_rules[] = {
                                    "a/b/c/d", "+/b/c/d", "a/+/c/d", "a/+/+/d", "+/+/+/+",
                                    "#", "a/#", "a/b/#", "a/b/c/#", "+/b/c/#"
                               };
    const char *bad_rules[] =  {
                                    "a/b/c", "b/+/c/d", "+/+/",
                                    "b/#", "/#", "/", ""
                               };
    int good_rules_len = sizeof(good_rules) / sizeof(char *);
    int bad_rules_len = sizeof(bad_rules) / sizeof(char *);
    int i;

    for (i=0;i<good_rules_len;i++)
    {
        if (NULL == (tn = topic_create_from_string(good_rules[i])))
        {
            fprintf(stderr, "err creating\n");
            return 1;
        }

        if (!topic_match_string(tn, topic))
        {
            fprintf(stderr, "%s didn't match %s", good_rules[i], topic);
            return 1;
        }

        topic_destroy(tn);
    }

    for (i=0;i<bad_rules_len;i++)
    {
        if (NULL == (tn = topic_create_from_string(bad_rules[i])))
        {
            fprintf(stderr, "err creating\n");
            return 1;
        }

        if (topic_match_string(tn, topic))
        {
            fprintf(stderr, "%s did match %s", bad_rules[i], topic);
            return 1;
        }

        topic_destroy(tn);
    }

    return 0;
}
#endif


