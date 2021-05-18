// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TESTRUNNERSWITCHER_H
#define TESTRUNNERSWITCHER_H

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
#include <cstring>
#include <cstdint>
#else
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#endif

#ifdef MBED_BUILD_TIMESTAMP
#define USE_CTEST
#endif

typedef void* TEST_MUTEX_HANDLE;

#ifdef USE_CTEST

#define TEST_DEFINE_ENUM_TYPE(type, ...) CTEST_DEFINE_ENUM_TYPE(type, __VA_ARGS__)
#define TEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, ...) CTEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, __VA_ARGS__)

// if both are defined do not forget to include the ctest to cpp unit test
#ifdef CPP_UNITTEST
#include "ctest_2_cppunittest.h"
#endif

#include "ctest.h" // IWYU pragma: export

#define BEGIN_TEST_SUITE(name)          CTEST_BEGIN_TEST_SUITE(name)
#define END_TEST_SUITE(name)            CTEST_END_TEST_SUITE(name)

#define TEST_SUITE_INITIALIZE(name)     CTEST_SUITE_INITIALIZE(name)
#define TEST_SUITE_CLEANUP(name)        CTEST_SUITE_CLEANUP(name)
#define TEST_FUNCTION_INITIALIZE(name)  CTEST_FUNCTION_INITIALIZE(name)
#define TEST_FUNCTION_CLEANUP(name)     CTEST_FUNCTION_CLEANUP(name)

#define TEST_FUNCTION(name)             CTEST_FUNCTION(name)

#define ASSERT_ARE_EQUAL                CTEST_ASSERT_ARE_EQUAL
#define ASSERT_ARE_NOT_EQUAL            CTEST_ASSERT_ARE_NOT_EQUAL
#define ASSERT_FAIL                     CTEST_ASSERT_FAIL
#define ASSERT_IS_NULL                  CTEST_ASSERT_IS_NULL
#define ASSERT_IS_NOT_NULL              CTEST_ASSERT_IS_NOT_NULL
#define ASSERT_IS_TRUE                  CTEST_ASSERT_IS_TRUE
#define ASSERT_IS_FALSE                 CTEST_ASSERT_IS_FALSE

#define RUN_TEST_SUITE(...)                         CTEST_RUN_TEST_SUITE(__VA_ARGS__)
#define RUN_TEST_SUITE_WITH_LEAK_CHECK_RETRIES(...) CTEST_RUN_TEST_SUITE_WITH_LEAK_CHECK_RETRIES(__VA_ARGS__)

#define TEST_MUTEX_CREATE()             (TEST_MUTEX_HANDLE)1
// the strlen check is simply to shut the compiler up and not create a hell of #pragma warning suppress
#define TEST_MUTEX_ACQUIRE(mutex)       (strlen("a") == 0)
#define TEST_MUTEX_RELEASE(mutex)
#define TEST_MUTEX_DESTROY(mutex)

#elif defined CPP_UNITTEST

#ifdef _MSC_VER
#pragma warning(disable:4505)
#endif

#include "CppUnitTest.h"
#include "testmutex.h"
#include "ctrs_sprintf.h"
#include "ctest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

typedef const char* char_ptr;
typedef const wchar_t* wchar_ptr;
typedef void* void_ptr;

#ifdef CPPUNITTEST_SYMBOL
extern "C" void CPPUNITTEST_SYMBOL(void) {}
#endif

#define BEGIN_TEST_SUITE(name)          TEST_CLASS(name) {

#define END_TEST_SUITE(name)            };

#define TEST_SUITE_INITIALIZE(name)     TEST_CLASS_INITIALIZE(name)
#define TEST_SUITE_CLEANUP(name)        TEST_CLASS_CLEANUP(name)
#define TEST_FUNCTION_INITIALIZE(name)  TEST_METHOD_INITIALIZE(name)
#define TEST_FUNCTION_CLEANUP(name)     TEST_METHOD_CLEANUP(name)

#define TEST_FUNCTION(name)             TEST_METHOD(name)

// these are generic macros for formatting the optional message
// they can be used in all the ASSERT macros without repeating the code over and over again
#define CONSTRUCT_CTRS_MESSAGE_FORMATTED(format, ...) \
    MU_IF(MU_COUNT_ARG(__VA_ARGS__), ctrs_sprintf_char(format, __VA_ARGS__), ctrs_sprintf_char(format));

#define CONSTRUCT_CTRS_MESSAGE_FORMATTED_EMPTY(...) \
    NULL

