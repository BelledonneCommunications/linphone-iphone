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
#include <stdio.h>
#include <stdarg.h>

extern const char *bc_tester_read_dir_prefix;
extern const char *bc_tester_writable_dir_prefix;

extern int bc_printf_verbosity_info;
extern int bc_printf_verbosity_error;

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
void bc_tester_printf(int level, const char *fmt, ...);

int bc_tester_nb_suites();
int bc_tester_nb_tests(const char* name);
void bc_tester_list_suites();
void bc_tester_list_tests(const char *suite_name);
const char * bc_tester_suite_name(int suite_index);
const char * bc_tester_test_name(const char *suite_name, int test_index);
int bc_tester_run_suite(test_suite_t *suite);
int bc_tester_run_tests(const char *suite_name, const char *test_name);
int bc_tester_suite_index(const char *suite_name);


/**
 * Get full path to the given resource
 *
 * @param name relative resource path (relative to bc_tester_writable_dir_prefix)
 * @return path to the resource. Must be freed by caller.
*/
char * bc_tester_res(const char *name);

/*Redefine the CU_... macros WITHOUT final ';' semicolon, to allow IF conditions */
#define BC_ASSERT_EQUAL(actual, expected) CU_assertImplementation(((actual) == (expected)), __LINE__, ("CU_ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_PASS(msg) CU_assertImplementation(CU_TRUE, __LINE__, ("CU_PASS(" #msg ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT(value) CU_assertImplementation((value), __LINE__, #value, __FILE__, "", CU_FALSE)
#define BC_ASSERT_FATAL(value) CU_assertImplementation((value), __LINE__, #value, __FILE__, "", CU_TRUE)
#define BC_TEST(value) CU_assertImplementation((value), __LINE__, #value, __FILE__, "", CU_FALSE)
#define BC_TEST_FATAL(value) CU_assertImplementation((value), __LINE__, #value, __FILE__, "", CU_TRUE)
#define BC_ASSERT_TRUE(value) CU_assertImplementation((value), __LINE__, ("CU_ASSERT_TRUE(" #value ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_TRUE_FATAL(value) CU_assertImplementation((value), __LINE__, ("CU_ASSERT_TRUE_FATAL(" #value ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_FALSE(value) CU_assertImplementation(!(value), __LINE__, ("CU_ASSERT_FALSE(" #value ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_FALSE_FATAL(value) CU_assertImplementation(!(value), __LINE__, ("CU_ASSERT_FALSE_FATAL(" #value ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_EQUAL(actual, expected) CU_assertImplementation(((actual) == (expected)), __LINE__, ("CU_ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_EQUAL_FATAL(actual, expected) CU_assertImplementation(((actual) == (expected)), __LINE__, ("CU_ASSERT_EQUAL_FATAL(" #actual "," #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_NOT_EQUAL(actual, expected) CU_assertImplementation(((actual) != (expected)), __LINE__, ("CU_ASSERT_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_NOT_EQUAL_FATAL(actual, expected) CU_assertImplementation(((actual) != (expected)), __LINE__, ("CU_ASSERT_NOT_EQUAL_FATAL(" #actual "," #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_PTR_EQUAL(actual, expected) CU_assertImplementation(((const void*)(actual) == (const void*)(expected)), __LINE__, ("CU_ASSERT_PTR_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_PTR_EQUAL_FATAL(actual, expected) CU_assertImplementation(((const void*)(actual) == (const void*)(expected)), __LINE__, ("CU_ASSERT_PTR_EQUAL_FATAL(" #actual "," #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_PTR_NOT_EQUAL(actual, expected) CU_assertImplementation(((const void*)(actual) != (const void*)(expected)), __LINE__, ("CU_ASSERT_PTR_NOT_EQUAL(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_PTR_NOT_EQUAL_FATAL(actual, expected) CU_assertImplementation(((const void*)(actual) != (const void*)(expected)), __LINE__, ("CU_ASSERT_PTR_NOT_EQUAL_FATAL(" #actual "," #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_PTR_NULL(value) CU_assertImplementation((NULL == (const void*)(value)), __LINE__, ("CU_ASSERT_PTR_NULL(" #value")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_PTR_NULL_FATAL(value) CU_assertImplementation((NULL == (const void*)(value)), __LINE__, ("CU_ASSERT_PTR_NULL_FATAL(" #value")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_PTR_NOT_NULL(value) CU_assertImplementation((NULL != (const void*)(value)), __LINE__, ("CU_ASSERT_PTR_NOT_NULL(" #value")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_PTR_NOT_NULL_FATAL(value) CU_assertImplementation((NULL != (const void*)(value)), __LINE__, ("CU_ASSERT_PTR_NOT_NULL_FATAL(" #value")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_STRING_EQUAL(actual, expected) CU_assertImplementation(!(strcmp((const char*)(actual), (const char*)(expected))), __LINE__, ("CU_ASSERT_STRING_EQUAL(" #actual ","  #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_STRING_EQUAL_FATAL(actual, expected) CU_assertImplementation(!(strcmp((const char*)(actual), (const char*)(expected))), __LINE__, ("CU_ASSERT_STRING_EQUAL_FATAL(" #actual ","  #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_STRING_NOT_EQUAL(actual, expected) CU_assertImplementation((strcmp((const char*)(actual), (const char*)(expected))), __LINE__, ("CU_ASSERT_STRING_NOT_EQUAL(" #actual ","  #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_STRING_NOT_EQUAL_FATAL(actual, expected) CU_assertImplementation((strcmp((const char*)(actual), (const char*)(expected))), __LINE__, ("CU_ASSERT_STRING_NOT_EQUAL_FATAL(" #actual ","  #expected ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_NSTRING_EQUAL(actual, expected, count) CU_assertImplementation(!(strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, ("CU_ASSERT_NSTRING_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_NSTRING_EQUAL_FATAL(actual, expected, count) CU_assertImplementation(!(strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, ("CU_ASSERT_NSTRING_EQUAL_FATAL(" #actual ","  #expected "," #count ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_NSTRING_NOT_EQUAL(actual, expected, count) CU_assertImplementation((strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, ("CU_ASSERT_NSTRING_NOT_EQUAL(" #actual ","  #expected "," #count ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_NSTRING_NOT_EQUAL_FATAL(actual, expected, count) CU_assertImplementation((strncmp((const char*)(actual), (const char*)(expected), (size_t)(count))), __LINE__, ("CU_ASSERT_NSTRING_NOT_EQUAL_FATAL(" #actual ","  #expected "," #count ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_DOUBLE_EQUAL(actual, expected, granularity) CU_assertImplementation(((fabs((double)(actual) - (expected)) <= fabs((double)(granularity)))), __LINE__, ("CU_ASSERT_DOUBLE_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_DOUBLE_EQUAL_FATAL(actual, expected, granularity) CU_assertImplementation(((fabs((double)(actual) - (expected)) <= fabs((double)(granularity)))), __LINE__, ("CU_ASSERT_DOUBLE_EQUAL_FATAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_DOUBLE_NOT_EQUAL(actual, expected, granularity) CU_assertImplementation(((fabs((double)(actual) - (expected)) > fabs((double)(granularity)))), __LINE__, ("CU_ASSERT_DOUBLE_NOT_EQUAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_DOUBLE_NOT_EQUAL_FATAL(actual, expected, granularity) CU_assertImplementation(((fabs((double)(actual) - (expected)) > fabs((double)(granularity)))), __LINE__, ("CU_ASSERT_DOUBLE_NOT_EQUAL_FATAL(" #actual ","  #expected "," #granularity ")"), __FILE__, "", CU_TRUE)
#define BC_ASSERT_GREATER(actual, expected) CU_assertImplementation(((actual) >= (expected)), __LINE__, ("CU_ASSERT_GREATER(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)
#define BC_ASSERT_LOWER(actual, expected) CU_assertImplementation(((actual) <= (expected)), __LINE__, ("CU_ASSERT_LOWER(" #actual "," #expected ")"), __FILE__, "", CU_FALSE)

