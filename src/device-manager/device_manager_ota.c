/**
 * @file
 * @brief Source file for service handling in the IoT control application
 *
 * @copyright Copyright (C) 2017 Wind River Systems, Inc. All Rights Reserved.
 *
 * @license The right to copy, distribute or otherwise make use of this software
 * may be licensed only pursuant to the terms of an applicable Wind River
 * license agreement.  No license to Wind River intellectual property rights is
 * granted herein.  All rights not licensed by Wind River are reserved by Wind
 * River.
 */
#include "device_manager_ota.h"
#include "utilities/app_path.h"
#include "device_manager_main.h"
#include "device_manager_file.h"
#include "api/shared/iot_base64.h"
#include "api/shared/iot_types.h"     /* for IOT_ACTION_NO_RETURN, IOT_RUNTIME_DIR */
#if defined( __unix__ ) && !defined( __ANDROID__ )
/*FIXME*/
/*#include "device_manager_scripts.h"*/
#endif /*defined( __unix__ ) && !defined( __ANDROID__ )*/
#include "iot_build.h"
#include "iot.h"

#include <archive.h>                   /* for archiving functions */
#include <archive_entry.h>             /* for adding files to an archive */

/** @brief Name of the parameter to manifest action */
#define DEVICE_MANAGER_OTA_MANIFEST_PARAM_DATA "0"
/** @brief Name of the manifest action */
#define DEVICE_MANAGER_OTA_MANIFEST         "manifest"
/** @brief Name of ota software update package */
#define DEVICE_MANAGER_OTA_PACKAGE_NAME     "software_update_package"
/** @brief Time for an ota package downloading to expired in ms */
#define DEVICE_MANAGER_OTA_TRANSFER_EXPIRY_TIME 3600000u
/** @brief Number of main loop's iteration to check pending ota package downloading */
#define DEVICE_MANAGER_OTA_PACKAGE_CHECK_PENDING \
	DEVICE_MANAGER_OTA_TRANSFER_EXPIRY_TIME / POLL_INTERVAL_MSEC
/** @brief Operation type of ota local install*/
#define OTA_LOCAL_INSTALL_OPERATION        "local-install"

/**
 * @brief Callback function to handle ota action
 *
 * @param[in,out]  request             request from the cloud
 * @param[in,out]  user_data           pointer to user defined data
 *
 * @retval IOT_STATUS_BAD_PARAMETER    user data provided is invalid
 * @retval IOT_STATUS_SUCCESS          file download started
 * @retval IOT_STATUS_FAILURE          failed to start file download
 */
static iot_status_t device_manager_ota( iot_action_request_t *request,
					 void *user_data );
/**
 * @brief Function to pull data from a read archive and write it to a write handle
 *
 * @param[in]      ar              structure containing source archive data
 * @param[in]      aw              structure containing a handle to write
 *
 * @retval archive error code      on failure
 * @retval ARCHIVE_OK              on success
 */
int device_manager_ota_copy_data(struct archive *ar, struct archive *aw);
/**
 * @brief Execute ota install
 *
 * @param[in]  device_manager_info  pointer to device manager data structure 
 * @param[in]  operation_type       pointer to ota operation type
 * @param[in]  package_path         pointer to ota package directory
 * @param[in]  file_name            pointer to ota package name
 *
 * @retval IOT_STATUS_BAD_PARAMETER    on failure
 * @retval IOT_STATUS_FAILURE          on failure
 * @retval IOT_STATUS_SUCCESS          on success
 */
 iot_status_t device_manager_ota_install_execute(
	struct device_manager_info *device_manager_info, const char *operation_type,
	const char *package_path, const char* file_name );
/**
 * @brief  parameter adjustment
 *
 * @param[in,out]  command_param       param to be adjusted
 * @param[in]      word                the word to be deleted
 *
 * @return         the length of the input param after adjustment
*/
/*FIXME*/
/*static size_t device_manager_ota_manifest_del_characters(*/
/*char *command_param, const char *word );*/
/**
 * @brief  parameter adjustment
 *
 * @param[in,out]  command_param       param to be adjusted
 * @param[in]      word                the word to be deleted
 *
 * @return         the length of the input param after adjustment
*/
static iot_status_t device_manager_ota_extract_package(
	iot_t *iot_lib, const char *package_path, const char * file_name );
