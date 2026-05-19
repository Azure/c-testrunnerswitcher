// Copyright (c) Microsoft. All rights reserved.
//
// Integration tests for test_watchdog.exe.
//
// Implemented as a plain Win32 console application (NOT a ctest-framework
// suite) because:
//   (a) the watchdog deliberately has no project-library dependencies, and
//   (b) we want a self-contained test exe that can be invoked directly by
//       CMake's `ctest` with a simple pass/fail exit code, without dragging
//       in c-logging or umock-c just to print test names.
//
// Each test_* function returns 0 on success, nonzero on failure. main()
// runs them in order, prints concise PASS/FAIL lines to stderr, and exits
// with the number of failures (clamped to 1..127).

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include <windows.h>

#define TWD_EXIT_TIMEOUT    124
#define TWD_EXIT_USAGE      2
#define MDMP_SIGNATURE      0x504d444d

// Resolved at runtime in main() from the path of this test exe — the
// watchdog binary lives alongside it. Resolving at runtime (instead of
// baking a build-time path via a CMake-defined macro) is essential for
// CI: build agent and test agent generally have different work-directory
// paths, so a compile-time absolute path would be wrong on the test
// agent.
static wchar_t TWD_EXE[MAX_PATH * 2];
static wchar_t g_test_root[MAX_PATH];

// ----- assertion helpers -----------------------------------------------------

#define TEST_FAIL_FMT(fmt, ...) \
    do { \
        fprintf(stderr, "    FAIL %s:%d: " fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
        return 1; \
    } while (0)

#define EXPECT_EQ_INT(actual, expected) \
    do { \
        int _a = (actual); int _e = (expected); \
        if (_a != _e) { \
            fprintf(stderr, "    FAIL %s:%d: expected %d, got %d\n", __FILE__, __LINE__, _e, _a); \
            return 1; \
        } \
    } while (0)

#define EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "    FAIL %s:%d: '%s' was false\n", __FILE__, __LINE__, #expr); \
            return 1; \
        } \
    } while (0)

// ----- support utilities -----------------------------------------------------

static void make_test_subdir(const wchar_t* test_name, wchar_t* out, size_t out_chars)
{
    (void)swprintf_s(out, out_chars, L"%ls\\%ls", g_test_root, test_name);
    (void)CreateDirectoryW(out, NULL);
    // wipe any leftovers from a prior run
    wchar_t pattern[MAX_PATH * 2];
    (void)swprintf_s(pattern, sizeof(pattern) / sizeof(pattern[0]), L"%ls\\*", out);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                wchar_t fp[MAX_PATH * 2];
                (void)swprintf_s(fp, sizeof(fp) / sizeof(fp[0]), L"%ls\\%ls", out, fd.cFileName);
                (void)DeleteFileW(fp);
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
}

// Quote a single argument per Win32 CommandLineToArgvW rules and append to dst.
static void append_arg(wchar_t* dst, size_t dst_chars, const wchar_t* arg)
{
    size_t cur = wcslen(dst);
    if (cur > 0)
    {
        if (cur + 2 < dst_chars) { dst[cur++] = L' '; dst[cur] = L'\0'; }
    }
    bool needs_quote = (arg[0] == L'\0') || (wcspbrk(arg, L" \t\"") != NULL);
    if (!needs_quote)
    {
        (void)wcscat_s(dst, dst_chars, arg);
        return;
    }
    if (cur + 1 < dst_chars) { dst[cur++] = L'"'; dst[cur] = L'\0'; }
    size_t bs = 0;
    for (const wchar_t* p = arg; *p != L'\0'; p++)
    {
        cur = wcslen(dst);
        if (*p == L'\\')
        {
            bs++;
            if (cur + 1 < dst_chars) { dst[cur++] = L'\\'; dst[cur] = L'\0'; }
        }
        else if (*p == L'"')
        {
            for (size_t k = 0; k < bs + 1; k++)
            {
                cur = wcslen(dst);
                if (cur + 1 < dst_chars) { dst[cur++] = L'\\'; dst[cur] = L'\0'; }
            }
            cur = wcslen(dst);
            if (cur + 1 < dst_chars) { dst[cur++] = L'"'; dst[cur] = L'\0'; }
            bs = 0;
        }
        else
        {
            if (cur + 1 < dst_chars) { dst[cur++] = *p; dst[cur] = L'\0'; }
            bs = 0;
        }
    }
    for (size_t k = 0; k < bs; k++)
    {
        cur = wcslen(dst);
        if (cur + 1 < dst_chars) { dst[cur++] = L'\\'; dst[cur] = L'\0'; }
    }
    cur = wcslen(dst);
    if (cur + 1 < dst_chars) { dst[cur++] = L'"'; dst[cur] = L'\0'; }
}

