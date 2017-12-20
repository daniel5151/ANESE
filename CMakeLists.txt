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

# anese source
include_directories(src)
file(GLOB_RECURSE SRC_FILES
  src/*.cc
  src/*.cpp
  src/*.h
  src/*.hpp
)

# some platform Dependant icon stuff
if (WIN32)
  set(SRC_FILES ${SRC_FILES} resources/icons/anese.rc)
endif()

# ANESE executable
add_executable(anese ${SRC_FILES})

# And now, for some shit-tier dependency management

# since there is no standard install directory for sdl2 on windows, change this
# variable to point to _your_ SDL2 dev lib directory
set(SDL2_MORE_INCLUDE_DIR "C:/sdl2")

# ---- SDL2 ---- #
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# ---- miniz ---- #
add_subdirectory(thirdparty/miniz)
include_directories(thirdparty/miniz)

# ---- args ---- #
add_subdirectory(thirdparty/args)
include_directories(thirdparty/args)

# ---- tinyfiledialogs ---- #
include_directories(thirdparty/tinyfiledialogs)
add_library(tinyfiledialogs STATIC thirdparty/tinyfiledialogs/tinyfiledialogs.c)

# ---- nes_snd_emu ---- #
include_directories(SYSTEM thirdparty/nes_snd_emu/nes_apu)
file(GLOB_RECURSE NES_SND_EMU_FILES
  thirdparty/nes_snd_emu/*.cpp
  thirdparty/nes_snd_emu/*.h)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set_source_files_properties(${NES_SND_EMU_FILES} PROPERTIES
    COMPILE_FLAGS "-O3 -Wno-everything") # it was written in 2005!
endif()
add_library(nes_snd_emu STATIC ${NES_SND_EMU_FILES})
target_include_directories(nes_snd_emu PUBLIC thirdparty/nes_snd_emu)

# Finally, link up!
target_link_libraries(anese ${SDL2_LIBRARY})
target_link_libraries(anese miniz)
target_link_libraries(anese args)
target_link_libraries(anese tinyfiledialogs)
target_link_libraries(anese nes_snd_emu)

install(TARGETS anese RUNTIME DESTINATION ${BIN_DIR})
