# CMake generated Testfile for 
# Source directory: /projects/nanomq/tools/nngcat
# Build directory: /projects/nanomq/build/tools/nngcat
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(nngcat_async "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_async_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_async PROPERTIES  TIMEOUT "10" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;23;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_ambiguous "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_ambiguous_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_ambiguous PROPERTIES  TIMEOUT "2" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;24;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_need_proto "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_need_proto_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_need_proto PROPERTIES  TIMEOUT "2" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;25;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_dup_proto "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_dup_proto_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_dup_proto PROPERTIES  TIMEOUT "2" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;26;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_help "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_help_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_help PROPERTIES  TIMEOUT "2" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;27;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_incompat "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_incompat_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_incompat PROPERTIES  TIMEOUT "2" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;28;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_pubsub "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_pubsub_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_pubsub PROPERTIES  TIMEOUT "20" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;29;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_recvmaxsz "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_recvmaxsz_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_recvmaxsz PROPERTIES  TIMEOUT "20" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;30;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_unlimited "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_unlimited_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_unlimited PROPERTIES  TIMEOUT "20" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;31;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
add_test(nngcat_stdin_pipe "/bin/bash" "/projects/nanomq/tools/nngcat/nngcat_stdin_pipe_test.sh" "/projects/nanomq/build/tools/nngcat/nngcat")
set_tests_properties(nngcat_stdin_pipe PROPERTIES  TIMEOUT "20" _BACKTRACE_TRIPLES "/projects/nanomq/tools/nngcat/CMakeLists.txt;20;add_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;32;add_nngcat_test;/projects/nanomq/tools/nngcat/CMakeLists.txt;0;")
