// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CTRS_LEAK_CHECK_H
#define CTRS_LEAK_CHECK_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "testrunnerswitcher.h"

#ifdef VLD_OPT_REPORT_TO_STDOUT
// MAIN_RUN_TEST_SUITE_WITH_LEAK_CHECK macro with VLD leak checks
#define MAIN_RUN_TEST_SUITE_WITH_LEAK_CHECK(test_suite) \
int main(void) \
{ \
    size_t failedTestCount = 0; \
    VLD_UINT initial_leak_count = VLDGetLeaksCount(); \
    RUN_TEST_SUITE(test_suite, failedTestCount); \
    failedTestCount = (failedTestCount > 0) ? failedTestCount : -(int)(VLDGetLeaksCount() - initial_leak_count); \
    return failedTestCount; \
}
#else
// MAIN_RUN_TEST_SUITE_WITH_LEAK_CHECK macro without VLD leak checks
#define MAIN_RUN_TEST_SUITE_WITH_LEAK_CHECK(test_suite) \
int main(void) \
{ \
    size_t failedTestCount = 0; \
    RUN_TEST_SUITE(test_suite, failedTestCount); \
    return failedTestCount; \
}
#endif

#endif CTRS_LEAK_CHECK_H
