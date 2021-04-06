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

BEGIN_TEST_SUITE(test_project_perf)

END_TEST_SUITE(test_project_perf)
