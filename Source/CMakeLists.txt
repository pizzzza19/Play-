cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_SOURCE_DIR}/../deps/Dependencies/cmake-modules
	${CMAKE_MODULE_PATH}
)
include(Header)

project(PlayCore)

set(PROJECT_LIBS)
set(DEFINITIONS_LIST)

include(PrecompiledHeader)

set(ENABLE_AMAZON_S3 ON CACHE BOOL "Enable loading disc from Amazon S3 servers")

if(DEBUGGER_INCLUDED)
	list(APPEND DEFINITIONS_LIST DEBUGGER_INCLUDED=1)
endif()
if(PROFILE)
	list(APPEND DEFINITIONS_LIST PROFILE=1)
endif()
list(APPEND DEFINITIONS_LIST _IOP_EMULATE_MODULES=1)

if(USE_AOT_CACHE)
	list(APPEND DEFINITIONS_LIST AOT_USE_CACHE=1)
endif()

if(BUILD_AOT_CACHE)
	list(APPEND DEFINITIONS_LIST AOT_BUILD_CACHE=1)
endif()

if(TARGET_PLATFORM_UNIX AND NOT TARGET_PLATFORM_UNIX_ARM AND NOT TARGET_PLATFORM_UNIX_AARCH64)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2 -mssse3")
endif()

# targets, packages and dependencies
if(NOT TARGET Framework)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../deps/Framework/build_cmake/Framework
		${CMAKE_CURRENT_BINARY_DIR}/Framework
		EXCLUDE_FROM_ALL
	)
endif()
list(APPEND PROJECT_LIBS Framework)

if(NOT TARGET Framework_Http)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/../deps/Framework/build_cmake/FrameworkHttp
		${CMAKE_CURRENT_BINARY_DIR}/FrameworkHttp
	)
endif()
list(APPEND PROJECT_LIBS Framework_Http)

if(NOT TARGET app_shared)
	add_subdirectory(
		${CMAKE_CURRENT_SOURCE_DIR}/app_shared
		${CMAKE_CURRENT_BINARY_DIR}/app_shared
	)
endif()
