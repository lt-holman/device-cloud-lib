/**
 * @file
 * @brief Header file declaring common unit test functionality
 *
 * @copyright Copyright (C) 2017-2018 Wind River Systems, Inc. All Rights Reserved.
 *
 * @license Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied."
 */

#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

/* header files to include before cmocka */
/* clang-format off */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
/* clang-format on */

#include <stdio.h> /* for snprintf */

/* function not available in older cmocka versions */
/* introduced in cmocka 1.1.1 */
/* introduced in cmocka 1.1.0 */
/* introduced in cmocka 1.0.1 */
#if !defined(assert_ptr_equal)
#define assert_ptr_equal(a,b) _assert_int_equal(\
	cast_to_largest_integral_type(a), \
	cast_to_largest_integral_type(b), __FILE__, __LINE__)
#endif /* if !defined(assert_ptr_equal) */

#if !defined(assert_ptr_not_equal)
#define assert_ptr_not_equal(a,b) _assert_int_not_equal(\
	cast_to_largest_integral_type(a), \
	cast_to_largest_integral_type(b), __FILE__, __LINE__)
#endif /* !defined(assert_ptr_not_equal) */

/* introduced in cmocka 1.0.0 */
#if !defined(test_realloc)
void *_test_realloc(void *ptr,
	const size_t size,
	const char *file,
	const int line);
#define test_realloc(ptr,size) _test_realloc(ptr, size, __FILE__, __LINE__)
#define NEED_TEST_REALLOC 1
#endif /* if !defined(test_realloc) */

#if !defined(cmocka_unit_test)
#define CMUnitTestFunction UnitTestFunction
typedef int (*CMFixtureFunction)(void **state);
struct CMUnitTest
{
	const char *name;
	CMUnitTestFunction test_func;
	CMFixtureFunction setup_func;
	CMFixtureFunction teardown_func;
};
#define cmocka_unit_test(f) {#f, f, NULL, NULL}
#endif /* if !defined(cmocka_unit_test) */

#if !defined(cmocka_unit_test_setup)
#define cmocka_unit_test_setup(f, setup) {#f, f, setup, NULL}
#endif /* if !defined(cmocka_unit_test_setup) */

#if !defined(cmocka_unit_test_teardown)
#define cmocka_unit_test_teardown(f, teardown) {#f, f, NULL, teardown}
#endif /* if !defined(cmocka_unit_test_teardown) */

#if !defined(cmocka_unit_test_setup_teardown)
#define cmocka_unit_test_setup_teardown(f, setup, teardown) {#f, f, setup, teardown}
#endif /* if !defined(cmocka_unit_test_setup_teardown) */

#if !defined(cmocka_run_group_tests)
int _cmocka_run_group_tests(const char *group_name,
	const struct CMUnitTest *const tests,
	const size_t num_tests,
	CMFixtureFunction group_setup,
	CMFixtureFunction group_teardown);
#define cmocka_run_group_tests(group_tests, group_setup, group_teardown) \
	_cmocka_run_group_tests(#group_tests, group_tests, \
		sizeof(group_tests)/sizeof(group_tests)[0], \
		group_setup, group_teardown)
#define NEED_CMOCKA_RUN_GROUP_TESTS 1
#endif /* if !defined(cmocka_run_group_tests) */

#if !defined(cmocka_run_group_tests_name)
#define cmocka_run_group_tests_name(group_name, group_tests, group_setup, \
		group_teardown) \
	_cmocka_run_group_tests(group_name, group_tests, \
		sizeof(group_tests)/sizeof(group_tests)[0], \
		group_setup, group_teardown)
#endif /* if !defined(cmocka_run_group_tests_name) */

/* introduced in cmocka 0.4.1 */
/* introduced in cmocka 0.4.0 */
#if !defined(assert_return_code)
void _assert_return_code(const LargestIntegralType result,
	size_t rlen,
	const LargestIntegralType error,
	const char * const expression,
	const char * const file,
	const int line);
#define assert_return_code(rc, error) _assert_return_code(\
	cast_to_largest_integral_type(a), sizeof(rc), \
	cast_to_largest_integral_type(error), #rc, __FILE__, __LINE__)
#define NEED_ASSERT_RETURN_CODE 1
#endif /* if !defined(assert_return_code) */

