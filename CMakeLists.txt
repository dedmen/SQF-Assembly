cmake_minimum_required (VERSION 3.6)
project (SQF-Assembly)

find_package (Threads)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(${PROJECT_SOURCE_DIR}/CMakeModules/cxx_compiler_functions.cmake)

set(USE_64BIT_BUILD ${CMAKE_CL_64})
option(USE_STATIC_LINKING "USE_STATIC_LINKING" ON)

if(USE_STATIC_LINKING) 
	message("WARNING: Linking statically")
	set(INTERCEPT_LINK_TYPE "static")
else()
	message("WARNING: Linking dynamically")
	set(INTERCEPT_LINK_TYPE "dynamic")
endif()


message("GENERATOR USED: '${CMAKE_GENERATOR}'")
message("COMPILER USED: '${CMAKE_CXX_COMPILER_ID}'")

if("${CMAKE_GENERATOR}" MATCHES "Visual Studio 15 2017 Win64")
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /std:c++latest" )
	set( USE_64BIT_BUILD ON)
elseif("${CMAKE_GENERATOR}" MATCHES "Visual Studio 15 2017")
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /std:c++latest" )
elseif("${CMAKE_GENERATOR}" MATCHES "Ninja")
	set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /std:c++latest" )
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	message(FATAL_ERROR "ERROR: You need a C++17 compatible compiler")
endif()

if(USE_64BIT_BUILD)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/win64/")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/win32/")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_INCLUDE_CURRENT_DIR ON) 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)


set(LIBRARY_PATH_ZSTD "${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/lib/zstd")

SET(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "no programs" FORCE)
set(ZSTD_USE_STATIC_RUNTIME ON CACHE BOOL "yes" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "no" FORCE)

add_subdirectory("${LIBRARY_PATH_ZSTD}/build/cmake")

add_subdirectory(src)