/*Add some custom defines with logs in case of fail*/
#define BC_ASSERT_EQUAL_INT(actual, expected) { \
	int cactual = (actual), cexpected = (expected); \
	if (! BC_ASSERT_EQUAL(cactual, cexpected)) { \
		bc_tester_printf(bc_printf_verbosity_error, "%s:%d - Expected " #actual " = " #expected " but was %d != %d\n", __FILE__, __LINE__, cactual, cexpected); \
	} \
}
#define BC_ASSERT_GREATER_INT(actual, expected) { \
	int cactual = (actual), cexpected = (expected); \
	if (! BC_ASSERT_GREATER(cactual, cexpected)) { \
		bc_tester_printf(bc_printf_verbosity_error, "%s:%d - Expected " #actual " >= " #expected " but was %d < %d\n", __FILE__, __LINE__, cactual, cexpected); \
	} \
}
#define BC_ASSERT_LOWER_INT(actual, expected) { \
	int cactual = (actual), cexpected = (expected); \
	if (! BC_ASSERT_LOWER(cactual, cexpected)) { \
		bc_tester_printf(bc_printf_verbosity_error, "%s:%d - Expected " #actual " <= " #expected " but was %d > %d\n", __FILE__, __LINE__, cactual, cexpected); \
	} \
}
#define BC_ASSERT_GREATER_UINT64_T(actual, expected) { \
	uint64_t cactual = (actual), cexpected = (expected); \
	if (! BC_ASSERT_GREATER(cactual, cexpected)) { \
		bc_tester_printf(bc_printf_verbosity_error, "%s:%d - Expected " #actual " >= " #expected " but was %lu < %lu\n", __FILE__, __LINE__, (long unsigned)cactual, (long unsigned)cexpected); \
	} \
}
#define BC_ASSERT_LOWER_UINT64_T(actual, expected) { \
	uint64_t cactual = (actual), cexpected = (expected); \
	if (! BC_ASSERT_LOWER(cactual, cexpected)) { \
		bc_tester_printf(bc_printf_verbosity_error, "%s:%d - Expected " #actual " <= " #expected " but was %lu > %lu\n", __FILE__, __LINE__, (long unsigned)cactual, (long unsigned)cexpected); \
	} \
}


#ifdef __cplusplus
}
#endif
#endif
