// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*the below symbol (CPPUNITTEST_SYMBOL) is defined by testrunnerswitcher.h for the purpose of linking the .dll when compiling as C++. Here there's no testrunnerswitcher so it is defined by hand.*/
/*it also serves (here) the purpose of not having an empty file*/
/*this code should be removed when a real project is build from this template*/
#ifdef CPPUNITTEST_SYMBOL
#ifdef __cplusplus
extern "C" {
#endif
    void CPPUNITTEST_SYMBOL(void) {}

#ifdef __cplusplus
}
#endif
#endif /*CPPUNITTEST_SYMBOL*/

#include "ctest_2_cppunittest.h"
#include "testrunnerswitcher.h"

#ifdef CPP_UNITTEST
#include "cppunittest.h"
using namespace ::Microsoft::VisualStudio::CppUnitTestFramework;

// these 2 variable definitions check that the size of the structures in ctest and cppunittest are the same
static int compile_time_check_CTEST_2_CPPUNITTEST_MethodMetadata_size[sizeof(CTEST_2_CPPUNITTEST_MethodMetadata) == sizeof(MethodMetadata) ? 1 : 0];
static int compile_time_check_CTEST_2_CPPUNITTEST_MemberMethodInfo_size[sizeof(CTEST_2_CPPUNITTEST_MemberMethodInfo) == sizeof(MemberMethodInfo) ? 1 : 0];
#endif

BEGIN_TEST_SUITE(ctest_2_cppunittest_ut)

END_TEST_SUITE(ctest_2_cppunittest_ut)
