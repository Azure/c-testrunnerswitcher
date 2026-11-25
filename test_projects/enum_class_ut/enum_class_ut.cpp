// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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

/*stands in for an enum class owned by an external C++ SDK*/
namespace external_sdk
{
    enum class test_enum_class
    {
        kFirst = 3,
        kSecond = 7
    };
}

using TEST_ENUM_CLASS = external_sdk::test_enum_class;

#define TEST_ENUM_CLASS_VALUES \
    kFirst, \
    kSecond

TEST_DEFINE_ENUM_CLASS_TYPE(TEST_ENUM_CLASS, TEST_ENUM_CLASS_VALUES)

BEGIN_TEST_SUITE(enum_class_ut)

TEST_FUNCTION(ASSERT_ARE_EQUAL_with_equal_enum_class_values_succeeds) // no-srs
{
    // act
    ASSERT_ARE_EQUAL(TEST_ENUM_CLASS, TEST_ENUM_CLASS::kFirst, TEST_ENUM_CLASS::kFirst);
}

TEST_FUNCTION(ASSERT_ARE_NOT_EQUAL_with_different_enum_class_values_succeeds) // no-srs
{
    // act
    ASSERT_ARE_NOT_EQUAL(TEST_ENUM_CLASS, TEST_ENUM_CLASS::kFirst, TEST_ENUM_CLASS::kSecond);
}

END_TEST_SUITE(enum_class_ut)
