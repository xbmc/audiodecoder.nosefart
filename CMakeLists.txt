project(audiodecoder.nosefart)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(xbmc REQUIRED)

add_subdirectory(lib/nosefart)

include_directories(${XBMC_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/nosefart/src)

set(NOSEFART_SOURCES src/NSFCodec.cpp)

set(DEPLIBS nosefart)

build_addon(audiodecoder.nosefart NOSEFART DEPLIBS)
