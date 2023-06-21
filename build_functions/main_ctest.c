// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "c_logging/logger.h.h"

#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;

    (void)logger_init();

    RUN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE, failedTestCount);

    logger_deinit();

    return failedTestCount;
}
