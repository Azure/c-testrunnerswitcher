// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>
#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    // test suite name comes from the CMakeLists.Txt
    RUN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE, failedTestCount);
    return failedTestCount;
}

