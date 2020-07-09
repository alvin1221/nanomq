# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/protocol/pair1
# Build directory: /projects/nanomq/build/src/protocol/pair1
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(pair1_test "/projects/nanomq/build/src/protocol/pair1/pair1_test" "-t")
set_tests_properties(pair1_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/pair1/CMakeLists.txt;19;nng_test;/projects/nanomq/src/protocol/pair1/CMakeLists.txt;0;")
add_test(pair1_poly_test "/projects/nanomq/build/src/protocol/pair1/pair1_poly_test" "-t")
set_tests_properties(pair1_poly_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/pair1/CMakeLists.txt;20;nng_test;/projects/nanomq/src/protocol/pair1/CMakeLists.txt;0;")
