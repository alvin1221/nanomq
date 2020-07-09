# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/protocol/reqrep0
# Build directory: /projects/nanomq/build/src/protocol/reqrep0
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(req_test "/projects/nanomq/build/src/protocol/reqrep0/req_test" "-t")
set_tests_properties(req_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;26;nng_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;0;")
add_test(rep_test "/projects/nanomq/build/src/protocol/reqrep0/rep_test" "-t")
set_tests_properties(rep_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;27;nng_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;0;")
add_test(xrep_test "/projects/nanomq/build/src/protocol/reqrep0/xrep_test" "-t")
set_tests_properties(xrep_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;28;nng_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;0;")
add_test(xreq_test "/projects/nanomq/build/src/protocol/reqrep0/xreq_test" "-t")
set_tests_properties(xreq_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;29;nng_test;/projects/nanomq/src/protocol/reqrep0/CMakeLists.txt;0;")
