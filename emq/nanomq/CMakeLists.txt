#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 2.8.8)

project(nanomq)
SET(CMAKE_C_FLAGS "-std=c99")
SET(CMAKE_CXX_FLAGS "-std=c++11 -O3")

aux_source_directory(. DIRSRCS)

include_directories(./ ./apps ./include)

add_subdirectory(apps)

set(PARALLEL 128 CACHE STRING "Parallelism (min 4, max 1000)")

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)
# find_package(nanolib REQUIRED)

include_directories(../../emq/nanolib/include)

add_executable(nanomq nanomq.c cmd.c file.c process.c apps.c hash.cc mqtt_db.c zmalloc.c pub_handler.c subscribe_handle.c)

target_link_libraries(nanomq apps)

target_link_libraries(nanomq nng::nng)
target_compile_definitions(nanomq PRIVATE -DPARALLEL=${PARALLEL})
