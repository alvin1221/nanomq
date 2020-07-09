# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/supplemental/sha1
# Build directory: /projects/nanomq/build/src/supplemental/sha1
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(sha1_test "/projects/nanomq/build/src/supplemental/sha1/sha1_test" "-t")
set_tests_properties(sha1_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/supplemental/sha1/CMakeLists.txt;12;nng_test;/projects/nanomq/src/supplemental/sha1/CMakeLists.txt;0;")
