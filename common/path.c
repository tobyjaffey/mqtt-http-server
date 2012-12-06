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
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <common/logging.h>
#include <common/path.h>

bool path_in_dir(const char *root, const char *path)
{
    char path_full[PATH_MAX];
    char root_full[PATH_MAX];

    if (strlen(root) >= PATH_MAX || strlen(path) >= PATH_MAX)
    {
        LOG_ERROR("too long");
        return false;
    }
    if (NULL == realpath(path, path_full))
    {
        LOG_DEBUG("bad path '%s'", path);
        return false;
    }
    if (NULL == realpath(root, root_full))
    {
        LOG_ERROR("bad root");
        return false;
    }
    if (strlen(root_full) > strlen(path_full))
    {
        LOG_ERROR("too short");
        return false;
    }
    if (0 != strncmp(root_full, path_full, strlen(root_full)))
    {
        LOG_ERROR("path root mismatch");
        return false;
    }
    return true;
}
