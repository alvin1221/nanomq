# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/compat/nanomsg
# Build directory: /projects/nanomq/build/src/compat/nanomsg
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(compat_tcp_test "/projects/nanomq/build/src/compat/nanomsg/compat_tcp_test" "-t")
set_tests_properties(compat_tcp_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/compat/nanomsg/CMakeLists.txt;15;nng_test;/projects/nanomq/src/compat/nanomsg/CMakeLists.txt;0;")
