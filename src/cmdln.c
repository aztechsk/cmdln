/*
 * cmdln.c
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

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <gentyp.h>
#include "sysconf.h"
#include "criterr.h"
#include "atom.h"
#include "msgconf.h"
#include "fmalloc.h"
#include "cmdln.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#if CMDLN_PARSER == 1

#if TERMIN != 1
 #error "cmdln.c depends on tin.c"
#endif

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
static boolean_t valid_number_string(const char *p);

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
	struct command_descriptor *p_n, *p_d;

	if (NULL == (p_n = pvPortMalloc(sizeof(struct command_descriptor)))) {
		crit_err_exit(MALLOC_ERROR);
	}
        memset(p_n, 0, sizeof(struct command_descriptor));
	p_n->p_name = estr;
	taskENTER_CRITICAL();
	if (descriptor_list) {
		p_d = descriptor_list;
		while (p_d->next) {
			p_d = p_d->next;
		}
		p_d->next = p_n;
	} else {
		descriptor_list = p_n;
	}
	taskEXIT_CRITICAL();
	return (p_n);
}

/**
 * parse_line
 */
void parse_line(char *line)
{
	struct command_descriptor *p_d;

	if (!descriptor_list) {
		return;
	}
	if (!(tokens_count = find_tokens(line))) {
		return;
	}
	p_d = descriptor_list;
	do {
		if (0 == strcmp(tokens[0], p_d->p_name)) {
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
	case COMMAND_NOARGS      :
		parse_command_noargs(p_d);
		break;
	case COMMAND_BOOLEAN     :
		parse_command_boolean(p_d);
		break;
	case COMMAND_CHAR        :
		parse_command_char(p_d);
		break;
	case COMMAND_INT         :
		parse_command_int(p_d);
		break;
	case COMMAND_CHAR_INT    :
		parse_command_char_int(p_d);
		break;
	case COMMAND_STRING      :
		parse_command_string(p_d);
		break;
	case COMMAND_CHAR_STRING :
		parse_command_char_string(p_d);
		break;
	case COMMAND_INT_STRING  :
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
			if (0 != strcmp(estr, p_d->p_name)) {
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
		if (0 == strcmp(tokens[1], "0")) {
			b = FALSE;
		} else if (0 == strcmp(tokens[1], "off")) {
			b = FALSE;
		} else if (0 == strcmp(tokens[1], "false")) {
			b = FALSE;
		} else if (0 == strcmp(tokens[1], "1")) {
			b = TRUE;
		} else if (0 == strcmp(tokens[1], "on")) {
			b = TRUE;
		} else if (0 == strcmp(tokens[1], "true")) {
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
		if (1 != strlen(tokens[1]) || !isalpha(*tokens[1])) {
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
		if (!valid_number_string(tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (1 != sscanf(tokens[1], "%d", &n)) {
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
		if (1 != strlen(tokens[1]) || !isalpha(*tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (!valid_number_string(tokens[2])) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		if (1 != sscanf(tokens[2], "%d", &n)) {
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
	int sz;

	if (tokens_count == 2) {
		if ((sz = strlen(tokens[1])) < 2) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (*tokens[1] != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (*(tokens[1] + sz - 1) != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		*(tokens[1] + sz - 1) = '\0';
		((void (*)(const char *)) p_d->p_handler)(tokens[1] + 1);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_char_string
 */
static void parse_command_char_string(struct command_descriptor *p_d)
{
	int sz;

	if (tokens_count == 3) {
		if (1 != strlen(tokens[1]) || !isalpha(*tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if ((sz = strlen(tokens[2])) < 2) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		if (*tokens[2] != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		if (*(tokens[2] + sz - 1) != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		*(tokens[2] + sz - 1) = '\0';
		((void (*)(char, const char *)) p_d->p_handler)(*tokens[1], tokens[2] + 1);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * parse_command_int_string
 */
static void parse_command_int_string(struct command_descriptor *p_d)
{
	int sz, n;

	if (tokens_count == 3) {
		if (!valid_number_string(tokens[1])) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if (1 != sscanf(tokens[1], "%d", &n)) {
			msg(INF, p_parse_param_n_error, 1);
			return;
		}
		if ((sz = strlen(tokens[2])) < 2) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		if (*tokens[2] != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		if (*(tokens[2] + sz - 1) != CMDLN_STRING_DELIMITER) {
			msg(INF, p_parse_param_n_error, 2);
			return;
		}
		*(tokens[2] + sz - 1) = '\0';
		((void (*)(int, const char *)) p_d->p_handler)(n, tokens[2] + 1);
	} else {
		msg(INF, p_num_of_param_error);
	}
}

/**
 * valid_number_string
 */
static boolean_t valid_number_string(const char *p)
{
	int i = 0;

	while (TRUE) {
		if (*(p + i) == '\0') {
			break;
		}
		if (i == 0 && (*p == '+' || *p == '-')) {
			i++;
			continue;
		}
		if (!isdigit(*(p + i))) {
			return (FALSE);
		}
		i++;
	}
	return (TRUE);
}
#endif
