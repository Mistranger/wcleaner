# wcleaner - a tool to clean out your PWAD from garbage

# General information
project(wcleaner)
cmake_minimum_required(VERSION 2.6)

# Version information
set(WCLEANER_MAJOR_VERSION 0)
set(WCLEANER_MINOR_VERSION 1)
set(WCLEANER_PATCH_VERSION 0)
set(WCLEANER_VERSION "${WCLEANER_MAJOR_VERSION}.${WCLEANER_MINOR_VERSION}.${WCLEANER_PATCH_VERSION}")

# Code

# include
include_directories(
	src/include
	${CMAKE_CURRENT_BINARY_DIR}
)
set(wcleaner_HEADERS
	src/include/config.h
	src/include/crc.h
	src/include/texopt.h
	src/include/textures.h
	src/include/util.h
	src/include/wad.h
	src/include/wcleaner.h
)

source_group(Include FILES ${wcleaner_HEADERS})

# sources
set(wcleaner_SOURCES
	src/config.c
	src/crc.c
	src/texopt.c
	src/util.c
	src/wad.c
	src/wcleaner.c
)
source_group(Sources FILES ${wcleaner_SOURCES})

add_executable(wcleaner WIN32 ${wcleaner_SOURCES} ${wcleaner_HEADERS})

set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib;.a")
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckSymbolExists)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")
endif()

set_target_properties(wcleaner PROPERTIES LINK_FLAGS "${LINK_FLAGS} -static-libgcc -static-libstdc++ -Wl,--stack,10485760 -mconsole")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")



