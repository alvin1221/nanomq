# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/protocol/pubsub0
# Build directory: /projects/nanomq/build/src/protocol/pubsub0
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(pub_test "/projects/nanomq/build/src/protocol/pubsub0/pub_test" "-t")
set_tests_properties(pub_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;26;nng_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;0;")
add_test(sub_test "/projects/nanomq/build/src/protocol/pubsub0/sub_test" "-t")
set_tests_properties(sub_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;27;nng_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;0;")
add_test(xsub_test "/projects/nanomq/build/src/protocol/pubsub0/xsub_test" "-t")
set_tests_properties(xsub_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;28;nng_test;/projects/nanomq/src/protocol/pubsub0/CMakeLists.txt;0;")
