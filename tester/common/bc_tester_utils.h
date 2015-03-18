/*
	tester - liblinphone test suite
	Copyright (C) 2013  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef TESTER_UTILS_H
#define TESTER_UTILS_H

#include "CUnit/Basic.h"
#include <stdarg.h>

extern const char *bc_tester_read_dir_prefix;
extern const char *bc_tester_writable_dir_prefix;

typedef void (*test_function_t)(void);
typedef int (*test_suite_function_t)(const char *name);

typedef struct {
	const char *name;
	test_function_t func;
} test_t;

typedef struct {
	const char *name;
	CU_InitializeFunc init_func;
	CU_CleanupFunc cleanup_func;
	int nb_tests;
	test_t *tests;
} test_suite_t;

#ifdef __cplusplus
extern "C" {
#endif


#define CHECK_ARG(argument, index, argc)                      \
if(index >= argc) {                                           \
fprintf(stderr, "Missing argument for \"%s\"\n", argument);   \
return -1;                                                    \
}                                                             \

void bc_tester_init(void (*ftester_printf)(int level, const char *fmt, va_list args)
					, int verbosity_info, int verbosity_error);
void bc_tester_helper(const char *name, const char* additionnal_helper);
int bc_tester_parse_args(int argc, char** argv, int argid);
int bc_tester_start();
void bc_tester_add_suite(test_suite_t *suite);
void bc_tester_uninit();

int bc_tester_nb_suites();
int bc_tester_nb_tests(const char* name);
void bc_tester_list_suites();
void bc_tester_list_tests(const char *suite_name);
const char * bc_tester_suite_name(int suite_index);
const char * bc_tester_test_name(const char *suite_name, int test_index);
int bc_tester_run_suite(test_suite_t *suite);
int bc_tester_run_tests(const char *suite_name, const char *test_name);
int bc_tester_suite_index(const char *suite_name);

#ifdef __cplusplus
}
#endif
#endif
