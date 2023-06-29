// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// These fixtures are to be used only with CPPUNITTEST
#define CPP_UNITTEST
#define USE_CTEST

#include "testrunnerswitcher.h"
#include "testmutex.h"

#include "c_logging/logger.h"

static TEST_MUTEX_HANDLE g_cppunittest_serialize_mutex;

void cppunittest_mutex_fixtures_suite_init(void)
{
    ASSERT_ARE_EQUAL(int, 0, logger_init());
    ASSERT_IS_NOT_NULL(g_cppunittest_serialize_mutex = TEST_MUTEX_CREATE());
}

void cppunittest_mutex_fixtures_suite_cleanup(void)
{
    TEST_MUTEX_DESTROY(g_cppunittest_serialize_mutex);
    logger_deinit();
}

void cppunittest_mutex_fixtures_function_init(void)
{
    if (TEST_MUTEX_ACQUIRE(g_cppunittest_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
}

void cppunittest_mutex_fixtures_function_cleanup(void)
{
    TEST_MUTEX_RELEASE(g_cppunittest_serialize_mutex);
}
