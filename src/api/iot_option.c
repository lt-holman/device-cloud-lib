/**
 * @file
 * @brief source file for optional option support
 *
 * @copyright Copyright (C) 2017 Wind River Systems, Inc. All Rights Reserved.
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

#include "iot_common.h"

/**
 * @brief Searches an options list for a specified option
 *
 * @param[in]      options             list of options to search through
 * @param[in]      name                name of option to find
 * @param[out]     found_idx           index of the found option (optional)
 *
 * @return a pointer to the matching option (or NULL if not found)
 */
static IOT_SECTION struct iot_option *iot_options_find(
	const iot_options_t *options,
	const char *name,
	iot_uint8_t *found_idx );


iot_options_t *iot_options_allocate( iot_t *lib )
{
	iot_options_t *result = NULL;
	if ( lib )
	{
#ifdef IOT_STACK_ONLY
		size_t i;
		if ( !lib->options )
		{
			lib->options = lib->_options_ptrs;
			for ( i = 0u; i < IOT_OPTION_MAX; ++i )
				lib->options[i] = &lib->_options[i];
		}
		if ( lib->options_count < IOT_OPTION_MAX )
			result = lib->options[lib->options_count];
#else
		if ( lib->options_count < IOT_OPTION_MAX )
			result = os_malloc( sizeof( struct iot_options ) );
#endif
		if ( result )
		{
			os_memzero( result, sizeof( struct iot_options ) );
			result->lib = lib;
#ifdef IOT_STACK_ONLY
			result->option = result->_option;
#else
			{
				struct iot_options **arr;
				arr = (struct iot_options **)os_realloc(
					lib->options,
					sizeof( struct iot_options** ) *
						(lib->options_count + 1u) );

				if ( arr )
					lib->options = arr;
				else
				{
					/* failed to allocate memory */
					os_free( result );
					result = NULL;
				}
			}
#endif /* ifdef IOT_STACK_ONLY */

			/* ensure memory allocation succeeded */
			if ( lib->options && result )
			{
				lib->options[lib->options_count] = result;

				/* add options array to library */
				++lib->options_count;
			}
		}
	}
	return result;
}

iot_status_t iot_options_free( iot_options_t *options )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( options && options->lib )
	{
		iot_uint8_t i;
		struct iot *const lib = options->lib;

		result = IOT_STATUS_NOT_FOUND;
		for ( i = 0u; i < lib->options_count &&
			options != lib->options[i]; ++i );

		if ( i < lib->options_count )
		{
			os_memmove( &lib->options[i],
				&lib->options[i + 1],
				sizeof( struct iot_options * ) *
					(lib->options_count - i - 1u) );

#ifndef IOT_STACK_ONLY
			/* free all option name & any set heap data */
			for ( i = 0u; i < options->option_count; ++i )
			{
				struct iot_option *const option =
					&options->option[i];
				if ( option )
				{
					if ( option->name )
						os_free( option->name );
					if ( option->data.heap_storage )
						os_free( option->data.heap_storage );
				}
			}

			/* free options array */
			if ( options->option )
				os_free( options->option );

			/* free options object */
			os_free( options );
#endif /* ifndef IOT_STACK_ONLY */

			--lib->options_count;
			result = IOT_STATUS_SUCCESS;
		}
	}
	return result;
}

iot_status_t iot_options_clear(
	iot_options_t *options,
	const char *name )
{
	return iot_options_set( options, name, IOT_TYPE_NULL, NULL );
}

