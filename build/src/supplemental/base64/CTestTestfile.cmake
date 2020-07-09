# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/supplemental/base64
# Build directory: /projects/nanomq/build/src/supplemental/base64
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(base64_test "/projects/nanomq/build/src/supplemental/base64/base64_test" "-t")
set_tests_properties(base64_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/supplemental/base64/CMakeLists.txt;15;nng_test;/projects/nanomq/src/supplemental/base64/CMakeLists.txt;0;")
