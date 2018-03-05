/**
 * @file
 * @brief source file for argument parsing functionality for an application
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

#include "app_arg.h"

#include <os.h>

/** @brief Prefix to use for short argument id */
#define APP_ARG_PREFIX_SHORT           '-'
/** @brief Prefix to use for long argument ids */
#define APP_ARG_PREFIX_LONG            "--"
/** @brief Character to use to split between a key & value pairs */
#define APP_ARG_VALUE_SPLIT            '='

unsigned int app_arg_count( const struct app_arg *args, char ch,
	const char *name )
{
	unsigned int result = 0u;
	int found = 0;

	while ( !found && args && ( args->ch || args->name ) )
	{
		if ( ( ch != 0 && ch == args->ch ) ||
			( name && args->name &&
		 os_strcmp( name, args->name ) == 0 ) )
		{
			result = args->hit;
			found = 1;
		}
		++args;
	}
	return result;
}

app_arg_iterator_t *app_arg_find(
	int argc,
	char **argv,
	app_arg_iterator_t *iter,
	char ch,
	const char *name )
{
	struct app_arg_iterator *rv = NULL;
	if ( iter )
	{
		iter->idx = 0;
		iter->ch = ch;
		iter->name = name;
		rv = app_arg_find_next( argc, argv, iter );
	}
	return rv;
}

struct app_arg_iterator *app_arg_find_next(
	int argc,
	char **argv,
	struct app_arg_iterator *iter )
{
	struct app_arg_iterator *rv = NULL;
	if ( iter && argv )
	{
		const size_t arg_prefix_long_len = os_strlen( APP_ARG_PREFIX_LONG );
		int match_found = 0;
		int cnt = iter->idx + 1;
		int no_key_count = 0;

		while ( cnt < argc && !match_found && no_key_count < 2 )
		{
			const char *c = argv[cnt];
			if ( c && os_strncmp( c, APP_ARG_PREFIX_LONG,
				arg_prefix_long_len ) == 0 )
			{
				c += arg_prefix_long_len;
				if ( *c == '\0' ) /* handle "--" case */
					no_key_count = 2u;
				else
				{
					const char *p;
					p = os_strchr( c, APP_ARG_VALUE_SPLIT );
					if ( iter->name )
					{
						if ( p )
						{
							if ( os_strncmp( iter->name,
								c, (size_t)(p - c) ) == 0 )
								match_found = 1;
						}
						else if ( os_strncmp( iter->name,
							c, os_strlen(iter->name) ) == 0 )
							match_found = 1;
					}
					else if ( iter->ch == '\0' ) /* return everything */
						match_found = 1;
				}
			}
			else if ( c && *c == APP_ARG_PREFIX_SHORT )
			{
				if ( iter->ch != '\0' )
				{
					if ( *(c+1) == iter->ch )
						++match_found;
				}
				else if ( !iter->name ) /* return everything */
					++match_found;
			}
			else
				++no_key_count;

			if ( !match_found && no_key_count < 2 )
				++cnt;
		}

		/* a match was found! */
		iter->idx = cnt;
		if ( match_found )
			rv = iter;
	}
	return rv;
}

int app_arg_iterator_key(
	int argc,
	char **argv,
	const app_arg_iterator_t *iter,
	size_t *key_len,
	const char **key )
{
	int rv = 0;
	if ( iter && iter->idx < argc )
	{
		const size_t arg_prefix_long_len = os_strlen( APP_ARG_PREFIX_LONG );
		const char *key_out = argv[iter->idx];
		size_t key_len_out = 0u;

		if ( key_out )
		{
			if ( os_strncmp( key_out, APP_ARG_PREFIX_LONG,
				arg_prefix_long_len ) == 0 )
			{
				const char *p;
				key_out += arg_prefix_long_len;
				p = os_strchr( key_out, APP_ARG_VALUE_SPLIT );
				if ( p )
					key_len_out = (size_t)(p - key_out);
				else
					key_len_out = os_strlen( key_out );
			}
			else if ( key_out[0u] == APP_ARG_PREFIX_SHORT &&
				  key_out[1u] != '\0' &&
				  key_out[1u] != APP_ARG_VALUE_SPLIT )
			{
				key_len_out = 1u;
				++key_out;
			}
		}

		/* return results */
		if ( key_len_out > 0u )
		{
			if ( key )
				*key = key_out;
			if ( key_len )
				*key_len = key_len_out;
			rv = 1;
		}
	}
	return rv;
}

