````instructions
# c-testrunnerswitcher Copilot Instructions

## Project Overview
c-testrunnerswitcher is a lightweight abstraction library that enables seamless switching between two C/C++ test frameworks:
- **ctest**: Azure's custom C test framework (portable, cross-platform)
- **CppUnitTest**: Microsoft's Visual Studio native test framework (Windows-specific)

The library provides a unified API through macro abstraction, allowing the same test code to run under either framework based on compile-time flags.

## Architecture & Core Components

### Main Header - Unified Test API
- **`inc/testrunnerswitcher.h`**: Primary abstraction layer providing unified macros for both test frameworks
- **Compilation Modes**:
  - `USE_CTEST`: Routes to ctest framework (cross-platform, default)
  - `CPP_UNITTEST`: Routes to Visual Studio CppUnitTest (Windows-only)
  - Both defined: Hybrid mode with special metadata generation

### Framework Bridge Components
- **`inc/ctest_2_cppunittest.h`**: Windows-specific metadata generation to make ctest tests discoverable by Visual Studio Test Explorer
- **`inc/run_cppunittest_suite.h`**: Template for generating CppUnitTest wrapper executables
- **`build_functions/main_cpp_unittest.cpp`**: Template main file for CppUnitTest projects

### Test Synchronization Infrastructure
- **`inc/testmutex.h` & `src/testmutex.c`**: Windows mutex implementation for CppUnitTest thread safety
- **`inc/cppunittest_mutex_fixtures.h` & `src/cppunittest_mutex_fixtures.cpp`**: Test fixtures providing mutex synchronization

## Development Workflow

### Build Configuration
```cmake
# Standard integration pattern
if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-testrunnerswitcher/CMakeLists.txt))
    add_subdirectory(deps/c-testrunnerswitcher)
endif()

target_link_libraries(your_test_project testrunnerswitcher)
```

### Test Framework Selection
- **Default**: Pure ctest mode (`USE_CTEST` only)
- **Windows CppUnitTest**: Set `use_cppunittest=ON` in CMake to enable `CPP_UNITTEST` 
- **Hybrid Mode**: Both flags defined - enables Visual Studio Test Explorer integration while using ctest execution

### CMake Build Options
- `run_unittests=ON|OFF`: Build unit test projects
- `run_int_tests=ON|OFF`: Build integration test projects  
- `run_perf_tests=ON|OFF`: Build performance test projects
- `use_cppunittest=ON|OFF`: Enable CppUnitTest compilation on Windows

### Test Project Structure
Projects in `test_projects/` demonstrate usage patterns:
- **`test_project_ut/`**: Basic unit test structure
- **`test_project_with_custom_main_ut/`**: Custom main() function pattern (bypasses test execution)
- **`test_project_e2e/`**, **`test_project_int/`**, **`test_project_perf/`**: Category-specific test examples

## Unified Test API Patterns

### Test Suite Definition
```c
#include "testrunnerswitcher.h"

BEGIN_TEST_SUITE(MySuite)

TEST_FUNCTION(test_basic_functionality)
{
    // arrange
    int expected = 42;
    
    // act  
    int actual = get_answer();
    
    // assert
    ASSERT_ARE_EQUAL(int, expected, actual);
}

END_TEST_SUITE(MySuite)
```

### Test Fixtures (Setup/Teardown)
```c
// Suite-level fixtures
TEST_SUITE_INITIALIZE(MySuiteInit)
{
    // One-time suite setup
}

TEST_SUITE_CLEANUP(MySuiteCleanup)
{
    // One-time suite teardown
}

// Test-level fixtures
TEST_FUNCTION_INITIALIZE(MyTestInit)
{
    // Per-test setup
}

TEST_FUNCTION_CLEANUP(MyTestCleanup)
{
    // Per-test cleanup
}
```

### Assertion Macros
```c
// Standard assertions - work with both frameworks
ASSERT_ARE_EQUAL(type, expected, actual, optional_message_format, ...)
ASSERT_ARE_NOT_EQUAL(type, expected, actual, optional_message_format, ...)
ASSERT_IS_TRUE(expression, optional_message_format, ...)
ASSERT_IS_FALSE(expression, optional_message_format, ...)
ASSERT_IS_NULL(value, optional_message_format, ...)
ASSERT_IS_NOT_NULL(value, optional_message_format, ...)
ASSERT_FAIL(message_format, ...)

// Custom type support
TEST_DEFINE_ENUM_TYPE(MY_ENUM, VALUE1, VALUE2, VALUE3)
TEST_DEFINE_ENUM_TYPE_WITHOUT_INVALID(MY_ENUM, VALUE1, VALUE2)
```

### Test Execution (ctest mode only)
```c
// Basic execution
RUN_TEST_SUITE(MySuite);

// With memory leak detection retries
RUN_TEST_SUITE_WITH_LEAK_CHECK_RETRIES(MySuite);
```

### Thread Synchronization (CppUnitTest)
```c
// Thread-safe test execution
TEST_MUTEX_HANDLE mutex = TEST_MUTEX_CREATE();
if (TEST_MUTEX_ACQUIRE(mutex))
{
    // Critical section code
    TEST_MUTEX_RELEASE(mutex);
}
TEST_MUTEX_DESTROY(mutex);
```

## Framework-Specific Behavior

