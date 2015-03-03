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

extern const char *tester_file_prefix;
extern const char *tester_writable_dir_prefix;

#define CHECK_ARG(argument, index, argc)                      \
if(index >= argc) {                                           \
fprintf(stderr, "Missing argument for \"%s\"\n", argument);   \
return -1;                                                    \
}                                                             \

int tester_parse_args(int argc, char** argv, int argid, const char * additionnal_helper);
int tester_start();


int tester_test_suite_index(const char *suite_name);
void tester_list_suites();
void tester_list_suite_tests(const char *suite_name);
int tester_test_index(const char *suite_name, const char *test_name);
int tester_nb_test_suites(void);
int tester_nb_tests(const char *suite_name);
const char * tester_test_suite_name(int suite_index);
const char * tester_test_name(const char *suite_name, int test_index);
int tester_run_tests(const char *suite_name, const char *test_name);

void tester_add_suite(test_suite_t *suite);
int tester_run_suite(test_suite_t *suite);

void tester_uninit();


#ifdef __cplusplus
}
#endif
#endif
