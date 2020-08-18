
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.
# not finished yet #

cmake_minimum_required (VERSION 2.8.8)

project(nanomq-nng)

add_subdirectory(nng)
add_subdirectory(nanolib)
add_subdirectory(nanomq)

add_dependencies(nanomq nng)

