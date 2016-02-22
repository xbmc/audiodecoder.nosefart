project(audiodecoder.nosefart)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(platform REQUIRED)

add_subdirectory(lib/nosefart)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/nosefart/src)

set(NOSEFART_SOURCES src/NSFCodec.cpp)

set(DEPLIBS ${platform_LIBRARIES} nosefart)

build_addon(audiodecoder.nosefart NOSEFART DEPLIBS)

include(CPack)
