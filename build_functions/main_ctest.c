// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>

#include "c_logging/logger.h"

#include "testrunnerswitcher.h"

int main(int argc, char* argv[])
{
    size_t failed_test_count = 0;

    // If a command line argument is provided, use it as a test name filter
    // to run only the test case matching that name
    const char* test_name_filter = (argc > 1) ? argv[1] : NULL;

    (void)logger_init();

    RUN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE, failed_test_count, test_name_filter);

    logger_deinit();

    return failed_test_count;
}
