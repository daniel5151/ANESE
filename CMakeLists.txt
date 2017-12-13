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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458 /wd4310 /wd4068")
endif()

option(NESTEST "test CPU against NESTEST golden log" OFF)
if (NESTEST)
  add_definitions(-DNESTEST)
endif()

option(DEBUG_PPU "Display PPU debug windows" OFF)
if (DEBUG_PPU)
  add_definitions(-DDEBUG_PPU)
endif()


# since there is no standard install directory for sdl2 on windows, change this
# variable to suit your needs
set(SDL2_MORE_INCLUDE_DIR "C:/sdl2")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

include_directories(src)
file(GLOB_RECURSE SRC_FILES src/*.cc src/*.h)

add_executable(anese ${SRC_FILES})

target_link_libraries(anese ${SDL2_LIBRARY})
install(TARGETS anese RUNTIME DESTINATION ${BIN_DIR})
