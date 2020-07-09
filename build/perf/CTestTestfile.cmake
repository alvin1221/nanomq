# CMake generated Testfile for 
# Source directory: /projects/nanomq/perf
# Build directory: /projects/nanomq/build/perf
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(inproc_lat "/projects/nanomq/build/perf/inproc_lat" "64" "10000")
set_tests_properties(inproc_lat PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/projects/nanomq/perf/CMakeLists.txt;46;add_test;/projects/nanomq/perf/CMakeLists.txt;0;")
add_test(inproc_thr "/projects/nanomq/build/perf/inproc_thr" "1400" "10000")
set_tests_properties(inproc_thr PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/projects/nanomq/perf/CMakeLists.txt;49;add_test;/projects/nanomq/perf/CMakeLists.txt;0;")
