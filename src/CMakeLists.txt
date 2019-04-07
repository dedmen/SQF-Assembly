cmake_minimum_required (VERSION 3.0)

set(INTERCEPT_CLIENT_PATH "${CMAKE_SOURCE_DIR}/intercept/src/client")

set(LIBRARY_PATH_ZSTD "${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/lib/zstd")

set(INTERCEPT_INCLUDE_PATH 
"${INTERCEPT_CLIENT_PATH}/headers"
"${INTERCEPT_CLIENT_PATH}/headers/client/"
"${INTERCEPT_CLIENT_PATH}/headers/client/sqf"
"${CMAKE_SOURCE_DIR}/intercept/src/host"
"${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/src"
"${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/lib/cpp-base64"
"${LIBRARY_PATH_ZSTD}/lib"
)

if(USE_64BIT_BUILD)
    set(INTERCEPT_PLUGIN_NAME "sqf-assembly_x64")
else()
    set(INTERCEPT_PLUGIN_NAME "sqf-assembly")
endif()


add_definitions(/DINTERCEPT_NO_THREAD_SAFETY)
add_definitions(/DNOMINMAX)
add_definitions(/DINTERCEPT_SQF_STRTYPE_RSTRING)
add_definitions(/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS) #No I don't cheat!
add_definitions(/DASC_INTERCEPT)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/rv/${INTERCEPT_PLUGIN_NAME}/intercept")

file(GLOB INTERCEPT_SOURCES "${INTERCEPT_CLIENT_PATH}/intercept/client/*.cpp"  "${INTERCEPT_CLIENT_PATH}/intercept/client/sqf/*.cpp" "${INTERCEPT_CLIENT_PATH}/intercept/shared/*.cpp")
SOURCE_GROUP("intercept" FILES ${INTERCEPT_SOURCES})


file(GLOB COMPILER_SOURCES "${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/src/scriptSerializer.cpp" "${CMAKE_SOURCE_DIR}/ArmaScriptCompiler/lib/cpp-base64/base64.cpp")
SOURCE_GROUP("scriptCompiler" FILES ${COMPILER_SOURCES})



file(GLOB_RECURSE INTERCEPT_SQFASM_SOURCES *.h *.hpp *.c *.cpp)
SOURCE_GROUP("src" FILES ${INTERCEPT_SQFASM_SOURCES})

set(library_sources ${INTERCEPT_SQFASM_SOURCES})

add_library( ${INTERCEPT_PLUGIN_NAME} SHARED ${library_sources} ${INTERCEPT_SOURCES} ${INTERCEPT_SQFASM_SQF_EXT_SOURCES} ${COMPILER_SOURCES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${INTERCEPT_INCLUDE_PATH} "${LIBRARY_PATH_ZSTD}/lib")

target_link_libraries(${INTERCEPT_PLUGIN_NAME} ${CMAKE_THREAD_LIBS_INIT} libzstd_static)

target_link_libraries(${INTERCEPT_PLUGIN_NAME} ${LINK_LIBS})
set_target_properties(${INTERCEPT_PLUGIN_NAME} PROPERTIES PREFIX "")
set_target_properties(${INTERCEPT_PLUGIN_NAME} PROPERTIES FOLDER intercept-cba)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(
		${INTERCEPT_PLUGIN_NAME} PRIVATE 
		"-std=c++1z"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
		"-s"
		"-fPIC"
		"-fpermissive"
		"-static-libgcc"
		"-static-libstdc++"
		"-march=i686"
		"-m32"
		"-Wno-ignored-attributes"
		"-static"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-ffunction-sections>"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-fdata-sections>"

	)
	target_link_options(${INTERCEPT_PLUGIN_NAME} PRIVATE "-m32" "-fPIC" "-static" "-static-libgcc" "-static-libstdc++"
	"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-Wl,--gc-sections>"
	"$<$<OR:$<CONFIG:Release>>:-Wl,--strip-all>"
	) 
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

	target_link_libraries(${INTERCEPT_PLUGIN_NAME} "libstdc++.a")
	set_target_properties(${INTERCEPT_PLUGIN_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
	set_target_properties(${INTERCEPT_PLUGIN_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
else()
	target_compile_options(
		${INTERCEPT_PLUGIN_NAME} PRIVATE 
		"/MP" "/Zi" "/EHsc"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/MT>"
		"$<$<CONFIG:Debug>:/MTd>"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ox>"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ob2>"
		"$<$<CONFIG:Debug>:/Ob0>"
		"$<$<CONFIG:Debug>:/Od>"
		"$<$<CONFIG:Debug>:/RTC1>"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Oi>"
		"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/Ot>"
	)
	target_link_options(${INTERCEPT_PLUGIN_NAME} PRIVATE "/OPT:REF" "/OPT:ICF" "/DEBUG:FULL") 
endif()

#Binary signing
if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sign.bat" AND MSVC)
	add_custom_command(TARGET ${INTERCEPT_PLUGIN_NAME}
		POST_BUILD
		COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sign.bat
		WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	)
endif()