int app_arg_iterator_value(
	int argc,
	char **argv,
	const app_arg_iterator_t *iter,
	size_t *value_len,
	const char **value )
{
	int rv = 0;
	if ( iter && iter->idx < argc )
	{
		const size_t arg_prefix_long_len =
			os_strlen( APP_ARG_PREFIX_LONG );
		const char *value_out = argv[iter->idx];
		size_t value_len_out = 0u;

		if ( value_out )
		{
			if ( os_strncmp( value_out, APP_ARG_PREFIX_LONG,
				arg_prefix_long_len ) == 0 ||
				*value_out == APP_ARG_PREFIX_SHORT )
			{
				if ( os_strncmp( value_out, APP_ARG_PREFIX_LONG,
					arg_prefix_long_len ) == 0 )
				{
					const char *p;
					value_out += arg_prefix_long_len;
					p = os_strchr( value_out, APP_ARG_VALUE_SPLIT );
					if ( p )
						value_out = p + 1;
					else
						value_out = NULL;
				}
				else
				{
					value_out += 2u;
					if ( *value_out == APP_ARG_VALUE_SPLIT )
						++value_out;
					else if ( *value_out == '\0' )
						value_out = NULL;
				}

				if ( !value_out && iter->idx + 1 < argc )
				{
					value_out = argv[iter->idx + 1];
					if ( value_out && (
						*value_out == APP_ARG_PREFIX_SHORT ||
						os_strncmp( value_out,
							APP_ARG_PREFIX_LONG,
							arg_prefix_long_len ) == 0 ))
						value_out = NULL;
				}
			}

			if ( value_out )
				value_len_out = os_strlen( value_out );
		}

		/* return results */
		if ( value_len_out > 0u )
		{
			if ( value )
				*value = value_out;
			if ( value_len )
				*value_len = value_len_out;
			rv = 1;
		}
	}
	return rv;
}

/** @todo rewrite to use iterator functions */
int app_arg_parse( struct app_arg *args, int argc, char **argv,
	int *pos )
{
	int result = EXIT_SUCCESS;
	int i;
	char ch = '\0';
	int pos_arg = 0;
	const char *name = NULL;
	const char *arg_id = NULL;
	const char **next_arg = NULL;
	struct app_arg *arg = args;
	while ( arg && ( arg->ch || arg->name ) )
	{
		arg->hit = 0u;
		++arg;
	}
	for ( i = 1u; i < argc && pos_arg == 0 && result == EXIT_SUCCESS; ++i )
	{
		const char *a = argv[i];
		arg = args;
		if ( next_arg )
		{
			if ( *a == '-' || *a == '\0' )
			{
				if ( arg_id && *arg_id == '[' )
					next_arg = NULL;
				else
					result = EXIT_FAILURE;
			}
			else
			{
				*next_arg = a;
				next_arg = NULL;
			}
		}
		else if ( !next_arg && *a != '-' )
			pos_arg = i;
		if ( !next_arg && *a == '-' )
		{
			int handled = 0;
			++a;
			while ( arg && ( arg->ch || arg->name ) && !handled )
			{
				if ( *a == '-' )
				{
					++a;
					if ( arg->name && os_strncmp( a, arg->name,
					     os_strlen( arg->name ) ) == 0 )
					{
						a += os_strlen( arg->name );
						name = arg->name;
						handled = 1;
					}
				}
				else if ( *a == arg->ch )
				{
					++a;
					ch = arg->ch;
					handled = 1;
				}
				if ( handled )
				{
					++arg->hit;
					if ( arg->param_result )
					{
						if ( *a && *a == APP_ARG_VALUE_SPLIT )
							++a;
						if ( *a )
							*arg->param_result = a;
						else
						{
							next_arg =
							    arg->param_result;
							arg_id = arg->param;
						}
					}
					else if ( *a )
					{
						os_fprintf( OS_STDERR,
							"Unexpected value \"%s\" "
							"for argument argument: ", a );
						result = EXIT_FAILURE;
					}
				}
				a = argv[i] + 1u;
				++arg;
			}
			if ( !handled )
			{
				os_fprintf( OS_STDERR,
					"unknown argument: %s\n", argv[i] );
				ch = 0;
				name = NULL;
				result = EXIT_FAILURE;
			}
		}
	}
	if ( next_arg && ( !arg_id || *arg_id != '[' ) )
	{
		os_fprintf( OS_STDERR, "%s", "expected " );
		if ( arg_id )
			os_fprintf( OS_STDERR, "\"%s\" ", arg_id );
		os_fprintf( OS_STDERR, "%s", "value for argument: " );
		result = EXIT_FAILURE;
	}

	/* check for required arguments */
	arg = args;
	while ( result == EXIT_SUCCESS && arg && ( arg->ch || arg->name ) )
	{
		if ( !arg->hit && arg->req )
		{
			os_fprintf( OS_STDERR,"%s",
				"required argument not specified: " );
			name = arg->name;
			ch = arg->ch;
			result = EXIT_FAILURE;
		}
		++arg;
	}
	if ( result == EXIT_FAILURE )
	{
		if ( ch )
			os_fprintf( OS_STDERR, "%c%c\n", APP_ARG_PREFIX_SHORT, ch );
		else if ( name )
			os_fprintf( OS_STDERR, "%s%s\n", APP_ARG_PREFIX_LONG, name );
	}
	else if ( !pos && pos_arg > 0 )
	{
		os_fprintf( OS_STDERR, "unknown argument: %s\n",
			argv[pos_arg] );
		result = EXIT_FAILURE;
	}
	else if ( pos )
		*pos = pos_arg;
	return result;
}