// Spawn TWD_EXE with the given arguments. Returns the exit code, or -1 if
// the watchdog itself failed to spawn or did not exit within wait_seconds.
static int run_watchdog(int argc, const wchar_t** argv, uint32_t wait_seconds)
{
    wchar_t cmdline[4096];
    cmdline[0] = L'\0';
    append_arg(cmdline, sizeof(cmdline) / sizeof(cmdline[0]), TWD_EXE);
    for (int i = 0; i < argc; i++)
    {
        append_arg(cmdline, sizeof(cmdline) / sizeof(cmdline[0]), argv[i]);
    }

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL ok = CreateProcessW(NULL, cmdline, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!ok)
    {
        (void)fprintf(stderr, "    CreateProcessW failed: %lu cmdline='%ls'\n",
            (unsigned long)GetLastError(), cmdline);
        return -1;
    }

    DWORD wait = WaitForSingleObject(pi.hProcess, wait_seconds * 1000u);
    int rc = -1;
    if (wait == WAIT_OBJECT_0)
    {
        DWORD exit_code = 0;
        if (GetExitCodeProcess(pi.hProcess, &exit_code))
        {
            rc = (int)exit_code;
        }
    }
    else if (wait == WAIT_TIMEOUT)
    {
        (void)fprintf(stderr, "    watchdog did not exit within %us; killing\n",
            wait_seconds);
        (void)TerminateProcess(pi.hProcess, 1);
        (void)WaitForSingleObject(pi.hProcess, 5000);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return rc;
}

static int count_dmp_files(const wchar_t* dir)
{
    wchar_t pattern[MAX_PATH * 2];
    (void)swprintf_s(pattern, sizeof(pattern) / sizeof(pattern[0]), L"%ls\\*.dmp", dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    int count = 0;
    do { count++; } while (FindNextFileW(h, &fd));
    FindClose(h);
    return count;
}

static bool find_first_dmp(const wchar_t* dir, wchar_t* out, size_t out_chars)
{
    wchar_t pattern[MAX_PATH * 2];
    (void)swprintf_s(pattern, sizeof(pattern) / sizeof(pattern[0]), L"%ls\\*.dmp", dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
    {
        out[0] = L'\0';
        return false;
    }
    (void)swprintf_s(out, out_chars, L"%ls\\%ls", dir, fd.cFileName);
    FindClose(h);
    return true;
}

static bool verify_minidump_magic(const wchar_t* path)
{
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    uint32_t magic = 0;
    DWORD read = 0;
    BOOL ok = ReadFile(h, &magic, sizeof(magic), &read, NULL);
    CloseHandle(h);
    return (ok && (read == sizeof(magic)) && (magic == MDMP_SIGNATURE));
}

// ----- test cases ------------------------------------------------------------

static int test_successful_child_returns_zero(void)
{
    wchar_t dump_dir[MAX_PATH];
    make_test_subdir(L"successful_child", dump_dir, sizeof(dump_dir) / sizeof(dump_dir[0]));

    const wchar_t* args[] = {
        L"--timeout-seconds", L"30",
        L"--dump-dir", dump_dir,
        L"--", L"cmd.exe", L"/c", L"exit 0"
    };
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 60);
    EXPECT_EQ_INT(rc, 0);
    EXPECT_EQ_INT(count_dmp_files(dump_dir), 0);
    return 0;
}

static int test_propagates_nonzero_child_exit_code(void)
{
    wchar_t dump_dir[MAX_PATH];
    make_test_subdir(L"nonzero_child", dump_dir, sizeof(dump_dir) / sizeof(dump_dir[0]));

    const wchar_t* args[] = {
        L"--timeout-seconds", L"30",
        L"--dump-dir", dump_dir,
        L"--", L"cmd.exe", L"/c", L"exit 42"
    };
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 60);
    EXPECT_EQ_INT(rc, 42);
    EXPECT_EQ_INT(count_dmp_files(dump_dir), 0);
    return 0;
}

static int test_times_out_and_captures_minidump(void)
{
    wchar_t dump_dir[MAX_PATH];
    make_test_subdir(L"timeout_dump", dump_dir, sizeof(dump_dir) / sizeof(dump_dir[0]));

    const wchar_t* args[] = {
        L"--timeout-seconds", L"2",
        L"--dump-timeout-seconds", L"60",
        L"--dump-dir", dump_dir,
        L"--", L"cmd.exe", L"/c", L"ping -n 60 127.0.0.1 > nul"
    };
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 120);
    EXPECT_EQ_INT(rc, TWD_EXIT_TIMEOUT);

    int n = count_dmp_files(dump_dir);
    if (n != 1)
    {
        TEST_FAIL_FMT("expected 1 dump file, got %d", n);
    }

    wchar_t dump_path[MAX_PATH];
    EXPECT_TRUE(find_first_dmp(dump_dir, dump_path, sizeof(dump_path) / sizeof(dump_path[0])));
    EXPECT_TRUE(verify_minidump_magic(dump_path));
    return 0;
}