### ctest Mode (`USE_CTEST`)
- **Cross-platform**: Runs on Windows, Linux, macOS
- **Direct execution**: Tests run as regular console applications
- **Colored output**: ANSI color support for test results
- **Memory checking**: VLD integration on Windows

### CppUnitTest Mode (`CPP_UNITTEST`)
- **Windows-only**: Requires Visual Studio environment
- **Test Explorer integration**: Tests appear in VS Test Explorer
- **C++ compilation**: Tests must compile as C++ even for C code
- **Mutex synchronization**: Uses Windows mutex for thread safety

### Hybrid Mode (both flags)
- **ctest execution** with **CppUnitTest metadata**
- Enables Visual Studio Test Explorer discovery while using ctest runner
- Requires special fixture setup: `cppunittest_mutex_fixtures_suite_init`

## Key Implementation Patterns

### Macro Abstraction Strategy
The library uses extensive macro metaprogramming to provide identical API surface while routing to completely different underlying implementations:

```c
// Single API call...
TEST_FUNCTION(my_test) { /* test code */ }

// Expands to different implementations:
// ctest: CTEST_FUNCTION(my_test) { /* test code */ }
// CppUnitTest: TEST_METHOD(my_test) { /* test code */ }
```

### Message Formatting
CppUnitTest mode provides sophisticated message formatting using `ctrs_sprintf.h`:
```c
// Supports printf-style format strings
ASSERT_ARE_EQUAL(int, 42, actual, "Expected answer but got %d", actual);

// Automatically converts to wide strings for CppUnitTest
// Uses stack-based formatting to avoid allocation overhead
```

### Custom Type Integration
```c
// Define custom types for both frameworks
TEST_DEFINE_ENUM_TYPE(STATUS_CODE, 
    STATUS_SUCCESS,
    STATUS_FAILURE,
    STATUS_PENDING
);

// Automatically generates:
// - String conversion functions
// - Comparison operators  
// - CppUnitTest ToString<> specializations
```

### CPPUNITTEST_SYMBOL Linking
When using CppUnitTest mode, the library defines `CPPUNITTEST_SYMBOL` to force proper DLL linking:
```c
#ifdef CPPUNITTEST_SYMBOL
extern "C" void CPPUNITTEST_SYMBOL(void) {}
#endif
```

## Project Integration Guidelines

### Choosing Test Framework
- **Pure C projects**: Use ctest mode for maximum portability
- **Windows-specific projects**: Consider CppUnitTest for Visual Studio integration
- **Cross-platform with Windows development**: Use hybrid mode for best of both

### Test Organization
- Keep test suites focused on single components
- Use descriptive test function names: `component_with_invalid_input_fails`
- Leverage fixtures for common setup/teardown patterns
- Group related tests in single source files

### Build System Integration
The library integrates with the standard Azure C library build system via `c-build-tools`:
- Uses standard `build_test_artifacts()` function
- Supports all test categories: `_ut`, `_int`, `_e2e`, `_perf`
- Inherits Azure C library quality gates and pipeline templates

## External Dependencies

### Core Dependencies
For comprehensive coding standards and build patterns, refer to:
- **c-build-tools Copilot Instructions** (`deps/c-build-tools/.github/copilot-instructions.md`) - Build infrastructure, quality gates, coding standards
- **ctest Copilot Instructions** (`deps/ctest/.github/copilot-instructions.md`) - C test framework patterns, assertion usage  
- **c-logging Copilot Instructions** (`deps/c-logging/.github/copilot-instructions.md`) - Logging integration, error handling
- **macro-utils-c Copilot Instructions** (`deps/macro-utils-c/.github/copilot-instructions.md`) - Macro metaprogramming patterns

### Build Standards Compliance
**CRITICAL**: All code changes must strictly follow the comprehensive coding standards in `deps/c-build-tools/.github/general_coding_instructions.md`, including:
- Function naming conventions (snake_case with module prefixes)
- Parameter validation patterns and error handling
- Header inclusion order and memory management
- Requirements traceability system (SRS/Codes_SRS/Tests_SRS)
- Goto usage rules and async callback patterns

## Common Patterns & Anti-Patterns

### ✅ Recommended Patterns
```c
// Use unified macros consistently
BEGIN_TEST_SUITE(component_tests)
TEST_FUNCTION(component_with_valid_input_succeeds)
{
    ASSERT_ARE_EQUAL(int, 0, component_function(valid_input));
}
END_TEST_SUITE(component_tests)

// Leverage optional message formatting
ASSERT_ARE_EQUAL(int, expected, actual, "Operation failed with code %d", error_code);
```

### ❌ Anti-Patterns to Avoid
```c
// Don't use framework-specific APIs directly
#ifdef USE_CTEST
    CTEST_ASSERT_ARE_EQUAL(int, a, b);  // Wrong - breaks abstraction
#endif

// Don't forget CPPUNITTEST_SYMBOL in test files
// Missing: #ifdef CPPUNITTEST_SYMBOL ... #endif  // Will cause linking errors

// Don't mix framework calls in hybrid mode
RUN_TEST_SUITE(MySuite);  // Wrong in CppUnitTest - this is ctest-only
```

The test runner switcher's primary value is enabling identical test code to work across different execution environments while maintaining the specific advantages of each framework.

````
