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

/* Helper function for parameterized test examples */
static int add_numbers(int a, int b)
{
    return a + b;
}

static int multiply_numbers(int a, int b)
{
    return a * b;
}

BEGIN_TEST_SUITE(test_project_ut)

TEST_FUNCTION(a) // no-srs // no-aaa
{

}

/*
 * Example 1: Parameterized test with 2 parameters
 * Tests that addition works correctly for various inputs
 */
PARAMETERIZED_TEST_FUNCTION(test_addition,
    ARGS(int, a, int, b, int, expected),
    CASE((0, 0, 0), with_zeros),
    CASE((1, 2, 3), with_small_positive_numbers),
    CASE((-1, 1, 0), with_negative_and_positive),
    CASE((100, 200, 300), with_larger_numbers))
{
    /* arrange - parameters are already set */

    /* act */
    int result = add_numbers(a, b);

    /* assert */
    ASSERT_ARE_EQUAL(int, expected, result, "Expected %d + %d = %d, but got %d", a, b, expected, result);
}

/*
 * Example 2: Parameterized test with 2 parameters for multiplication
 */
PARAMETERIZED_TEST_FUNCTION(test_multiplication,
    ARGS(int, x, int, y, int, expected_product),
    CASE((0, 5, 0), with_zero),
    CASE((1, 1, 1), with_ones),
    CASE((2, 3, 6), with_small_numbers),
    CASE((-2, 3, -6), with_negative_factor))
{
    /* act */
    int result = multiply_numbers(x, y);

    /* assert */
    ASSERT_ARE_EQUAL(int, expected_product, result);
}

/*
 * Example 3: Parameterized test with single parameter
 */
PARAMETERIZED_TEST_FUNCTION(test_is_positive,
    ARGS(int, value, int, expected_is_positive),
    CASE((1, 1), for_positive_one),
    CASE((100, 1), for_hundred),
    CASE((0, 0), for_zero),
    CASE((-1, 0), for_negative_one))
{
    /* act */
    int result = (value > 0) ? 1 : 0;

    /* assert */
    ASSERT_ARE_EQUAL(int, expected_is_positive, result);
}

END_TEST_SUITE(test_project_ut)
