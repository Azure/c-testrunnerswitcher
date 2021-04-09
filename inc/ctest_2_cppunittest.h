// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CTEST_2_CPPUNITTEST_H
#define CTEST_2_CPPUNITTEST_H

#ifdef CPP_UNITTEST

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

    // This structure matches 100% the structure used by CppUnitTest to store its metadata in the shared section where the metadata is stored
    typedef struct CTEST_2_CPPUNITTEST_MethodMetadata_TAG
    {
        const wchar_t* tag;
        const wchar_t* methodName;
        const unsigned char* helpMethodName;
        const unsigned char* helpMethodDecoratedName;
        const wchar_t* sourceFile;
        int lineNo;
    } CTEST_2_CPPUNITTEST_MethodMetadata;

    typedef enum CTEST_2_CPPUNITTEST_MemberMethodTypes_TAG
    {
        TestMethod,
        TestMethodSetup,
        TestMethodCleanup,
        TestObjectSetup,
        TestObjectCleanup,
        TestClassSetup,
        TestClassCleanup,
    } CTEST_2_CPPUNITTEST_MemberMethodTypes;

    typedef struct CTEST_2_CPPUNITTEST_MemberMethodInfo_TAG
    {
        CTEST_2_CPPUNITTEST_MemberMethodTypes methodType;
        union
        {
            void* pVoidMethod;
            void* pVoidStaticMethod;
        } method;

        const CTEST_2_CPPUNITTEST_MethodMetadata* metadata;
    } CTEST_2_CPPUNITTEST_MemberMethodInfo;

    // copied from CppUnitTest, do not have a better solution at the moment
#define ALLOCATE_TESTDATA_SECTION_VERSION __declspec(allocate("testvers$"))
#define ALLOCATE_TESTDATA_SECTION_CLASS __declspec(allocate("testdata$_A_class"))
#define ALLOCATE_TESTDATA_SECTION_METHOD __declspec(allocate("testdata$_B_method"))
#define ALLOCATE_TESTDATA_SECTION_ATTRIBUTE __declspec(allocate("testdata$_C_attribute"))

// unfortunately we have to create the sections before using them
// this code is also taken from CppUnitTest.h
#pragma section("testvers$", read, shared)
#pragma section("testdata$_A_class", read, shared)
#pragma section("testdata$_B_method", read, shared)
#pragma section("testdata$_C_attribute", read, shared)

// The reader of the metadata really really wants to have in the helperMethodName the class to which the "methods" belong
// Based on reverse engineering the code at https://devdiv.visualstudio.com/DevDiv/_git/VS?path=%2Fsrc%2Fvset%2FAgile%2FCppUnit%2FDiscoverer%2FTestMetaDataReaderForLatestVersion.cs&_a=contents&version=GBmain

// these macros are used by ctest to generate extra metadata needed by CppUnitTest
#define IMPLEMENT_CPP_UNIT_FIXTURE(infoTag, cppunittest_func_name, ctest_func_name, method_type) \
__declspec(dllexport) CTEST_2_CPPUNITTEST_MemberMethodInfo* __stdcall MU_C2(GetTestMethodInfo_, cppunittest_func_name)() \
{ \
    ALLOCATE_TESTDATA_SECTION_METHOD static const CTEST_2_CPPUNITTEST_MethodMetadata MU_C2(CppUnitTestMethodMetadata_, cppunittest_func_name) = \
        { L"" infoTag, L"" MU_TOSTRING(cppunittest_func_name), (const unsigned char*)"DummyTestClass::" MU_TOSTRING(cppunittest_func_name), (const unsigned char*)(__FUNCDNAME__) }; \
    static CTEST_2_CPPUNITTEST_MemberMethodInfo s_Info = { method_type, {NULL}, &MU_C2(CppUnitTestMethodMetadata_, cppunittest_func_name) }; \
    s_Info.method.pVoidStaticMethod = (void*)ctest_func_name; \
    return &s_Info; \
}

#define CTEST_CUSTOM_TEST_SUITE_INITIALIZE_CODE(func_name) \
    IMPLEMENT_CPP_UNIT_FIXTURE("TestClassInitializeInfo", func_name, TestSuiteInitialize, TestClassSetup)

#define CTEST_CUSTOM_TEST_SUITE_CLEANUP_CODE(func_name) \
    IMPLEMENT_CPP_UNIT_FIXTURE("TestClassCleanupInfo", func_name, TestSuiteCleanup, TestClassCleanup)

#define CTEST_CUSTOM_TEST_FUNCTION_INITIALIZE_CODE(func_name) \
    IMPLEMENT_CPP_UNIT_FIXTURE("TestMethodInitializeInfo", func_name, TestFunctionInitialize, TestMethodSetup)

#define CTEST_CUSTOM_TEST_FUNCTION_CLEANUP_CODE(func_name) \
    IMPLEMENT_CPP_UNIT_FIXTURE("TestMethodCleanupInfo", func_name, TestFunctionCleanup, TestMethodCleanup)

#define CTEST_CUSTOM_TEST_FUNCTION_CODE(func_name) \
__declspec(dllexport) CTEST_2_CPPUNITTEST_MemberMethodInfo* __stdcall MU_C2(GetTestMethodInfo_, func_name)() \
{ \
    ALLOCATE_TESTDATA_SECTION_METHOD static const CTEST_2_CPPUNITTEST_MethodMetadata MU_C2(CppUnitTestMethodMetadata_, func_name) = \
        { L"TestMethodInfo", L"" MU_TOSTRING(func_name), (const unsigned char*)("DummyTestClass::__GetTestMethodInfo_" MU_TOSTRING(func_name)), (const unsigned char*)(__FUNCDNAME__), L"" __FILE__, __LINE__ }; \
    static CTEST_2_CPPUNITTEST_MemberMethodInfo s_Info = { TestMethod, {NULL}, &MU_C2(CppUnitTestMethodMetadata_, func_name) }; \
    s_Info.method.pVoidStaticMethod = (void*)func_name; \
    return &s_Info; \
}

#ifdef __cplusplus
}
#endif

#endif

#endif /* CTEST_2_CPPUNITTEST_H */
