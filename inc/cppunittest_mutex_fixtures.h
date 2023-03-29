// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CPPUNITTEST_MUTEX_FIXTURES_H
#define CPPUNITTEST_MUTEX_FIXTURES_H

#ifdef __cplusplus
extern "C" {
#endif

void cppunittest_mutex_fixtures_suite_init(void);
void cppunittest_mutex_fixtures_suite_cleanup(void);
void cppunittest_mutex_fixtures_function_init(void);
void cppunittest_mutex_fixtures_function_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // CPPUNITTEST_MUTEX_FIXTURES_H
