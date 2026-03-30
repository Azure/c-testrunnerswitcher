# CMake generated Testfile for 
# Source directory: C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int
# Build directory: C:/src/04_bot/c-testrunnerswitcher/build/test_projects/test_project_int/exe
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(test_project_int "C:/src/04_bot/c-testrunnerswitcher/build/Debug/test_project_int_exe_testrunnerswitcher/test_project_int_exe_testrunnerswitcher.exe")
  set_tests_properties(test_project_int PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/Debug/test_project_int_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;16;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(test_project_int "C:/src/04_bot/c-testrunnerswitcher/build/Release/test_project_int_exe_testrunnerswitcher/test_project_int_exe_testrunnerswitcher.exe")
  set_tests_properties(test_project_int PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/Release/test_project_int_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;16;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(test_project_int "C:/src/04_bot/c-testrunnerswitcher/build/MinSizeRel/test_project_int_exe_testrunnerswitcher/test_project_int_exe_testrunnerswitcher.exe")
  set_tests_properties(test_project_int PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/MinSizeRel/test_project_int_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;16;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(test_project_int "C:/src/04_bot/c-testrunnerswitcher/build/RelWithDebInfo/test_project_int_exe_testrunnerswitcher/test_project_int_exe_testrunnerswitcher.exe")
  set_tests_properties(test_project_int PROPERTIES  WORKING_DIRECTORY "C:/src/04_bot/c-testrunnerswitcher/build/RelWithDebInfo/test_project_int_exe_testrunnerswitcher" _BACKTRACE_TRIPLES "C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;331;add_test;C:/src/04_bot/c-testrunnerswitcher/build_functions/CMakeLists.txt;394;build_exe;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;16;build_test_artifacts;C:/src/04_bot/c-testrunnerswitcher/test_projects/test_project_int/CMakeLists.txt;0;")
else()
  add_test(test_project_int NOT_AVAILABLE)
endif()