#define CONSTRUCT_CTRS_MESSAGE(...) \
    MU_IF(MU_COUNT_ARG(__VA_ARGS__), CONSTRUCT_CTRS_MESSAGE_FORMATTED, CONSTRUCT_CTRS_MESSAGE_FORMATTED_EMPTY)(__VA_ARGS__)

#define ASSERT_ARE_EQUAL(type, A, B, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::AreEqual((type)(A), (type)(B), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_ARE_NOT_EQUAL(type, A, B, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::AreNotEqual((type)(A), (type)(B), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_FAIL(...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::Fail(cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_IS_TRUE(expression, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::IsTrue((expression), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_IS_FALSE(expression, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::IsFalse((expression), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_IS_NOT_NULL(value, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::IsNotNull((value), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define ASSERT_IS_NULL(value, ...) \
    do \
    { \
        char* ctrs_message = CONSTRUCT_CTRS_MESSAGE(__VA_ARGS__); \
        std::wstring cppUnitTestMessage = ToString(ctrs_message); \
        ctrs_sprintf_free(ctrs_message); \
        Assert::IsNull((value), cppUnitTestMessage.c_str()); \
    } while ((void)0, 0)

#define RUN_TEST_SUITE(...)
#define RUN_TEST_SUITE_WITH_LEAK_CHECK_RETRIES(...)

#define TEST_MUTEX_CREATE()                                 testmutex_create()
#define TEST_MUTEX_ACQUIRE(mutex)                           testmutex_acquire(mutex)
#define TEST_MUTEX_RELEASE(mutex)                           testmutex_release(mutex)
#define TEST_MUTEX_DESTROY(mutex)                           testmutex_destroy(mutex)

#define TEST_USE_CTEST_FUNCTIONS_FOR_TYPE(my_type) \
namespace Microsoft \
{ \
    namespace VisualStudio \
    { \
        namespace CppUnitTestFramework \
        { \
            template<> \
            inline std::wstring ToString<my_type>(const my_type& value) \
            { \
                char temp_str[1024]; \
                std::wstring result; \
                if (MU_C2(my_type,_ToString)(temp_str, sizeof(temp_str), value) != 0) \
                { \
                    return L""; \
                } \
                else \
                { \
                    int size_needed_in_chars = MultiByteToWideChar(CP_UTF8, 0, &temp_str[0], -1, NULL, 0); \
                    if (size_needed_in_chars == 0) \
                    { \
                        result = L""; \
                    } \
                    else \
                    { \
                        WCHAR* widechar_string = (WCHAR*)malloc(size_needed_in_chars * sizeof(WCHAR)); \
                        if (widechar_string == NULL) \
                        { \
                            result = L""; \
                        } \
                        else \
                        { \
                            if (MultiByteToWideChar(CP_UTF8, 0, temp_str, -1, widechar_string, size_needed_in_chars) == 0) \
                            { \
                                result = L""; \
                            } \
                            else \
                            { \
                                result = std::wstring(widechar_string); \
                            } \
                            free(widechar_string); \
                        } \
                    } \
                } \
                return result; \
            } \
            template<> \
            static void Assert::AreEqual<my_type>(const my_type& expected, const my_type& actual, const wchar_t* message, const __LineInfo* pLineInfo) \
            { \
                FailOnCondition((MU_C2(my_type,_Compare)(expected, actual) == 0), EQUALS_MESSAGE(expected, actual, message), pLineInfo); \
            } \
        } \
    } \
} \

#define TEST_DEFINE_ENUM_TYPE(type, ...) \
    CTEST_DEFINE_ENUM_TYPE(type) \
    TEST_USE_CTEST_FUNCTIONS_FOR_TYPE(type)

#define TEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, ...) \
    CTEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type) \
    TEST_USE_CTEST_FUNCTIONS_FOR_TYPE(type)

/*because for some reason this is not defined by Visual Studio, it is defined here, so it is not multiplied in every single other unittest*/
namespace Microsoft
{
    namespace VisualStudio
    {
        namespace CppUnitTestFramework
        {

/*Visual Studio 2019 version 16.1.0 has in C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\VS\UnitTest\include\CppUnitTestAssert.h the same template*/
#if _MSC_VER < 1921
            template<> inline std::wstring ToString<int64_t>(const int64_t& t)
            {
                RETURN_WIDE_STRING(t);
            }
#endif
            template<> inline std::wstring ToString<uint16_t>(const uint16_t& t)
            {
                RETURN_WIDE_STRING(t);
            }
        }
    }
}

#else
#error No test runner defined
#endif

#endif