static int test_env_timeout_override_takes_precedence(void)
{
    wchar_t dump_dir[MAX_PATH];
    make_test_subdir(L"env_override", dump_dir, sizeof(dump_dir) / sizeof(dump_dir[0]));

    EXPECT_TRUE(SetEnvironmentVariableW(L"TEST_WATCHDOG_TIMEOUT_SECONDS", L"2"));

    const wchar_t* args[] = {
        L"--timeout-seconds", L"60",
        L"--dump-timeout-seconds", L"60",
        L"--dump-dir", dump_dir,
        L"--", L"cmd.exe", L"/c", L"ping -n 30 127.0.0.1 > nul"
    };

    DWORD start = GetTickCount();
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 90);
    DWORD elapsed_ms = GetTickCount() - start;

    (void)SetEnvironmentVariableW(L"TEST_WATCHDOG_TIMEOUT_SECONDS", NULL);

    EXPECT_EQ_INT(rc, TWD_EXIT_TIMEOUT);
    if (elapsed_ms >= 25000)
    {
        TEST_FAIL_FMT("watchdog took %lu ms; env override appears not to have applied",
            (unsigned long)elapsed_ms);
    }
    return 0;
}

static int test_rejects_missing_dump_dir(void)
{
    const wchar_t* args[] = {
        L"--timeout-seconds", L"30",
        L"--", L"cmd.exe", L"/c", L"exit 0"
    };
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 30);
    EXPECT_EQ_INT(rc, TWD_EXIT_USAGE);
    return 0;
}

static int test_dump_tree_captures_descendant_dumps(void)
{
    wchar_t dump_dir[MAX_PATH];
    make_test_subdir(L"dump_tree", dump_dir, sizeof(dump_dir) / sizeof(dump_dir[0]));

    const wchar_t* args[] = {
        L"--timeout-seconds", L"3",
        L"--dump-timeout-seconds", L"60",
        L"--dump-tree",
        L"--dump-dir", dump_dir,
        L"--", L"cmd.exe", L"/c",
        L"start /B cmd.exe /c \"ping -n 60 127.0.0.1 > nul\" & ping -n 60 127.0.0.1 > nul"
    };
    int rc = run_watchdog((int)(sizeof(args) / sizeof(args[0])), args, 180);
    EXPECT_EQ_INT(rc, TWD_EXIT_TIMEOUT);

    int n = count_dmp_files(dump_dir);
    if (n < 2)
    {
        TEST_FAIL_FMT("expected at least 2 dumps with --dump-tree, got %d", n);
    }
    return 0;
}

// ----- main ------------------------------------------------------------------

typedef int (*test_fn)(void);
typedef struct
{
    const char* name;
    test_fn     fn;
} TEST_CASE;

