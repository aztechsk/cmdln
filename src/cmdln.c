/*
 * cmdln.c
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

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#include <gentyp.h>
#include "sysconf.h"
#include "criterr.h"
#include "atom.h"
#include "msgconf.h"
#include "fmalloc.h"
#include "cmdln.h"

#if CMDLN_PARSER == 1

#if TERMIN != 1
#error "cmdln.c depends on tin.c"
#endif

const char *const cmd_accp = ">>\n";

#define TOKENS_NUMB 3

enum command_type {
	COMMAND_NOARGS,
	COMMAND_BOOLEAN,
	COMMAND_CHAR,
	COMMAND_INT,
	COMMAND_CHAR_INT,
	COMMAND_STRING,
	COMMAND_CHAR_STRING,
	COMMAND_INT_STRING
};

struct command_descriptor {
	enum command_type type;
	const char *p_name;
	void (*p_handler)(void);
	struct command_descriptor *volatile next;
};

static struct command_descriptor *volatile descriptor_list;
static struct command_descriptor *volatile descriptor_tail;
static char *tokens[TOKENS_NUMB];
static int tokens_count;
static const char *const p_num_of_param_error = "bad number of parameters\n";
static const char *const p_parse_param_n_error = "parameter %d parse error\n";
static const char *const estr = "";

static void add_command(enum command_type t, const char *p_n, void (*p_h)(void));
static struct command_descriptor *create_command_descriptor(void);
static int find_tokens(char *line);
static void parse_command_noargs(struct command_descriptor *p_d);
static void parse_command_boolean(struct command_descriptor *p_d);
static void parse_command_char(struct command_descriptor *p_d);
static void parse_command_int(struct command_descriptor *p_d);
static void parse_command_char_int(struct command_descriptor *p_d);
static void parse_command_string(struct command_descriptor *p_d);
static void parse_command_char_string(struct command_descriptor *p_d);
static void parse_command_int_string(struct command_descriptor *p_d);
static char *parse_string(char *p);
static boolean_t parse_int(const char *p, int *v);

/**
 * add_command_noargs
 */