struct iot_option *iot_options_find(
	const iot_options_t *options,
	const char *name,
	iot_uint8_t *found_idx )
{
	iot_uint8_t cur_idx = 0u;
	struct iot_option *opt = NULL;
	if ( options )
	{
		iot_uint8_t min_idx = 0u;
		iot_uint8_t max_idx = options->option_count;

		/* binary search through list to find configuration item */
		while ( !opt && (iot_uint8_t)(max_idx - min_idx) > 0u )
		{
			int cmp_result;

			cur_idx = (max_idx - min_idx) / 2u + min_idx;
			opt = &options->option[cur_idx];
			cmp_result = os_strncasecmp( name, opt->name,
				IOT_NAME_MAX_LEN );
			if ( cmp_result > 0 )
			{
				++cur_idx;
				min_idx = cur_idx;
				opt = NULL;
			}
			else if ( cmp_result < 0 )
			{
				max_idx = cur_idx;
				opt = NULL;
			}
		}

		/* search next item, in case we rounded down with '/ 2u' */
		if ( !opt && options->option_count > 0u &&
			cur_idx < options->option_count - 1u )
		{
			++cur_idx;
			opt = &options->option[cur_idx];

			if ( os_strncasecmp( name, opt->name,
				IOT_NAME_MAX_LEN ) != 0 )
			{
				--cur_idx;
				opt = NULL;
			}
		}
	}

	/* return index if desired */
	if ( found_idx )
		*found_idx = cur_idx;
	return opt;
}

iot_status_t iot_options_get(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	iot_type_t type, ... )
{
	va_list args;
	iot_status_t result;

	va_start( args, type );
	result = iot_options_get_args( options, name, convert, type, args );
	va_end( args );

	return result;
}

iot_status_t iot_options_get_args(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	iot_type_t type,
	va_list args )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( options && name )
	{
		struct iot_option *const opt = iot_options_find(
			options, name, NULL );

		result = IOT_STATUS_NOT_FOUND;
		if ( opt )
			result = iot_common_arg_get(
				&opt->data, convert, type, args );
	}
	return result;
}

iot_status_t iot_options_get_bool(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	iot_bool_t *value )
{
	return iot_options_get( options, name, convert, IOT_TYPE_BOOL, value );
}

iot_status_t iot_options_get_integer(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	iot_int64_t *value )
{
	return iot_options_get( options, name, convert, IOT_TYPE_INT64, value );
}

iot_status_t iot_options_get_location(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	const iot_location_t **value )
{
	return iot_options_get( options, name, convert, IOT_TYPE_LOCATION, value );
}

iot_status_t iot_options_get_raw(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	size_t *length,
	const void **data )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( data )
	{
		struct iot_data_raw raw_data;
		os_memzero( &raw_data, sizeof( struct iot_data_raw ) );
		result = iot_options_get( options, name, convert,
			IOT_TYPE_RAW, &raw_data );
		if ( length )
			*length = raw_data.length;
		*data = raw_data.ptr;
	}
	return result;
}

iot_status_t iot_options_get_real(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	iot_float64_t *value )
{
	return iot_options_get( options, name, convert, IOT_TYPE_FLOAT64, value );
}

iot_status_t iot_options_get_string(
	const iot_options_t *options,
	const char *name,
	iot_bool_t convert,
	const char **value )
{
	return iot_options_get( options, name, convert, IOT_TYPE_STRING, value );
}

iot_status_t iot_options_set(
	iot_options_t *options,
	const char *name,
	iot_type_t type, ... )
{
	va_list args;
	iot_status_t result;

	va_start( args, type );
	result = iot_options_set_args( options, name, type, args );
	va_end( args );

	return result;
}

iot_status_t iot_options_set_args(
	iot_options_t *options,
	const char *name,
	iot_type_t type,
	va_list args )
{
	struct iot_data data;
	iot_status_t result;

	os_memzero( &data, sizeof( struct iot_data ) );
	result = iot_common_arg_set( &data, IOT_TRUE, type, args );
	if ( result == IOT_STATUS_SUCCESS )
		result = iot_options_set_data( options, name, &data );
	return result;
}

iot_status_t iot_options_set_bool(
	iot_options_t *options,
	const char *name,
	iot_bool_t value )
{
	return iot_options_set( options, name, IOT_TYPE_BOOL, value );
}

