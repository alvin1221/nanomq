# CMake generated Testfile for 
# Source directory: /projects/nanomq/src/protocol/survey0
# Build directory: /projects/nanomq/build/src/protocol/survey0
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(respond_test "/projects/nanomq/build/src/protocol/survey0/respond_test" "-t")
set_tests_properties(respond_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;26;nng_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;0;")
add_test(survey_test "/projects/nanomq/build/src/protocol/survey0/survey_test" "-t")
set_tests_properties(survey_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;27;nng_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;0;")
add_test(xrespond_test "/projects/nanomq/build/src/protocol/survey0/xrespond_test" "-t")
set_tests_properties(xrespond_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;28;nng_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;0;")
add_test(xsurvey_test "/projects/nanomq/build/src/protocol/survey0/xsurvey_test" "-t")
set_tests_properties(xsurvey_test PROPERTIES  TIMEOUT "180" _BACKTRACE_TRIPLES "/projects/nanomq/CMakeLists.txt;380;add_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;29;nng_test;/projects/nanomq/src/protocol/survey0/CMakeLists.txt;0;")