void app_arg_usage( const struct app_arg *args, size_t col,
	const char *app, const char *desc, const char *pos,
	const char *pos_desc )
{
	unsigned int i;
	int has_type[] = { 0, 0 };
	const struct app_arg *arg;
	const char *app_name = "exec";
	size_t pos_len = 0u;
	if ( app )
	{
		app_name = os_strrchr( app, '/' );
		if ( app_name )
			++app_name;
		else
			app_name = app;
	}
	os_printf( "usage: %s", app_name );

	arg = args;
	while( arg && ( arg->ch || arg->name ) )
	{
		os_printf( "%s", " " );
		if ( !arg->req )
		{
			os_printf( "%s", "[" );
			++has_type[1];
		}
		else
			++has_type[0];
		if ( arg->ch )
			os_printf( "%c%c", APP_ARG_PREFIX_SHORT, arg->ch );
		else
			os_printf( "%s%s", APP_ARG_PREFIX_LONG, arg->name );
		if ( arg->param )
			os_printf( " %s", arg->param );
		if ( !arg->req )
			os_printf( "%s", "]" );
		++arg;
	}

	/* handle positional argument display */
	if ( pos )
	{
		int pos_opt = 0;
		int pos_multi = 0;
		pos_len = os_strlen( pos );
		if ( pos[pos_len - 1u] == '+' )
		{
			--pos_len;
			pos_multi = 1;
		}
		if ( *pos == '[' )
		{
			++pos;
			pos_opt = 1;
			pos_len -= 2u;
		}
		os_printf( "%s", " " );
		if ( !pos_opt )
		{
			os_printf( "%.*s", (int)pos_len, pos );
			if ( pos_multi )
				os_printf( "%s", " " );
		}
		if ( pos_multi || pos_opt )
			os_printf( "[%.*s", (int)pos_len, pos );
		if ( pos_multi )
			os_printf( "%s", " ..." );
		if ( pos_multi || pos_opt )
			os_printf( "%s", "]" );
	}
	os_printf( "%s", "\n" );
	if ( desc )
		os_printf( "\n%s\n", desc );
	if ( pos )
	{
		os_printf( "\npositional arguments:\n%-.*s%*s",
			(int)pos_len, pos, (int)(col - pos_len), "" );
		if ( pos_desc )
			os_printf( "%s", pos_desc );
		os_printf( "%s", "\n" );
	}

	if ( col ) --col;
	for ( i = 0u; i < 2u; ++i )
	{
		if ( has_type[i] )
		{
			os_printf( "%s", "\n" );
			if ( i )
				os_printf( "%s", "optional arguments:\n" );
			else
				os_printf( "%s", "required arguments:\n" );
			arg = args;
			while ( arg && ( arg->ch || arg->name ) )
			{
				if ( ( i && !arg->req ) || ( !i && arg->req ) )
				{
					size_t id_len = 0u;
					size_t line_len = 0u;
					/* calculate the size of the parameter id tag */
					if ( arg->param )
					{
						id_len = col;
						if ( arg->ch )
							id_len -= 3u; /* "-c " */
						if ( arg->name )
							id_len -=
							  ( os_strlen( arg->name ) + 3u ); /* "--name " */
						if ( arg->ch && arg->name )
						{
							id_len -= 2u; /* ", " */
							id_len /= 2u;
						}
						if ( id_len > os_strlen( arg->param ) ) /* arg*/
							id_len = os_strlen( arg->param );
					}
					if ( arg->ch )
					{
						os_printf( "%c%c",
							APP_ARG_PREFIX_SHORT,
							arg->ch );
						line_len = 2u;
						if ( arg->param )
						{
							os_printf( " %*.*s",
								(int)id_len,
								(int)id_len,
								arg->param );
							line_len += id_len + 1u;
						}
						if ( arg->name )
						{
							os_printf( "%s", ", " );
							line_len += 2u;
						}
					}
					if ( arg->name )
					{
						size_t max_name_len = col - line_len - 2u;
						if ( arg->param )
							max_name_len -= id_len - 1u; /* " " */
						os_printf( "%s%.*s",
							APP_ARG_PREFIX_LONG,
							(int)max_name_len,
							arg->name );
						if ( os_strlen( arg->name )
							< max_name_len )
							line_len +=
								os_strlen( arg->name ) + 2u;
						else
							line_len += max_name_len + 2u;
						if ( arg->param )
						{
							os_printf( " %*.*s",
								(int)id_len,
								(int)id_len,
								arg->param );
							line_len += id_len + 1u;
						}
					}
					if ( line_len < col )
					{
						line_len = col - line_len;
						os_printf( "%*.*s",
							(int)line_len,
							(int)line_len, " " );
					}
					if ( arg->desc )
						os_printf( " %s",
							arg->desc );
					os_printf( "%s", "\n" );
				}
				++arg;
			}
		}
	}
}

