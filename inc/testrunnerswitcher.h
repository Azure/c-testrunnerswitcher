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

#include "cppunittest_mutex_fixtures.h"

typedef void* TEST_MUTEX_HANDLE;

// a macro useful for disabling tests while debugging
#define DISABLED_TEST_FUNCTION(name)    void name(void)

/*
 * PARAMETERIZED_TEST_FUNCTION - A macro for defining parameterized test functions
 *
 * Usage:
 *   PARAMETERIZED_TEST_FUNCTION(base_name,
 *       ARGS(type1, param1, type2, param2),
 *       CASE((value1, value2), suffix1),
 *       CASE((value3, value4), suffix2))
 *   {
 *       // test code using param1, param2
 *   }
 *
 * This expands to:
 *   - A static function declaration
 *   - Multiple TEST_FUNCTION wrappers that call the static function with the specified arguments
 *   - The static function definition with the test body
 *
 * Example:
 *   PARAMETERIZED_TEST_FUNCTION(test_addition,
 *       ARGS(int, a, int, b, int, expected),
 *       CASE((1, 2, 3), when_adding_1_and_2),
 *       CASE((0, 0, 0), when_adding_zeros))
 *   {
 *       ASSERT_ARE_EQUAL(int, expected, a + b);
 *   }
 */

/* Helper macros for extracting parts from ARGS(...) */
/* ARGS format: (type1, name1, type2, name2, ...) - uses MU_FOR_EACH_2 */

/* Add comma before parameter declaration (used for all but first) */
#define PTRS_ARGS_DECL_REST_DO(type, name) , type name
#define PTRS_ARGS_DECL_REST(...) MU_FOR_EACH_2(PTRS_ARGS_DECL_REST_DO, __VA_ARGS__)

/* Main ARGS declaration - handles 2+ args (1+ pairs) */
#define PTRS_ARGS_DECL_2(t1, n1) t1 n1
#define PTRS_ARGS_DECL_MORE(t1, n1, ...) t1 n1 PTRS_ARGS_DECL_REST(__VA_ARGS__)
#define PTRS_ARGS_DECL_DISPATCH_0(...) PTRS_ARGS_DECL_MORE(__VA_ARGS__)
#define PTRS_ARGS_DECL_DISPATCH_1(...) PTRS_ARGS_DECL_2(__VA_ARGS__)
#define PTRS_ARGS_DECL(...) MU_C2(PTRS_ARGS_DECL_DISPATCH_, MU_ISEMPTY(MU_IF(MU_ISZERO(MU_DEC(MU_DEC(MU_COUNT_ARG(__VA_ARGS__)))), 1, )))(__VA_ARGS__)

/* The ARGS wrapper just passes through */
#define ARGS(...) __VA_ARGS__

/* Strip parentheses helper */
#define PTRS_STRIP_PARENS(...) __VA_ARGS__

/* Generate a single TEST_FUNCTION wrapper for a CASE */
/* This is called with (base_name, (values), suffix) - values wrapped in parens */
#define PTRS_GENERATE_ONE_WRAPPER_IMPL(base_name, values, suffix) \
    TEST_FUNCTION(MU_C3(base_name, _, suffix)) \
    { \
        MU_C2(base_name, _impl)(PTRS_STRIP_PARENS values); \
    }

/* Helper to call IMPL with expanded args - this forces argument re-parsing */
#define PTRS_GENERATE_ONE_WRAPPER_CALL(base_name, ...) PTRS_GENERATE_ONE_WRAPPER_IMPL(base_name, __VA_ARGS__)

/* When we paste PTRS_EXPAND_CASE_ with CASE(values, suffix), we get PTRS_EXPAND_CASE_CASE(values, suffix) */
/* which expands to just: values, suffix (removing the CASE wrapper) */
#define PTRS_EXPAND_CASE_CASE(values, suffix) values, suffix

/* This is called by MU_FOR_EACH_1_KEEP_1 with (base_name, CASE((vals), suffix)) */
/* We use MU_C2B to paste PTRS_EXPAND_CASE_ with the case_item (which starts with CASE) */
/* This gives us PTRS_EXPAND_CASE_CASE((vals), suffix) which expands to (vals), suffix */
/* PTRS_GENERATE_ONE_WRAPPER_CALL uses __VA_ARGS__ to allow the expansion to split into multiple args */
#define PTRS_GENERATE_ONE_WRAPPER(base_name, case_item) PTRS_GENERATE_ONE_WRAPPER_CALL(base_name, MU_C2B(PTRS_EXPAND_CASE_, case_item))

