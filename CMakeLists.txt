cmake_minimum_required(VERSION 3.5)
project(audiodecoder.nosefart)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)

add_subdirectory(lib/nosefart)

include_directories(${KODI_INCLUDE_DIR}/..
                    ${PROJECT_SOURCE_DIR}/lib/nosefart/src)

set(NOSEFART_SOURCES src/NSFCodec.cpp)

set(DEPLIBS nosefart)

build_addon(audiodecoder.nosefart NOSEFART DEPLIBS)

include(CPack)
