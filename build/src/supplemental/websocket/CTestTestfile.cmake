# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/supplemental/websocket
# Build directory: /projects/nanomq/build/src/supplemental/websocket
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(websocket_test "/projects/nanomq/build/src/supplemental/websocket/websocket_test" "-t")
set_tests_properties(websocket_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/supplemental/websocket/CMakeLists.txt;19;nng_test;/projects/nanomq/src/supplemental/websocket/CMakeLists.txt;0;")
