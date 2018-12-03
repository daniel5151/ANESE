# I have no idea how cmake works, this is pretty much me copying and pasting
# stuff from around the web until things "work"
# I mean, that's how you learn, right?

project(anese)
cmake_minimum_required(VERSION 2.8.3)

if(APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)
endif(APPLE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${anese_SOURCE_DIR}/cmake")
set(BIN_DIR ${anese_SOURCE_DIR}/bin)

# anese source
include_directories(src)
file(GLOB_RECURSE SRC_FILES
  src/*.cc
  src/*.cpp
  src/*.h
  src/*.hpp
)

# ANESE executable
add_executable(anese ${SRC_FILES})

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Setup compiler flags for different platforms
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fno-exceptions -fno-rtti -Wno-class-memaccess -std=c++11")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -fsanitize=undefined -fsanitize=address")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -g")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /O2 /fp:fast")
  # suppress some MSVC warnings

  # suppress warnings about "unsafe" funcs (fprintf and such)
  target_compile_definitions(anese PRIVATE _CRT_SECURE_NO_WARNINGS)

  # 4458 - declaration of 'x' hides class member
  #   * non issue, since I always refer to class members through `this`
  # 4310 - cast truncates constant value
  # 4068 - unknown pragma
  # 4244 - type conversion may lead to loss of data
  # 4127 - conditianal expression is constant
  # 4800 - forcing value to bool 'true' or 'false' (performance warning)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4458 /wd4310 /wd4068 /wd4244 /wd4127 /wd4800")

  # add icon manifest to windows builds
  set(SRC_FILES ${SRC_FILES} resources/icons/anese.rc)
endif()

# handle various debug options
option(NESTEST "test CPU against NESTEST golden log" OFF)
if (NESTEST)
  target_compile_definitions(anese PRIVATE NESTEST)
endif()

# And now, for some shit-tier dependency management

# ---- header only libs ---- #
include_directories(thirdparty/headeronly)

# ---- SDL2 ---- #
# since there is no standard install directory for sdl2 on windows, change this
# variable to point to _your_ SDL2 dev lib directory
set(SDL2_MORE_INCLUDE_DIR "C:/sdl2")
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# ---- SDL_inprint ---- #
include_directories(thirdparty/SDL_inprint)
add_library(SDL_inprint STATIC thirdparty/SDL_inprint/SDL_inprint2.cc)

# ---- SimpleINI ---- #
include_directories(thirdparty/SimpleINI)
add_library(SimpleINI STATIC thirdparty/SimpleINI/ConvertUTF.c)

# ---- miniz ---- #
add_subdirectory(thirdparty/miniz)
include_directories(thirdparty/miniz)

# Finally, link up!
target_link_libraries(anese
  ${SDL2_LIBRARY}
  SDL_inprint
  SimpleINI
  miniz
)

if (APPLE)
  # Do some spooky macOS bundle magic that took far to long to figure out...
  # note: this is a brittle system, as it relies on the SDL2 version on homebrew
  #       to stay constant! Not ideal!
  install(CODE "
    set(CMAKE_INSTALL_LOCAL_ONLY true) # no need to install miniz!
    execute_process(COMMAND sh -c \"
      cp /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib ../resources/ANESE.app/Contents/Frameworks/
      chmod 755 ../resources/ANESE.app/Contents/Frameworks/libSDL2-2.0.0.dylib
      cp ./anese ../resources/ANESE.app/Contents/MacOS/
      install_name_tool -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @executable_path/../Frameworks/libSDL2-2.0.0.dylib ../resources/ANESE.app/Contents/MacOS/anese
      mkdir ../bin/
      cp -R ../resources/ANESE.app ../bin/
      echo 'Success! ANESE.app built in `bin` directory.'
    \")
  " COMPONENT Runtime)
else()
  install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY true)") # no need to install miniz!
  install(TARGETS anese RUNTIME DESTINATION ${BIN_DIR})
endif()
