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
#ifndef LOGGING_H
#define LOGGING_H 1

#include <stdarg.h>

#define LOGLEVEL_CRITICAL (-2)
#define LOGLEVEL_USER  (-1)
#define LOGLEVEL_ERROR 0
#define LOGLEVEL_WARN  2
#define LOGLEVEL_INFO  3
#define LOGLEVEL_DEBUG 4

#define LOG_CRITICAL(...) log_msg(LOGLEVEL_CRITICAL, __FUNCTION__, __VA_ARGS__)
#define LOG_USER(...) log_msg(LOGLEVEL_USER, __FUNCTION__, __VA_ARGS__)
#define LOG_ERROR(...) log_msg(LOGLEVEL_ERROR, __FUNCTION__, __VA_ARGS__)
#define LOG_WARN(...) log_msg(LOGLEVEL_WARN, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO(...) log_msg(LOGLEVEL_INFO, __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG(...) log_msg(LOGLEVEL_DEBUG, __FUNCTION__, __VA_ARGS__)

#define LOG_USER_NOLN(...) log_msg_noln(LOGLEVEL_USER, __FUNCTION__, __VA_ARGS__)
#define LOG_ERROR_NOLN(...) log_msg_noln(LOGLEVEL_ERROR, __FUNCTION__, __VA_ARGS__)
#define LOG_WARN_NOLN(...) log_msg_noln(LOGLEVEL_WARN, __FUNCTION__, __VA_ARGS__)
#define LOG_INFO_NOLN(...) log_msg_noln(LOGLEVEL_INFO, __FUNCTION__, __VA_ARGS__)
#define LOG_DEBUG_NOLN(...) log_msg_noln(LOGLEVEL_DEBUG, __FUNCTION__, __VA_ARGS__)

void log_init(void);
void log_setlevel(int level);
void log_msg(int level, const char *func, const char *format, ...);
void log_msg_noln(int level, const char *func, const char *format, ...);

#endif