static const TEST_CASE g_tests[] =
{
    { "successful_child_returns_zero",          test_successful_child_returns_zero },
    { "propagates_nonzero_child_exit_code",     test_propagates_nonzero_child_exit_code },
    { "times_out_and_captures_minidump",        test_times_out_and_captures_minidump },
    { "env_timeout_override_takes_precedence",  test_env_timeout_override_takes_precedence },
    { "rejects_missing_dump_dir",               test_rejects_missing_dump_dir },
    { "dump_tree_captures_descendant_dumps",    test_dump_tree_captures_descendant_dumps },
};

int main(int argc, char* argv[])
{
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* filter = (argc > 1) ? argv[1] : NULL;

    // Resolve TWD_EXE as <directory_of_this_exe>\test_watchdog.exe. The
    // build system places both binaries side-by-side in the test config
    // output directory, and at run time the test agent's checkout path
    // typically differs from the build agent's — so a compile-time path
    // baked via a macro would be wrong on the test agent.
    DWORD self_len = GetModuleFileNameW(NULL, TWD_EXE,
        (DWORD)(sizeof(TWD_EXE) / sizeof(TWD_EXE[0])));
    if (self_len == 0 || self_len >= sizeof(TWD_EXE) / sizeof(TWD_EXE[0]))
    {
        (void)fprintf(stderr, "GetModuleFileNameW failed: %lu\n",
            (unsigned long)GetLastError());
        return 1;
    }
    wchar_t* last_sep = TWD_EXE;
    for (wchar_t* p = TWD_EXE; *p != L'\0'; p++)
    {
        if (*p == L'\\' || *p == L'/')
        {
            last_sep = p + 1;
        }
    }
    *last_sep = L'\0';
    if (wcscat_s(TWD_EXE, sizeof(TWD_EXE) / sizeof(TWD_EXE[0]),
        L"test_watchdog.exe") != 0)
    {
        (void)fprintf(stderr, "Failed to build TWD_EXE path\n");
        return 1;
    }
    if (GetFileAttributesW(TWD_EXE) == INVALID_FILE_ATTRIBUTES)
    {
        (void)fprintf(stderr, "Watchdog not found at '%ls' (GetLastError=%lu)\n",
            TWD_EXE, (unsigned long)GetLastError());
        return 1;
    }

    wchar_t tmp[MAX_PATH];
    DWORD n = GetTempPathW((DWORD)(sizeof(tmp) / sizeof(tmp[0])), tmp);
    if (n == 0 || n >= sizeof(tmp) / sizeof(tmp[0]))
    {
        (void)fprintf(stderr, "GetTempPathW failed\n");
        return 1;
    }
    (void)swprintf_s(g_test_root, sizeof(g_test_root) / sizeof(g_test_root[0]),
        L"%lstwd_int_%lu", tmp, (unsigned long)GetCurrentProcessId());
    (void)CreateDirectoryW(g_test_root, NULL);

    const size_t test_count = sizeof(g_tests) / sizeof(g_tests[0]);
    size_t failed = 0;
    size_t ran = 0;

    (void)fprintf(stderr, " === Executing test suite test_watchdog_int (%zu tests) ===\n"
                          " === Watchdog binary: %ls ===\n",
        test_count, TWD_EXE);

    for (size_t i = 0; i < test_count; i++)
    {
        const TEST_CASE* tc = &g_tests[i];
        if (filter != NULL && filter[0] != '\0' && strcmp(filter, tc->name) != 0)
        {
            continue;
        }
        ran++;
        (void)fprintf(stderr, "Executing test %s ...\n", tc->name);
        int rc = tc->fn();
        if (rc == 0)
        {
            (void)fprintf(stderr, "  Test %s result = Succeeded.\n", tc->name);
        }
        else
        {
            failed++;
            (void)fprintf(stderr, "  Test %s result = !!! FAILED !!!\n", tc->name);
        }
    }

    (void)fprintf(stderr, "%zu tests ran, %zu failed, %zu succeeded.\n",
        ran, failed, ran - failed);

    if (ran == 0)
    {
        (void)fprintf(stderr, "no tests matched filter '%s'\n", filter ? filter : "(none)");
        return 1;
    }
    if (failed > 127) failed = 127;
    return (int)failed;
}