/**
 * @brief Function to extract ota package
 *
 * @param[in]      sw_update_package        ota package file
 *
 * @retval IOT_STATUS_FAILURE          on failure
 * @retval IOT_STATUS_SUCCESS          on success
 */
iot_status_t device_manager_ota_extract_package_perform(
	iot_t *iot_lib, const char * sw_update_package);

iot_status_t device_manager_ota_deregister(
	struct device_manager_info  *device_manager )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( device_manager )
	{
		iot_action_t *ota_manifest = device_manager->ota_manifest;
		iot_t *const iot_lib = device_manager->iot_lib;

		/* manifest(ota) action */
		result = iot_action_deregister( ota_manifest, NULL, 0u );
		if ( result == IOT_STATUS_SUCCESS )
		{
			iot_action_free( ota_manifest, 0u );
			device_manager->ota_manifest = NULL;
		}
		else
			IOT_LOG( iot_lib, IOT_LOG_ERROR,
				"Failed to deregister action %s",
				"manifest(ota)" );

		result = IOT_STATUS_SUCCESS;
	}
	return result;
}

iot_status_t device_manager_ota_register(
	struct device_manager_info  *device_manager )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( device_manager )
	{
		iot_t *const iot_lib = device_manager->iot_lib;
		iot_action_t *ota_manifest = NULL;

		/* manifest (i.e. OTA ) */
		ota_manifest = iot_action_allocate( iot_lib,
			DEVICE_MANAGER_OTA_MANIFEST );
		iot_action_parameter_add( ota_manifest,
			DEVICE_MANAGER_OTA_MANIFEST_PARAM_DATA,
			IOT_PARAMETER_IN_REQUIRED, IOT_TYPE_STRING, 0u );
		iot_action_flags_set( ota_manifest,
			IOT_ACTION_EXCLUSIVE_DEVICE );
		result = iot_action_register_callback( ota_manifest,
			&device_manager_ota,device_manager, NULL, 0u );
		if ( result == IOT_STATUS_SUCCESS )
		{
			device_manager->ota_manifest = ota_manifest;
			IOT_LOG( iot_lib, IOT_LOG_DEBUG,
			"Registered action: %s", DEVICE_MANAGER_OTA_MANIFEST );
		}
		else
			IOT_LOG( iot_lib, IOT_LOG_ERROR,
			"Failed to register action: %s; reason: %s",
			DEVICE_MANAGER_OTA_MANIFEST, iot_error( result ) );

		result = IOT_STATUS_SUCCESS;
	}
	return result;
}


