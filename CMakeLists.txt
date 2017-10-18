# I have no idea how cmake works
# But I want to use it for those sweet-sweet cross-platform builds

project( anese )
cmake_minimum_required( VERSION 2.8.3 )

if(NOT MSVC)
	include( CheckCXXCompilerFlag )
	check_cxx_compiler_flag( "-std=c++11" COMPILER_SUPPORTS_CXX11 )
	if( COMPILER_SUPPORTS_CXX11 )
	    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -g" )
	else()
		message( FATAL_ERROR "Could not find C++ 11 capable compiler" )
	endif()
endif()

include_directories( src )
file(GLOB_RECURSE SRC_FILES src/*.cc src/*.h)

add_executable( anese ${SRC_FILES} )