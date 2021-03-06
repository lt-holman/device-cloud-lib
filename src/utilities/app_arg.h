/**
 * @file
 * @brief header file for argument parsing functionality for an application
 *
 * @copyright Copyright (C) 2016-2018 Wind River Systems, Inc. All Rights Reserved.
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

#ifndef APP_ARG_H
#define APP_ARG_H

#include <stdlib.h> /* for size_t */
#include "os.h"     /* osal functions */

/** @brief Argument is required (no flags specified) */
#define APP_ARG_FLAG_REQUIRED          0x0
/** @brief Argument is optional */
#define APP_ARG_FLAG_OPTIONAL          0x1
/** @brief Argument is allowed to be specified multiple times */
#define APP_ARG_FLAG_MULTI             0x2
/** @brief Argument has required parameters */
#define APP_ARG_FLAG_PARAM_OPTIONAL    0x4

/** @brief Structure defining an arugment for an application */
struct app_arg
{
	/** @brief Argument single character token (i.e. specified with "-") */
	char ch;
	/** @brief Argument multi character token (i.e. specified with "--") */
	const char *name;
	/** @brief Flags for this argument */
	int flags;
	/** @brief Parameter id used in display (optional) */
	const char *param;
	/** @brief Location set pointer if found (optional) */
	const char **param_result;
	/** @brief Argument description (optional) */
	const char *desc;
	/** @brief Number of times argument was found (initialize to 0)*/
	unsigned int hit;
};

/** @brief Structure defining the iterator for going through arguments */
struct app_arg_iterator
{
	/** @brief Character key iterating through */
	char ch;
	/** @brief Full name iterating through */
	const char *name;
	/** @brief Current index */
	int idx;
};
/** @brief Structure of initializing an @p app_arg_iterator structure */
#define APP_ARG_ITERATOR_INIT { '\0', NULL, 0u }
/** @brief Type defining an argument iterator */
typedef struct app_arg_iterator app_arg_iterator_t;

/**
 * @brief Returns the number of times an argument was specified
 *
 * @param[in]      args                pointer to the application arguments
 *                                     array
 * @param[in]      ch                  single character token (optional)
 * @param[in]      name                multiple character token (optional)
 *
 * @return the number of times that the argument was specified
 *
 * @see app_arg_parse
 * @see app_arg_usage
 */
unsigned int app_arg_count( const struct app_arg *args, char ch,
	const char *name );

/**
 * @brief Create an iterator finding all arguments matching criteria
 *
 * @note if neither @p ch or @p name is not given then this will return an
 * iterator to every argument pair passed to the function.  If both are
 * specified then this function will return any item matching either @p ch or
 * @p name.
 *
 * @param[in]      argc                number of command line arguments
 * @param[in]      argv                array of command line arguments
 * @param[in,out]  iter                iterator object to initialize
 * @param[in]      ch                  argument key to match (optional)
 * @param[in]      name                argument name to match (optional)
 *
 * @retval         NULL                no matching arguments found
 * @retval         !NULL               iterator to matching argument
 *
 * @see app_arg_find_next
 * @see app_arg_iterator_key
 * @see app_arg_iterator_value
 */
app_arg_iterator_t *app_arg_find(
	int argc,
	char **argv,
	app_arg_iterator_t *iter,
	char ch,
	const char *name );

/**
 * @brief Finds the next item in the iterator
 *
 * @param[in]      argc                number of command line arguments
 * @param[in]      argv                array of command line arguments
 * @param[in,out]  iter                iterator object to increment
 *
 * @retval         NULL                no more matching arguments found
 * @retval         !NULL               next matching argument
 *
 * @see app_arg_find
 * @see app_arg_iterator_key
 * @see app_arg_iterator_value
 */
app_arg_iterator_t *app_arg_find_next(
	int argc,
	char **argv,
	app_arg_iterator_t *iter );

/**
 * @brief Returns the key for the item an iterator points to
 *
 * @param[in]      argc                number of command line arguments
 * @param[in]      argv                array of command line arguments
 * @param[in]      iter                iterator object to retrieve key for
 * @param[out]     key_len             string length of the key (optional)
 * @param[out]     key                 pointer to the key (optional)
 *
 * @retval         0                   on failure
 * @retval         1                   on success
 *
 * @see app_arg_find
 * @see app_arg_find_next
 * @see app_arg_iterator_value
 */
int app_arg_iterator_key(
	int argc,
	char **argv,
	const app_arg_iterator_t *iter,
	size_t *key_len,
	const char **key );

/**
 * @brief Returns the value for the item an iterator points to
 *
 * @param[in]      argc                number of command line arguments
 * @param[in]      argv                array of command line arguments
 * @param[in]      iter                iterator object to retrieve value for
 * @param[out]     value_len           string length of the value (optional)
 * @param[out]     value               pointer to the value (optional)
 *
 * @retval         0                   on failure
 * @retval         1                   on success
 *
 * @see app_arg_find
 * @see app_arg_find_next
 * @see app_arg_iterator_key
 */
int app_arg_iterator_value(
	int argc,
	char **argv,
	const app_arg_iterator_t *iter,
	size_t *value_len,
	const char **value );

/**
 * @brief Parses arguments passed to the application
 *
 * @param[in,out]  args                array specifying arguments expected by
 *                                     the application
 * @param[in]      argc                number of arguments passed to the
 *                                     application
 * @param[in]      argv                array of arguments passed to the
 *                                     application
 * @param[in,out]  pos                 index of first positional argument
 *                                     (optional)
 *
 * @note This function doesn't check for optional or required positional
 * arguments
 *
 * @note If no positional arguments are found then @p pos will return a value
 * of 0
 *
 * @retval EXIT_FAILURE                application encountered an error
 * @retval EXIT_SUCCESS                application completed successfully
 *
 * @see app_arg_count
 * @see app_arg_usage
 */
int app_arg_parse( struct app_arg *args, int argc, char **argv,
	int *pos );

/**
 * @brief Prints to stdout which arguments will be handled by the application
 *
 * @param[in]      args                array specifying arguments expected by
 *                                     the application
 * @param[in]      col                 column index at which to align argument
 *                                     descriptions
 * @param[in]      app                 name of the application (optional)
 * @param[in]      desc                description of the application (optional)
 * @param[in]      pos                 id name to use for positional arguments
 *                                     (optional)
 * @param[in]      pos_desc            description for positional arguments
 *                                     (optional)
 *
 * @note @b pos can be of the format "id", "[id]", "id+" or "[id]+", where 'id'
 * is the name to use for the argument. Enclosing the id in brackets '[]'
 * indicates that positional argument(s) are optional.  A plus '+' at the end
 * indicates that multiple positional arguments are able to be handled.
 *
 * @see app_arg_count
 * @see app_arg_parse
 */
void app_arg_usage( const struct app_arg *args, size_t col,
	const char *app, const char *desc, const char *pos,
	const char *pos_desc );

#endif /* ifndef APP_ARG_H */