iot_status_t device_manager_ota( iot_action_request_t *request, void *user_data )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	
	if ( request && user_data )
	{
		struct device_manager_info * device_manager_info =
			(struct device_manager_info * )user_data;
		struct device_manager_file_io_info * file_io =
			&device_manager_info->file_io_info;
		struct device_manager_file_transfer * transfer =
			file_io->file_transfer_ptr[file_io->file_transfer_count];
		struct device_manager_ota_manifest * ota_manifest_info =
			&transfer->ota_transfer_info;
		char operation[ DEVICE_MANAGER_OTA_MANIFEST_STRING_MAX_LENGTH + 1u ];
		iot_t *const iot_lib = device_manager_info->iot_lib;

		if ( result == IOT_STATUS_SUCCESS )
		{
			char sw_update_dir[ PATH_MAX + 1u ];
			char download_dir[ PATH_MAX + 1u ];
			const char *pkg_name = "sw-update-pkg";
			char package_name[ PATH_MAX + 1u ];
			unsigned int check_loop = 0;

			pkg_name = os_strrchr(
				ota_manifest_info->download_url,
				'/'
				);

			if ( pkg_name != NULL && *(pkg_name++) != '\0' )
				os_strncpy( package_name,
					pkg_name,
					PATH_MAX );
			else
				os_strncpy( package_name,
					DEVICE_MANAGER_OTA_PACKAGE_NAME,
					os_strlen ( DEVICE_MANAGER_OTA_PACKAGE_NAME) );

			/* set the software update and package download directories */
			if ( OS_STATUS_SUCCESS == os_make_path( sw_update_dir,
				PATH_MAX, device_manager_info->runtime_dir, "update", NULL ) )
			{
				/*
				 * Create update directory before starting.
				 * Clean old update directory if it exists
				 */
				if ( os_directory_exists( sw_update_dir ) )
					os_directory_delete( sw_update_dir,
						NULL, IOT_TRUE );

				result = os_directory_create(
					sw_update_dir,
					DIRECTORY_CREATE_MAX_TIMEOUT );

				os_strncpy( file_io->download_dir,
						download_dir, PATH_MAX );
			}
			if ( result == IOT_STATUS_SUCCESS )
			{
				/* download packages. Once csp ota function is ready, the
				 * other manifest information will be provided. Currently
				 * they are not required by epo http server. Just put them
				 * NULL now.
				 */
				/*FIXME*/
				/*result = device_manager_file_download_perform(*/
				/*device_manager_info,*/
				/*FILE_TRANSFER_OTA,*/
				/*ota_manifest_info->download_url,*/
				/*NULL,*/
				/*NULL,*/
				/*package_name,*/
				/*ota_manifest_info->checksum_sh256*/
				/*);*/

				/* try downloading again. After csp ota function is ready, the
				 * ota final result will response to csp, not return to dxl. and the
				 * try downloading will also be done in device main fuction.
				 * This might be temporary solution till csp is ready
				 */
				while ( result == IOT_STATUS_TRY_AGAIN &&
					check_loop < DEVICE_MANAGER_OTA_PACKAGE_CHECK_PENDING )
				{
					IOT_LOG( iot_lib , IOT_LOG_ERROR,
						"try to resume ota package download. loop times: %d",
						check_loop );
					os_time_sleep( POLL_INTERVAL_MSEC,
						IOT_FALSE );
					if ( transfer->state == FILE_TRANSFER_PENDING )
					{
						/*FIXME*/
						/*result = device_manager_file_transfer_perform(*/
						/*device_manager_info,*/
						/*transfer, 1 );*/
					}else
						break;
					check_loop ++;
				}

				if ( result == IOT_STATUS_SUCCESS)
					result = device_manager_ota_install_execute(
						device_manager_info,
						operation,
						download_dir,
						package_name);

				IOT_LOG( iot_lib , IOT_LOG_TRACE,
					"software update install result: %d", result );
			}
		}
	}
	return result;
}

/*FIXME*/
#if 0
size_t device_manager_ota_manifest_del_characters( 
		char *command_param, const char *word )
{
	size_t word_length = os_strlen( word );
	size_t param_length = 0;

	if ( word_length != 0 && command_param )
	{
		char *param = command_param;
		param_length = os_strlen( command_param );
		while ( ( param = os_strstr( param, word ) ) != NULL )
		{
			char *dst = param;
			char *src = param + word_length;
			os_memmove(dst, src, param_length + 1 );
			param_length -= word_length;
		}
	}
	return param_length;
}
#endif

