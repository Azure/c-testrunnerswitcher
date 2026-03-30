# CMake generated Testfile for 
# Source directory: C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut
# Build directory: C:/src/04_bot/c-testrunnerswitcher/build/test_projects/ctest_2_cppunittest_ut/exe
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ctest_2_cppunittest_ut "C:/src/04_bot/c-testrunnerswitcher/build/Debug/ctest_2_cppunittest_ut_exe_testrunnerswitcher/ctest_2_cppunittest_ut_exe_testrunnerswitcher.exe")
  set_tests_properties(ctest_2_cppunittest_ut PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/Debug/ctest_2_cppunittest_ut_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;19;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ctest_2_cppunittest_ut "C:/src/04_bot/c-testrunnerswitcher/build/Release/ctest_2_cppunittest_ut_exe_testrunnerswitcher/ctest_2_cppunittest_ut_exe_testrunnerswitcher.exe")
  set_tests_properties(ctest_2_cppunittest_ut PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/Release/ctest_2_cppunittest_ut_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;19;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ctest_2_cppunittest_ut "C:/src/04_bot/c-testrunnerswitcher/build/MinSizeRel/ctest_2_cppunittest_ut_exe_testrunnerswitcher/ctest_2_cppunittest_ut_exe_testrunnerswitcher.exe")
  set_tests_properties(ctest_2_cppunittest_ut PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/MinSizeRel/ctest_2_cppunittest_ut_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;19;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ctest_2_cppunittest_ut "C:/src/04_bot/c-testrunnerswitcher/build/RelWithDebInfo/ctest_2_cppunittest_ut_exe_testrunnerswitcher/ctest_2_cppunittest_ut_exe_testrunnerswitcher.exe")
  set_tests_properties(ctest_2_cppunittest_ut PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/RelWithDebInfo/ctest_2_cppunittest_ut_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;19;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/ctest_2_cppunittest_ut/CMakeLists.txt;0;")
else()
  add_test(ctest_2_cppunittest_ut NOT_AVAILABLE)
endif()
