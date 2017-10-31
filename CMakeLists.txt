# I have no idea how cmake works
# But I want to use it for those sweet-sweet cross-platform builds

project(anese)
cmake_minimum_required(VERSION 2.8.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${anese_SOURCE_DIR}/cmake")
set(BIN_DIR ${anese_SOURCE_DIR}/bin)

# Bump up warning levels appropriately for clang, gcc & msvc and build in debug mode
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
endif()

option(NESTEST "Move intial PC to 0xC000 for NESTEST" OFF)
if (NESTEST)
  add_definitions(-DNESTEST)
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