iot_status_t device_manager_ota_install_execute(
	struct device_manager_info *device_manager_info,
	const char * operation_type,
	const char *package_path, const char* file_name )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( device_manager_info && package_path && package_path[0] != '\0' &&
		operation_type && operation_type[0] != '\0' &&
		file_name && file_name[0] != '\0')
	{
		char iot_update_dup_path[PATH_MAX + 1u] = "";
		char command_with_params[PATH_MAX + 1u];
		iot_t *const iot_lib = device_manager_info->iot_lib;

		result = device_manager_ota_extract_package(iot_lib, package_path, file_name);

		IOT_LOG( iot_lib, IOT_LOG_TRACE,
				"software update package_path: %s, file_name: %s",
				package_path,
				file_name);
		if ( result ==IOT_STATUS_SUCCESS )
		{
			char iot_update_path[PATH_MAX + 1u];
			char exec_dir[PATH_MAX + 1u];

			result = IOT_STATUS_EXECUTION_ERROR;
			app_path_executable_directory_get(exec_dir, PATH_MAX);
			if ( app_path_which( iot_update_path, PATH_MAX, exec_dir, IOT_UPDATE_TARGET) )
			{
				/**
				  * IDP system Truested Path Execution (TPE) protection
				  * restricts the execution of files under certain circumastances
				  * determined by their path. The copy of iot-update in the
				  * directory on IDP must have execution permissions. It's hard to
				  * guarantee the directory have such permission for all IDP security
				  * combinations. It's safe to use the default execution directory to
				  * execute the copy of iot-update.
				  * It is also applicable to other systems execpt for Android dut to it has
				  * other permission restriction.
				  */
				const char *iot_update_dup_dir = NULL;
#ifdef  __ANDROID__
				char temp_dir[PATH_MAX + 1];
				iot_update_dup_dir = os_directory_get_temp_dir(
					temp_dir, PATH_MAX );
#else
				iot_update_dup_dir = exec_dir;
#endif /* #ifdef __ANDROID__*/
				if ( OS_STATUS_SUCCESS == os_make_path(
					iot_update_dup_path,
					PATH_MAX,
					iot_update_dup_dir,
					IOT_UPDATE_TARGET"-copy"IOT_EXE_SUFFIX,
					NULL ) )
				{
					os_file_copy(
						iot_update_path,
						iot_update_dup_path );
				}
				if ( os_file_exists( iot_update_dup_path ) )
					os_snprintf( command_with_params,
						PATH_MAX,
						"\"%s\" --path \"%s\"",
						iot_update_dup_path,
						package_path );
				else
					os_snprintf( command_with_params,
						PATH_MAX,
						"\"%s\" --path \"%s\"",
						iot_update_path,
						package_path );
			
			}
		}

		if ( command_with_params[0] != '\0' )
		{
			char buf[1u] = "\0";
			char *out_buf[2u] = { buf, buf };
			size_t out_len[2u] = { 1u, 1u };
			int system_ret = 1;

			IOT_LOG( iot_lib, IOT_LOG_TRACE,
				"Executing command: %s", command_with_params );

			result = os_system_run_wait( command_with_params,
				&system_ret, out_buf, out_len, 0U );

			IOT_LOG( iot_lib, IOT_LOG_TRACE,
				"Completed executing OTA script with result: %i",
				system_ret );

			if ( system_ret != 0 )
				result = IOT_STATUS_EXECUTION_ERROR;
		}

		if ( ( iot_update_dup_path[0] != '\0' ) &&
		     ( os_file_exists( iot_update_dup_path ) != IOT_FALSE ) )
			os_file_delete( iot_update_dup_path );
	}

	return result;
}

iot_status_t device_manager_ota_extract_package(iot_t *iot_lib,
	const char *package_path, const char *file_name )
{
	iot_status_t result = IOT_STATUS_BAD_PARAMETER;
	if ( iot_lib && package_path && file_name )
	{
		char sw_update_package[ PATH_MAX + 1u ];

		if ( os_directory_exists ( package_path ) )
		{
			char cwd[1024u];
			os_file_chown( sw_update_package, IOT_USER );
			if ( os_directory_current( cwd, PATH_MAX ) == OS_STATUS_SUCCESS
				&& cwd[0] != '\0' )
			{
				if ( OS_STATUS_SUCCESS == os_directory_change(package_path) )
					IOT_LOG( iot_lib, IOT_LOG_TRACE,
						"Msg: Change current working directory to %s\n ",
						package_path );
			}

			/*
			 * extract ota package
			*/
			if ( os_file_exists ( file_name ) )
				result = device_manager_ota_extract_package_perform(
					iot_lib, file_name ) ;
			if ( cwd [0] != '\0')
				os_directory_change( cwd );
		}
	}
	return result;
}

