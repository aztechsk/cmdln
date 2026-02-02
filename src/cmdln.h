/*
 * cmdln.h
 *
 * Copyright (c) 2020 Jan Rusnak <jan@rusnak.sk>
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

#ifndef CMDLN_H
#define CMDLN_H

#ifndef CMDLN_PARSER
 #define CMDLN_PARSER 0
#endif

#if CMDLN_PARSER == 1

extern const char *const cmd_accp;

/**
 * add_command_noargs
 */
void add_command_noargs(const char *p_name, void (*p_handler)(void));

/**
 * add_command_boolean
 */
void add_command_boolean(const char *p_name, void (*p_handler)(boolean_t));

/**
 * add_command_char
 */
void add_command_char(const char *p_name, void (*p_handler)(char));

/**
 * add_command_int
 */
void add_command_int(const char *p_name, void (*p_handler)(int));

/**
 * add_command_char_int
 */
void add_command_char_int(const char *p_name, void (*p_handler)(char, int));

/**
 * add_command_string
 */
void add_command_string(const char *p_name, void (*p_handler)(const char *));

/**
 * add_command_char_string
 */
void add_command_char_string(const char *p_name, void (*p_handler)(char, const char *));

/**
 * add_command_int_string
 */
void add_command_int_string(const char *p_name, void (*p_handler)(int, const char *));

/**
 * parse_line
 */
void parse_line(char *line);

/**
 * cmdln_hlp
 */
void cmdln_hlp(void);
#endif

#endif
