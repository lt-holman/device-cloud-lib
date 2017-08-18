#
# Copyright (C) 2017 Wind River Systems, Inc. All Rights Reserved.
#
# The right to copy, distribute or otherwise make use of this software may
# be licensed only pursuant to the terms of an applicable Wind River license
# agreement.  No license to Wind River intellectual property rights is granted
# herein.  All rights not licensed by Wind River are reserved by Wind River.
#

set( TARGET "iot-control" )

# Convert a JSON Schema file to C source code
set( JSON_CMAKE_SCRIPT       "${CMAKE_SOURCE_DIR}/build-sys/cmake/scripts/json_schema_to_c.cmake" )
set( JSON_SCHEMA_VARIABLE    "IOT_CONNECT_SCHEMA" )
set( JSON_SCHEMA_FILE        "iot-connect.schema.json" )

set( JSON_SCHEMA_HEADER_FILE "${JSON_SCHEMA_FILE}.h" )
set( JSON_SCHEMA_SOURCE_FILE "${JSON_SCHEMA_FILE}.c" )

add_custom_command( OUTPUT
		"${CMAKE_CURRENT_BINARY_DIR}/${JSON_SCHEMA_HEADER_FILE}"
		"${CMAKE_CURRENT_BINARY_DIR}/${JSON_SCHEMA_SOURCE_FILE}"
	DEPENDS "${JSON_SCHEMA_FILE}"
	COMMAND "${CMAKE_COMMAND}"
		"-D" "JSON_SCHEMA_VARIABLE:STRING=${JSON_SCHEMA_VARIABLE}"
		"-D" "JSON_SCHEMA_FILE:PATH=${CMAKE_CURRENT_SOURCE_DIR}/${JSON_SCHEMA_FILE}"
		"-D" "JSON_SCHEMA_HEADER_FILE:PATH=${JSON_SCHEMA_HEADER_FILE}"
		"-D" "JSON_SCHEMA_SOURCE_FILE:PATH=${JSON_SCHEMA_SOURCE_FILE}"
		"-P" "${JSON_CMAKE_SCRIPT}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Generating C source & header files from JSON schema"
)

#################################################
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )


set( IOT_HDRS_C ${IOT_HDRS_C}
	"control_main.h"
	"control_config.h"
	"${CMAKE_CURRENT_BINARY_DIR}/${JSON_SCHEMA_HEADER_FILE}"
)

set( IOT_SRCS_C ${IOT_SRCS_C}
	"control.c"
	"control_main.c"
	"control_config.c"
	"${CMAKE_CURRENT_BINARY_DIR}/${JSON_SCHEMA_SOURCE_FILE}"
)

add_executable( "${TARGET}"
	${IOT_HDRS_C}
	${IOT_HDRS_CXX}
	${IOT_SRCS_C}
	${IOT_SRCS_CXX}
	${IOT_VERSION_FILES}
)

target_link_libraries( "${TARGET}"
	iot
	iotutils
)

# Installation instructions
install( TARGETS ${TARGET}
	RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT core
)