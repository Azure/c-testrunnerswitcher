#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName ctest_2_cppunittest_ut)

set(${theseTestsName}_test_files
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_cpp_files
    ${theseTestsName}.cpp
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_testrunnerswitcher")
