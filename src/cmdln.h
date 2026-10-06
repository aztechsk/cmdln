/*
 * cmdln.h
 *
 * Copyright (c) 2026 Jan Rusnak <jan@rusnak.sk>
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

#include <gentyp.h>

/**
 * @file cmdln.h
 * @brief Runtime command registration and command-line parsing.
 *
 * The module registers named commands with fixed handler signatures and
 * dispatches mutable command lines to matching handlers.
 *
 * Supported handler parameter forms:
 * - no parameters,
 * - boolean,
 * - character,
 * - integer,
 * - character + integer,
 * - string,
 * - character + string,
 * - integer + string.
 *
 * Registration rules:
 * - Commands may be registered from task context at any time, including while
 *   parsing is active.
 * - Concurrent registration of different command names is supported. A newly
 *   registered command becomes visible only after registration is complete.
 * - Concurrent registration of the same command name is a programming error.
 * - Command names must be non-empty, contain no ASCII space character (`' '`),
 *   and not exceed `TERMIN_MAX_ROW_LENGTH`.
 * - Command names are not copied. The supplied name must remain valid and
 *   unchanged for the lifetime of the registration.
 * - Registered commands are permanent; the module provides no unregister API.
 *
 * Parsing rules:
 * - @ref parse_line is designed for one TIN/parser task and is not reentrant.
 * - The input line must be writable and NUL-terminated within
 *   `TERMIN_MAX_ROW_LENGTH`; longer input is not dispatched.
 * - Parsing modifies the supplied line buffer in place.
 * - Arguments are separated by ASCII space characters (`' '`). Strings without
 *   spaces may be unquoted; strings containing spaces must be enclosed by
 *   `CMDLN_STRING_DELIMITER`.
 * - Command names and textual boolean values are case-sensitive.
 * - Command handlers are invoked synchronously in the context of the task
 *   calling @ref parse_line.
 * - A string handler receives a pointer into the supplied line buffer. The
 *   handler must copy the string if it must outlive that buffer.
 *
 * @note Invalid registration arguments, detected duplicate command names,
 *       allocation failures, and a NULL input line are fatal programming/runtime
 *       errors handled by `crit_err_exit()`.
 */

#ifndef CMDLN_PARSER
#define CMDLN_PARSER 0
#endif

#if CMDLN_PARSER == 1

/** @brief Command acceptance marker string. */
extern const char *const cmd_accp;

/**
 * @brief Register a command with no parameters.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Command handler; must not be NULL.
 */
void add_command_noargs(const char *p_name, void (*p_handler)(void));

/**
 * @brief Register a command with one boolean parameter.
 *
 * Accepted false values are `0`, `off`, and `false`; accepted true values are
 * `1`, `on`, and `true`.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the parsed boolean value; must not be NULL.
 */
void add_command_boolean(const char *p_name, void (*p_handler)(boolean_t));

/**
 * @brief Register a command with one character parameter.
 *
 * The parameter must be a single alphabetic character.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the parsed character; must not be NULL.
 */
void add_command_char(const char *p_name, void (*p_handler)(char));

/**
 * @brief Register a command with one integer parameter.
 *
 * The parameter is parsed as a base-10 value and must fit in an @c int.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the parsed integer; must not be NULL.
 */
void add_command_int(const char *p_name, void (*p_handler)(int));

/**
 * @brief Register a command with character and integer parameters.
 *
 * The first parameter must be a single alphabetic character. The second is
 * parsed as a base-10 value and must fit in an @c int.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the parsed character and integer; must not be NULL.
 */
void add_command_char_int(const char *p_name, void (*p_handler)(char, int));

/**
 * @brief Register a command with one string parameter.
 *
 * A string without spaces may be unquoted. A string containing spaces must be
 * enclosed by `CMDLN_STRING_DELIMITER`.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving a pointer into the input line buffer; must not be NULL.
 */
void add_command_string(const char *p_name, void (*p_handler)(const char *));

/**
 * @brief Register a command with character and string parameters.
 *
 * The first parameter must be a single alphabetic character. The string follows
 * the quoting rules described for @ref add_command_string.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the character and a pointer into the input line buffer;
 *                   must not be NULL.
 */
void add_command_char_string(const char *p_name, void (*p_handler)(char, const char *));

/**
 * @brief Register a command with integer and string parameters.
 *
 * The first parameter is parsed as a base-10 value and must fit in an @c int.
 * The string follows the quoting rules described for @ref add_command_string.
 *
 * @param p_name     Non-empty command name; the pointer is retained by the module.
 * @param p_handler  Handler receiving the integer and a pointer into the input line buffer;
 *                   must not be NULL.
 */
void add_command_int_string(const char *p_name, void (*p_handler)(int, const char *));

/**
 * @brief Parse and dispatch one command line.
 *
 * The line is tokenized in place and may be modified before the handler is
 * invoked. Input longer than `TERMIN_MAX_ROW_LENGTH` is not dispatched.
 *
 * @param line Writable, NUL-terminated command line; must not be NULL.
 *
 * @note This function is intended to be called by one TIN/parser task only and
 *       is not reentrant.
 */
void parse_line(char *line);

/**
 * @brief Print the names of all registered commands.
 *
 * @note This function must be called from task context.
 */
void cmdln_hlp(void);

#endif

#endif