void add_command_noargs(const char *p_name, void (*p_handler)(void))
{
	add_command(COMMAND_NOARGS, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_boolean
 */
void add_command_boolean(const char *p_name, void (*p_handler)(boolean_t))
{
	add_command(COMMAND_BOOLEAN, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_char
 */
void add_command_char(const char *p_name, void (*p_handler)(char))
{
	add_command(COMMAND_CHAR, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_int
 */
void add_command_int(const char *p_name, void (*p_handler)(int))
{
	add_command(COMMAND_INT, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_char_int
 */
void add_command_char_int(const char *p_name, void (*p_handler)(char, int))
{
	add_command(COMMAND_CHAR_INT, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_string
 */
void add_command_string(const char *p_name, void (*p_handler)(const char *))
{
	add_command(COMMAND_STRING, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_char_string
 */
void add_command_char_string(const char *p_name, void (*p_handler)(char, const char *))
{
	add_command(COMMAND_CHAR_STRING, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command_int_string
 */
void add_command_int_string(const char *p_name, void (*p_handler)(int, const char *))
{
	add_command(COMMAND_INT_STRING, p_name, (void (*)(void)) p_handler);
}

/**
 * add_command
 */
static void add_command(enum command_type t, const char *p_n, void (*p_h)(void))
{
	struct command_descriptor *cd;

	if (p_n == NULL || *p_n == '\0' || p_h == NULL) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (strchr(p_n, ' ') != NULL || strlen(p_n) > TERMIN_MAX_ROW_LENGTH) {
		crit_err_exit(BAD_PARAMETER);
	}
	/*
	 * Concurrent registration of the same command name is a programming error.
	 */
	for (cd = descriptor_list; cd; cd = cd->next) {
		if (strcmp(cd->p_name, p_n) == 0) {
			crit_err_exit(UNEXP_PROG_STATE);
		}
	}
	cd = create_command_descriptor();
	cd->type = t;
	cd->p_handler = p_h;
	barrier();
	cd->p_name = p_n;
}

/**
 * create_command_descriptor
 */
static struct command_descriptor *create_command_descriptor(void)
{
	struct command_descriptor *p_n;

	p_n = pvPortMalloc(sizeof(struct command_descriptor));
	if (p_n == NULL) {
		crit_err_exit(MALLOC_ERROR);
	}
	p_n->p_name = estr;
	p_n->next = NULL;
	taskENTER_CRITICAL();
	if (descriptor_tail) {
		descriptor_tail->next = p_n;
	} else {
		descriptor_list = p_n;
	}
	descriptor_tail = p_n;
	taskEXIT_CRITICAL();
	return (p_n);
}

/**
 * parse_line
 */
void parse_line(char *line)
{
	struct command_descriptor *p_d;

	if (line == NULL) {
		crit_err_exit(BAD_PARAMETER);
	}
	if (!descriptor_list) {
		return;
	}
	tokens_count = find_tokens(line);
	if (!tokens_count) {
		return;
	}
	p_d = descriptor_list;
	do {
		if (strcmp(tokens[0], p_d->p_name) == 0) {
			break;
		}
		if (p_d->next) {
			p_d = p_d->next;
		} else {
			msg(INF, "unknown command\n");
			return;
		}
	} while (TRUE);
	switch (p_d->type) {
	case COMMAND_NOARGS :
		parse_command_noargs(p_d);
		break;
	case COMMAND_BOOLEAN :
		parse_command_boolean(p_d);
		break;
	case COMMAND_CHAR :
		parse_command_char(p_d);
		break;
	case COMMAND_INT :
		parse_command_int(p_d);
		break;
	case COMMAND_CHAR_INT :
		parse_command_char_int(p_d);
		break;
	case COMMAND_STRING :
		parse_command_string(p_d);
		break;
	case COMMAND_CHAR_STRING :
		parse_command_char_string(p_d);
		break;
	case COMMAND_INT_STRING :
		parse_command_int_string(p_d);
		break;
	}
}

/**
 * cmdln_hlp
 */
void cmdln_hlp(void)
{
	struct command_descriptor *p_d;
	int i = 0;
	UBaseType_t pr;
	boolean_t nl = TRUE;

	msg(INF, ">>\n");
	if (descriptor_list) {
		p_d = descriptor_list;
		pr = uxTaskPriorityGet(NULL);
		vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
		do {
			if (strcmp(estr, p_d->p_name) != 0) {
				if (!i) {
					msg(INF, "cmd> %s", p_d->p_name);
				} else {
					msg(INF, " %s", p_d->p_name);
				}
				if (++i == 5) {
					i = 0;
					msg(INF, "\n");
					nl = TRUE;
					vTaskDelay(200 / portTICK_PERIOD_MS);
				} else {
					nl = FALSE;
				}
			}
		} while ((p_d = p_d->next));
		if (!nl) {
			msg(INF, "\n");
		}
		vTaskPrioritySet(NULL, pr);
	}
}

/**
 * find_tokens
 */
static int find_tokens(char *line)
{
	int t, i, dlm_pos;
	boolean_t spc_mode, dlm_mode;

	for (i = 0; i < TOKENS_NUMB; i++) {
		tokens[i] = NULL;
	}
	t = 0;
	spc_mode = TRUE;
	dlm_mode = FALSE;
	dlm_pos = 0;
	for (i = 0; i < TERMIN_MAX_ROW_LENGTH + 1; i++) {
		if (*(line + i) == '\0') {
			break;
		}
		if (spc_mode) {
			if (*(line + i) == ' ') {
				continue;
			} else {
				spc_mode = FALSE;
				if (*(line + i) == CMDLN_STRING_DELIMITER) {
					dlm_mode = TRUE;
					dlm_pos = i;
				} else {
					dlm_mode = FALSE;
				}
				if (t < TOKENS_NUMB) {
					tokens[t++] = line + i;
				} else {
					t++;
				}
			}
		} else {
			if (dlm_mode) {
				if (*(line + i) == ' ') {
					if (*(line + i - 1) == CMDLN_STRING_DELIMITER) {
						if (dlm_pos != i - 1) {
							*(line + i) = '\0';
							spc_mode = TRUE;
						}
					}
				}
			} else {
				if (*(line + i) == ' ') {
					*(line + i) = '\0';
					spc_mode = TRUE;
				}
			}
		}
	}
	if (i > TERMIN_MAX_ROW_LENGTH) {
		return (0);
	}
	return (t);
}

/**
 * parse_command_noargs
 */
static void parse_command_noargs(struct command_descriptor *p_d)
{
	if (tokens_count == 1) {
		(*p_d->p_handler)();
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_boolean
 */
static void parse_command_boolean(struct command_descriptor *p_d)
{
	boolean_t b;

	if (tokens_count == 2) {
		if (strcmp(tokens[1], "0") == 0) {
			b = FALSE;
		} else if (strcmp(tokens[1], "off") == 0) {
			b = FALSE;
		} else if (strcmp(tokens[1], "false") == 0) {
			b = FALSE;
		} else if (strcmp(tokens[1], "1") == 0) {
			b = TRUE;
		} else if (strcmp(tokens[1], "on") == 0) {
			b = TRUE;
		} else if (strcmp(tokens[1], "true") == 0) {
			b = TRUE;
		} else {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		((void (*)(boolean_t)) p_d->p_handler)(b);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_char
 */
static void parse_command_char(struct command_descriptor *p_d)
{
	if (tokens_count == 2) {
		if (strlen(tokens[1]) != 1 || !isalpha((unsigned char) *tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		((void (*)(char)) p_d->p_handler)(*tokens[1]);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_int
 */
static void parse_command_int(struct command_descriptor *p_d)
{
	int n;

	if (tokens_count == 2) {
		if (!parse_int(tokens[1], &n)) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		((void (*)(int)) p_d->p_handler)(n);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_char_int
 */
static void parse_command_char_int(struct command_descriptor *p_d)
{
	int n;

	if (tokens_count == 3) {
		if (strlen(tokens[1]) != 1 || !isalpha((unsigned char) *tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (!parse_int(tokens[2], &n)) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		((void (*)(char, int)) p_d->p_handler)(*tokens[1], n);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_string
 */
static void parse_command_string(struct command_descriptor *p_d)
{
	char *p;

	if (tokens_count == 2) {
		p = parse_string(tokens[1]);
		if (p == NULL) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		((void (*)(const char *)) p_d->p_handler)(p);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_char_string
 */
static void parse_command_char_string(struct command_descriptor *p_d)
{
	char *p;

	if (tokens_count == 3) {
		if (strlen(tokens[1]) != 1 || !isalpha((unsigned char) *tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		p = parse_string(tokens[2]);
		if (p == NULL) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		((void (*)(char, const char *)) p_d->p_handler)(*tokens[1], p);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_int_string
 */
static void parse_command_int_string(struct command_descriptor *p_d)
{
	char *p;
	int n;

	if (tokens_count == 3) {
		if (!parse_int(tokens[1], &n)) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		p = parse_string(tokens[2]);
		if (p == NULL) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		((void (*)(int, const char *)) p_d->p_handler)(n, p);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_string
 */
static char *parse_string(char *p)
{
	size_t sz;

	if (*p != CMDLN_STRING_DELIMITER) {
		return (p);
	}
	sz = strlen(p);
	if (sz < 2 || *(p + sz - 1) != CMDLN_STRING_DELIMITER) {
		return (NULL);
	}
	*(p + sz - 1) = '\0';
	return (p + 1);
}

/**
 * parse_int
 */
static boolean_t parse_int(const char *p, int *v)
{
	char *end;
	long long n;

	if (p == NULL || v == NULL || *p == '\0' || isspace((unsigned char) *p)) {
		return (FALSE);
	}
	errno = 0;
	n = strtoll(p, &end, 10);
	if (errno == ERANGE || end == p || *end != '\0' || n < INT_MIN || n > INT_MAX) {
		return (FALSE);
	}
	*v = (int) n;
	return (TRUE);
}

#endif
