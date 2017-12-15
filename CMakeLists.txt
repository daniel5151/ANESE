# I have no idea how cmake works
# But I want to use it for those sweet-sweet cross-platform builds

project(anese)
cmake_minimum_required(VERSION 2.8.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${anese_SOURCE_DIR}/cmake")
set(BIN_DIR ${anese_SOURCE_DIR}/bin)

# Bump up warning levels appropriately for clang, gcc & msvc and build in debug mode
if (NOT WIN32 AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2 /fp:fast")
  # suppress some MSVC warnings
  add_definitions(-D_CRT_SECURE_NO_WARNINGS) # unsafe funcs (fprintf and such)
  # 4458 - declaration of 'x' hides class member
  #   * non issue, since I tend to refer to class members through this->
  # 4310 - cast truncates constant value
  # 4068 - unknown pragma
  # 4244 - type conversion may lead to loss of data
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458 /wd4310 /wd4068 /wd4244")
  # and hey, let's add an icon too, for that extra bit of class

endif()

option(NESTEST "test CPU against NESTEST golden log" OFF)
if (NESTEST)
  add_definitions(-DNESTEST)
endif()

option(DEBUG_PPU "Display PPU debug windows" OFF)
if (DEBUG_PPU)
  add_definitions(-DDEBUG_PPU)
endif()

# ANESE executable

# anese source
include_directories(src)
file(GLOB_RECURSE SRC_FILES src/*.cc src/*.h)

# some platform Dependant icon stuff
if (WIN32)
  set(SRC_FILES ${SRC_FILES} resources/icons/anese.rc)
endif()

add_executable(anese ${SRC_FILES})

# Here is my very shitty way of doing 3rd party dependencies: Manually.

# ---- SDL2 ---- #
# since there is no standard install directory for sdl2 on windows, change this
# variable to point to _your_ SDL2 dev lib directory
set(SDL2_MORE_INCLUDE_DIR "C:/sdl2")

include_directories(${SDL2_INCLUDE_DIR})
find_package(SDL2 REQUIRED)
target_link_libraries(anese ${SDL2_LIBRARY})

# ---- args ---- #
add_subdirectory(thirdparty/args)
target_link_libraries(anese args)

# ---- tinyfiledialogs ---- #
set(TINY_FILE_DIALOGS_DIR thirdparty/tinyfiledialogs/)
include_directories(${TINY_FILE_DIALOGS_DIR})
add_library(TINY_FILE_DIALOGS STATIC ${TINY_FILE_DIALOGS_DIR}/tinyfiledialogs.c)
target_link_libraries(anese TINY_FILE_DIALOGS)

# target_link_libraries(anese ${NFD_LIBRARY})
install(TARGETS anese RUNTIME DESTINATION ${BIN_DIR})
