#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required (VERSION 2.8.8)

project(nanolib)

#aux_source_directory(. DIRSRCS)

include_directories(./include)

#add_subdirectory(./packet_parser)

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)

# list of source files
set(libsrc hash.cc mqtt_db.c zmalloc.c)

# this is the "object library" target: compiles the sources only once
add_library(nanolib OBJECT ${libsrc})

# shared libraries need PIC
set_property(TARGET nanolib PROPERTY POSITION_INDEPENDENT_CODE 1)

# shared and static libraries built from the same object files
add_library(nanolib_shared SHARED $<TARGET_OBJECTS:nanolib>)
#add_library(nanolib_static STATIC $<TARGET_OBJECTS:nanolib>)


#add_executable(test hash.cc mqtt_db.c test.c zmalloc.c)