/* introduced in cmocka 0.3.2 */
/* introduced in cmocka 0.3.1 */
/* introduced in cmocka 0.3.0 */
/* introduced in cmocka 0.2.0
 * - assert_false
 * - assert_in_range
 * - assert_in_set
 * - assert_int_equal
 * - assert_int_not_equal
 * - assert_memory_equal
 * - assert_memory_not_equal
 * - assert_non_null
 * - assert_not_in_range
 * - assert_not_in_set
 * - assert_null **
 * - assert_string_equal
 * - assert_string_not_equal
 * - assert_true
 *
 * - expect_assert_failure
 * - mock_assert
 *
 * - test_calloc
 * - test_malloc
 * - test_free
 */

#ifdef __cplusplus
extern "C" {
#endif /* ifdef __cplusplus */

#ifndef __GNUC__
#define __attribute__( x ) /* gcc specific */
#endif                     /* ifndef __GNUC__ */

/* In Windows some signatures of functions are slightly different */
#if defined( _WIN32 ) && !defined( snprintf )
#define snprintf _snprintf
#endif /* if defined( _WIN32 ) && !defined( snprintf ) */

/* functions */
/**
 * @brief Called to destory test support system
 *
 * @param[in]      argc                number of command-line arguments
 * @param[in]      argv                array of command-line arguments
 */
void test_finalize( int argc, char **argv );

/**
 * @brief Generates a random string for testing
 *
 * @note This function uses a pseudo-random generator to provide
 *       reproductability between test runs, if given the same seed
 *
 * @note The returned string is null terminated
 *
 * @param[out]     dest                destination to put the generated string
 * @param[in]      len                 size of the destination buffer, returned
 *                                     string is null-terminated (thus random
 *                                     characters are: len - 1u)
 */
void test_generate_random_string( char *dest, size_t len );

/**
 * @brief Generates a random universally unique identifer (UUID) for testing
 *
 * @note This function uses a pseudo-random generator to provide
 *       reproductability between test runs, if given the same seed
 *
 * @note The returned string is null terminated
 *
 * @param[out]     dest                destination to put the generated string
 * @param[in]      len                 size of the destination buffer, returned
 *                                     string is null-terminated (thus random
 *                                     characters are: len - 1u).  Note that
 *                                     only the first 36 character (37 +
 *                                     null-terminator) are encoded
 * @param[in]      to_upper            convert the hexidecimal characters to
 *                                     upper-case
 */
void test_generate_random_uuid( char *dest, size_t len, int to_upper );

/**
 * @brief Called to initialize test support system
 *
 * @param[in]      argc                number of command-line arguments
 * @param[in]      argv                array of command-line arguments
 */
void test_initialize( int argc, char **argv );

/**
 * @brief Checks to see if an argument was passed on the command line
 *
 * @param[in]      argc                number of command-line arguments
 * @param[in]      argv                array of command-line arguments
 * @param[in]      name                name of argument (NULL if none)
 * @param[in]      abbrev              abbreviation character ('\0' if none)
 * @param[in]      idx                 matching index if argument specified
 *                                     multiple times (0 for first match)
 * @param[out]     value               value set (NULL if no output value)
 *
 *
 * @retval 0  argument found
 * @retval -1 argument not found
 * @retval -2 argument found, but no value found
 */
int test_parse_arg(
	int argc,
	char **argv,
	const char *name,
	const char abbrev,
	unsigned int idx,
	const char **value );

/* macros */
/**
 * @def FUNCTION_NAME
 * @brief Macro that will hold the name of the current function
 */
#ifndef FUNCTION_NAME
#if defined __func__
#define FUNCTION_NAME __func__
#elif defined __PRETTY_FUNCTION__
#define FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined __FUNCTION__
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME ""
#endif
#endif

/**
 * @brief Macro test displays the name of the test case
 * @param[in]  x   name of the test case
 */
#define test_case( x ) test_case_out( FUNCTION_NAME, x )
/**
 * @brief Macro that displays the name of the test case
 * @param[in] x    name of the test case in printf format
 * @param[in] ...  printf format flags for the test case name
 */
#define test_case_printf( x, ... ) test_case_out( FUNCTION_NAME, x, __VA_ARGS__ )

#ifdef __cplusplus
}
#endif /* ifdef __cplusplus */

/**
 * @brief Whether low-level system function mocking is currently enabled
 */
extern int MOCK_SYSTEM_ENABLED;

#endif /* ifndef TEST_SUPPORT_H */