iot_status_t device_manager_ota_extract_package_perform(
	iot_t *iot_lib, const char * sw_update_package)
{
	int result = IOT_STATUS_BAD_PARAMETER;
	if( sw_update_package )
	{
		struct archive *a;
		struct archive *ext;
		struct archive_entry *entry;
		int flags;

		result = IOT_STATUS_SUCCESS;
		/* Select which attributes we want to restore. */
		flags = ARCHIVE_EXTRACT_TIME;
		flags |= ARCHIVE_EXTRACT_PERM;
		flags |= ARCHIVE_EXTRACT_ACL;
		flags |= ARCHIVE_EXTRACT_FFLAGS;

		a = archive_read_new();
		archive_read_support_format_all(a);
		archive_read_support_filter_all(a);
		ext = archive_write_disk_new();
		archive_write_disk_set_options(ext, flags);
		archive_write_disk_set_standard_lookup(ext);
		if ( archive_read_open_filename(a, sw_update_package, 10240u)
			== ARCHIVE_OK)
		{
			int r;
			while ( result == IOT_STATUS_SUCCESS )
			{
				r = archive_read_next_header(a, &entry);

				if (r == ARCHIVE_EOF)
					break;
				if (r < ARCHIVE_OK)
				{
					IOT_LOG( iot_lib, IOT_LOG_ERROR,
						"Error: reading archive header: %s",
						archive_error_string(a));
						result = IOT_STATUS_FAILURE;
				}
				else if (r < ARCHIVE_WARN)
				{
					IOT_LOG( iot_lib, IOT_LOG_ERROR,
						"Error:reading archive header: %d",
						 r);
					result = IOT_STATUS_FAILURE;
				}
				if ( result == IOT_STATUS_SUCCESS)
				{
					r = archive_write_header(ext, entry);
					if (r < ARCHIVE_OK)
					{
						IOT_LOG( iot_lib, IOT_LOG_ERROR,
						"Error: writing archive header: %s",
						archive_error_string(ext));
						
						result = IOT_STATUS_FAILURE;
					}
					else if (archive_entry_size(entry) > 0)
					{
						r = device_manager_ota_copy_data(a, ext);
						if (r < ARCHIVE_OK)
						{
							IOT_LOG( iot_lib, IOT_LOG_ERROR,
							"Error: copy archive : %s",
							archive_error_string(ext));
							result = IOT_STATUS_FAILURE;
						}
						else if (r < ARCHIVE_WARN)
						{
							IOT_LOG( iot_lib, IOT_LOG_ERROR,
							"Error: copy archive: %d",
							r);
							result = IOT_STATUS_FAILURE;
						}
					}
				}
				if ( result == IOT_STATUS_SUCCESS )
				{
					r = archive_write_finish_entry(ext);
					if (r < ARCHIVE_OK)
					{
						IOT_LOG( iot_lib, IOT_LOG_ERROR,
						"Error: writing archive finish entry: %s",
						 archive_error_string(ext));
						result = IOT_STATUS_FAILURE;
					}
					else if (r < ARCHIVE_WARN)
					{
						IOT_LOG( iot_lib, IOT_LOG_ERROR,
						"Error: writing archive finish entry: %d",
						r);

						result = IOT_STATUS_FAILURE;
					}
				}
			}
		}
		else
			IOT_LOG( iot_lib, IOT_LOG_ERROR, "%s",
				"Error: open archive filename");
		archive_read_close(a);
		archive_read_free(a);
		archive_write_close(ext);
		archive_write_free(ext);
	}
	return result;
}

int device_manager_ota_copy_data(struct archive *ar, struct archive *aw)
{
	int r = ARCHIVE_WARN;
	const void *buff = NULL;
	size_t size;
	int64_t offset;
	int status = IOT_TRUE;

	if ( ar && aw )
	{
		while( status == IOT_TRUE)
		{
			r= archive_read_data_block(ar, &buff, &size, &offset);
			if (r == ARCHIVE_EOF)
			{
				status = IOT_FALSE;
				r = ARCHIVE_OK;
			}
			if ( r < ARCHIVE_OK)
				status = IOT_FALSE;

			if ( status == IOT_TRUE )
			{
				r = archive_write_data_block(aw, buff, size, offset);
				if (r < ARCHIVE_OK)
				{
					status = IOT_FALSE;
				}
			}
		}
	}
	return r;
}