/* Main PARAMETERIZED_TEST_FUNCTION macro */
/* Usage: PARAMETERIZED_TEST_FUNCTION(name, ARGS(...), CASE((vals), suffix), ...) { body } */
#define PARAMETERIZED_TEST_FUNCTION(base_name, args, ...) \
    static void MU_C2(base_name, _impl)(PTRS_ARGS_DECL(args)); \
    MU_FOR_EACH_1_KEEP_1(PTRS_GENERATE_ONE_WRAPPER, base_name, __VA_ARGS__) \
    static void MU_C2(base_name, _impl)(PTRS_ARGS_DECL(args))

#ifdef USE_CTEST

#define TEST_DEFINE_ENUM_TYPE(type, ...) CTEST_DEFINE_ENUM_TYPE(type, __VA_ARGS__)
#define TEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, ...) CTEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, __VA_ARGS__)

// if both are defined do not forget to include the ctest to cpp unit test
#ifdef CPP_UNITTEST
#include "ctest_2_cppunittest.h"

// These macros support optional variadic fixture functions that are passed through to CTEST_SUITE_*/CTEST_FUNCTION_*.
// For INITIALIZE macros: cppunittest mutex fixture runs first, then any user-provided fixtures.
// For CLEANUP macros: user-provided fixtures run first, then cppunittest mutex fixture runs last.
#define TEST_SUITE_INITIALIZE(name, ...)     CTEST_SUITE_INITIALIZE(name, cppunittest_mutex_fixtures_suite_init, ##__VA_ARGS__)
#define TEST_SUITE_CLEANUP(name, ...)        CTEST_SUITE_CLEANUP(name, ##__VA_ARGS__, cppunittest_mutex_fixtures_suite_cleanup)
#define TEST_FUNCTION_INITIALIZE(name, ...)  CTEST_FUNCTION_INITIALIZE(name, cppunittest_mutex_fixtures_function_init, ##__VA_ARGS__)
#define TEST_FUNCTION_CLEANUP(name, ...)     CTEST_FUNCTION_CLEANUP(name, ##__VA_ARGS__, cppunittest_mutex_fixtures_function_cleanup)

#else

// These macros support optional variadic fixture functions that are passed through to CTEST_SUITE_*/CTEST_FUNCTION_*.
#define TEST_SUITE_INITIALIZE(name, ...)     CTEST_SUITE_INITIALIZE(name, ##__VA_ARGS__)
#define TEST_SUITE_CLEANUP(name, ...)        CTEST_SUITE_CLEANUP(name, ##__VA_ARGS__)
#define TEST_FUNCTION_INITIALIZE(name, ...)  CTEST_FUNCTION_INITIALIZE(name, ##__VA_ARGS__)
#define TEST_FUNCTION_CLEANUP(name, ...)     CTEST_FUNCTION_CLEANUP(name, ##__VA_ARGS__)

#endif

#include "ctest.h" // IWYU pragma: export

#define BEGIN_TEST_SUITE(name)          CTEST_BEGIN_TEST_SUITE(name)
#define END_TEST_SUITE(name)            CTEST_END_TEST_SUITE(name)

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

typedef char* char_ptr;
typedef wchar_t* wchar_ptr;
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
                MU_C2(my_type,_ToString)(temp_str, sizeof(temp_str), value); \
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
    CTEST_DEFINE_ENUM_TYPE(type, __VA_ARGS__) \
    TEST_USE_CTEST_FUNCTIONS_FOR_TYPE(type)

#define TEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, ...) \
    CTEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(type, __VA_ARGS__) \
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
/*Visual Studio 2022 version 17.5.3 has in C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\VS\UnitTest\include\CppUnitTestAssert.h the same template*/
#if _MSC_VER < 1935
            template<> inline std::wstring ToString<uint16_t>(const uint16_t& t)
            {
                RETURN_WIDE_STRING(t);
            }
#endif
        }
    }
}

#else
#error No test runner defined
#endif

#endif