iot_status_t iot_options_set_data(
	iot_options_t *options,
	const char *name,
	const struct iot_data *data )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( options && name && *name != '\0' && data )
	{
		struct iot_option *opt;
		iot_uint8_t cur_idx = 0u;

		opt = iot_options_find( options, name, &cur_idx );

		/* delete item */
		if ( opt && data->type == IOT_TYPE_NULL )
		{
#ifndef IOT_STACK_ONLY
			os_free( opt->name );
			os_free( opt->data.heap_storage );
#endif /* ifndef IOT_STACK_ONLY */

			/* move all options after current item up 1 spot */
			os_memmove( &options->option[cur_idx],
				&options->option[cur_idx + 1],
				sizeof( struct iot_option ) *
					(options->option_count - cur_idx) );
			--options->option_count;
			result = IOT_STATUS_SUCCESS;
		}

		/* adding a new option */
		result = IOT_STATUS_FULL;
		if ( !opt && options->option_count < IOT_OPTION_MAX )
		{
#ifndef IOT_STACK_ONLY
			void *ptr = os_realloc( options->option,
				sizeof( struct iot_option ) *
					( options->option_count + 1u ) );

			result = IOT_STATUS_NO_MEMORY;
			if ( ptr )
				options->option = ptr;
			if ( options->option && ptr )
#endif /* ifndef IOT_STACK_ONLY */
			{
				/* move options, up 1 spot after current one */
				os_memmove( &options->option[cur_idx + 1u],
					&options->option[cur_idx],
					sizeof( struct iot_option ) *
						(options->option_count - cur_idx) );
				opt = &options->option[cur_idx];
				os_memzero( opt, sizeof( struct iot_option ) );
				++options->option_count;
			}
		}

		/* add or update the option */
		if ( opt && data->type != IOT_TYPE_NULL )
		{
			iot_bool_t update = IOT_TRUE;
			/* add name if not already given */
			if (
#ifndef IOT_STACK_ONLY
				!opt->name ||
#endif /* ifndef IOT_STACK_ONLY */
				*opt->name == '\0' )
			{
				size_t name_len = os_strlen( name );
				if ( name_len > IOT_NAME_MAX_LEN )
					name_len = IOT_NAME_MAX_LEN;
#ifndef IOT_STACK_ONLY
				opt->name = os_malloc( (name_len + 1u) );
				result = IOT_STATUS_NO_MEMORY;
				if ( !opt->name )
					update = IOT_FALSE;
				else
#endif /* ifndef IOT_STACK_ONLY */
				{
					os_strncpy( opt->name, name, name_len );
					opt->name[name_len] = '\0';
				}
			}

			if ( update )
			{
				os_free_null( (void **)&opt->data.heap_storage );
				result = iot_common_data_copy( &opt->data, data,
					IOT_TRUE );

				/** @todo fix up on failure */
				if ( result != IOT_STATUS_SUCCESS )
				{
#ifndef IOT_STACK_ONLY
					os_free( opt->name );
#endif
					--options->option_count;
					os_memmove( &options->option[cur_idx],
						&options->option[cur_idx + 1u],
						sizeof( struct iot_option ) *
							(options->option_count - cur_idx) );
				}
			}
		}
	}

	if ( result != IOT_STATUS_SUCCESS )
	{
		if ( options && name )
			IOT_LOG( options->lib, IOT_LOG_NOTICE,
				"Unable to store value for \"%s\"; Reason: %s",
				name, iot_error( result ) );
	}

	return result;
}

iot_status_t iot_options_set_integer(
	iot_options_t *options,
	const char *name,
	iot_int64_t value )
{
	return iot_options_set( options, name, IOT_TYPE_INT64, value );
}

iot_status_t iot_options_set_location(
	iot_options_t *options,
	const char *name,
	const iot_location_t *value )
{
	return iot_options_set( options, name, IOT_TYPE_LOCATION, value );
}

iot_status_t iot_options_set_raw(
	iot_options_t *options,
	const char *name,
	size_t length,
	const void *value )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( value )
	{
		struct iot_data_raw raw_data;
		os_memzero( &raw_data, sizeof( struct iot_data_raw ) );
		raw_data.ptr = value;
		raw_data.length = length;
		return iot_options_set( options, name,
			IOT_TYPE_RAW, &raw_data );
	}
	return result;
}

iot_status_t iot_options_set_real(
	iot_options_t *options,
	const char *name,
	iot_float64_t value )
{
	return iot_options_set( options, name, IOT_TYPE_FLOAT64, value );
}

iot_status_t iot_options_set_string(
	iot_options_t *options,
	const char *name,
	const char *value )
{
	return iot_options_set( options, name, IOT_TYPE_STRING, value );
}

