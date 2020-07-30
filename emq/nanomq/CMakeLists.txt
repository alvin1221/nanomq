#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required (VERSION 2.8.7)

project(nanomq)

aux_source_directory(. DIRSRCS)

include_directories(./ ./apps ./include)

add_subdirectory(apps)

set(PARALLEL 128 CACHE STRING "Parallelism (min 4, max 1000)")

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)

add_executable(server server.c)
add_executable(nanomq nanomq.c cmd.c file.c process.c apps.c property_handle.c)

target_link_libraries(nanomq apps)

target_link_libraries(server nng::nng)
target_compile_definitions(server PRIVATE -DPARALLEL=${PARALLEL})
