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

#include "testrunnerswitcher.h"

#ifdef CPP_UNITTEST
#include "cppunittest.h"
using namespace ::Microsoft::VisualStudio::CppUnitTestFramework;
#endif

BEGIN_TEST_SUITE(ctest_2_cppunittest_ut)

TEST_FUNCTION(ctest_2_cppunittest_struct_sizes_match) // no-srs // no-aaa
{
#ifdef CPP_UNITTEST
    // verify the sizes of the structures in ctest and cppunittest are the same
    ASSERT_ARE_EQUAL(size_t, sizeof(MethodMetadata), sizeof(CTEST_2_CPPUNITTEST_MethodMetadata));
    ASSERT_ARE_EQUAL(size_t, sizeof(MemberMethodInfo), sizeof(CTEST_2_CPPUNITTEST_MemberMethodInfo));
#endif
}

END_TEST_SUITE(ctest_2_cppunittest_ut)
