// Copyright (c) Microsoft. All rights reserved.
//
// test_watchdog: wraps a child test process with a timeout. On timeout,
// captures a full-memory minidump via a helper subprocess (the same binary
// in --dump-helper mode) before terminating the process tree.
//
// Self-contained: depends only on kernel32, dbghelp, psapi. No project libs.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <time.h>

#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#include <shellapi.h>

#define TWD_DEFAULT_TIMEOUT_SECONDS         1200u
#define TWD_DEFAULT_DUMP_TIMEOUT_SECONDS    180u
#define TWD_DEFAULT_MAX_DUMP_BYTES_GB       4u
#define TWD_DEFAULT_MAX_DUMP_BYTES          ((uint64_t)TWD_DEFAULT_MAX_DUMP_BYTES_GB * 1024ull * 1024ull * 1024ull)

#define TWD_MIN_TIMEOUT_SECONDS             1u
#define TWD_MAX_TIMEOUT_SECONDS             7200u

#define TWD_EXIT_TIMEOUT                    124
#define TWD_EXIT_CANCELLED                  130
#define TWD_EXIT_USAGE                      2
#define TWD_EXIT_SPAWN_FAILED               125

#define TWD_FULL_DUMP_FLAGS \
    (MiniDumpWithFullMemory | \
     MiniDumpWithHandleData | \
     MiniDumpWithThreadInfo | \
     MiniDumpWithUnloadedModules | \
     MiniDumpWithFullMemoryInfo)

#define TWD_TRIAGE_DUMP_FLAGS \
    (MiniDumpWithHandleData | \
     MiniDumpWithThreadInfo | \
     MiniDumpWithUnloadedModules)

typedef struct TWD_CONFIG_TAG
{
    uint32_t timeout_seconds;
    uint32_t dump_timeout_seconds;
    uint64_t max_dump_bytes;
    bool     dump_tree;
    bool     full_dump;
    wchar_t* dump_dir;        // owned
    wchar_t* child_exe;       // owned (full path)
    int      child_argc;
    wchar_t** child_argv;     // points into argv array; not owned
} TWD_CONFIG;

// Global state so the console control handler can terminate the job
// promptly on Ctrl-C / pipeline cancellation.
static HANDLE g_job_handle = NULL;
static volatile LONG g_cancelled = 0;

static void twd_log(const char* level, const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)fprintf(stderr, "##[%s] test_watchdog: ", level);
    (void)vfwprintf(stderr, fmt, args);
    (void)fprintf(stderr, "\n");
    (void)fflush(stderr);
    va_end(args);
}

#define TWD_LOG_INFO(...)    twd_log("section", __VA_ARGS__)
#define TWD_LOG_WARNING(...) twd_log("warning", __VA_ARGS__)
#define TWD_LOG_ERROR(...)   twd_log("error",   __VA_ARGS__)

static void print_usage(void)
{
    (void)fprintf(stderr,
        "test_watchdog - wrap a test process with a timeout, capturing a\n"
        "                minidump before killing on hang.\n"
        "\n"
        "Usage:\n"
        "  test_watchdog [options] -- <child_exe> [args...]\n"
        "\n"
        "Options:\n"
        "  --timeout-seconds N         Kill child after N seconds (default %u).\n"
        "  --dump-dir <path>           Where to write minidumps (required).\n"
        "  --dump-timeout-seconds M    Max seconds for dump capture (default %u).\n"
        "  --max-dump-bytes B          Skip dump if dump-dir already exceeds B bytes\n"
        "                              (default %u GiB).\n"
        "  --dump-tree                 Also dump descendant processes (default off).\n"
        "  --no-full-dump              Capture triage dump instead of full memory.\n"
        "  -h, --help                  Show this help.\n"
        "\n"
        "Environment overrides (override CLI when set and valid):\n"
        "  TEST_WATCHDOG_TIMEOUT_SECONDS\n"
        "  TEST_WATCHDOG_DUMP_DIR\n"
        "  TEST_WATCHDOG_FULL_DUMP=0   - request triage dump\n"
        "\n"
        "Internal helper mode (do not invoke directly):\n"
        "  test_watchdog --dump-helper <pid> <dump_path> <flags_hex>\n"
        "\n"
        "Exit codes:\n"
        "  <child code>  on normal child completion\n"
        "  %d            on watchdog timeout (dump attempted, tree killed)\n"
        "  %d            on Ctrl-C / cancellation\n"
        "  %d            on usage error\n"
        "  %d            on spawn failure\n",
        TWD_DEFAULT_TIMEOUT_SECONDS,
        TWD_DEFAULT_DUMP_TIMEOUT_SECONDS,
        TWD_DEFAULT_MAX_DUMP_BYTES_GB,
        TWD_EXIT_TIMEOUT, TWD_EXIT_CANCELLED,
        TWD_EXIT_USAGE, TWD_EXIT_SPAWN_FAILED);
    (void)fflush(stderr);
}

static bool parse_uint32(const wchar_t* s, uint32_t* out)
{
    if ((s == NULL) || (s[0] == L'\0'))
    {
        return false;
    }
    wchar_t* end = NULL;
    unsigned long v = wcstoul(s, &end, 10);
    if ((end == NULL) || (*end != L'\0') || (v > 0xFFFFFFFFu))
    {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static bool parse_uint64(const wchar_t* s, uint64_t* out)
{
    if ((s == NULL) || (s[0] == L'\0'))
    {
        return false;
    }
    wchar_t* end = NULL;
    unsigned long long v = _wcstoui64(s, &end, 10);
    if ((end == NULL) || (*end != L'\0'))
    {
        return false;
    }
    *out = (uint64_t)v;
    return true;
}

static wchar_t* dup_wstr(const wchar_t* s)
{
    if (s == NULL)
    {
        return NULL;
    }
    size_t n = wcslen(s) + 1;
    wchar_t* r = (wchar_t*)malloc(n * sizeof(wchar_t));
    if (r != NULL)
    {
        memcpy(r, s, n * sizeof(wchar_t));
    }
    return r;
}

static bool twd_parse_args(int argc, wchar_t** argv, TWD_CONFIG* cfg)
{
    cfg->timeout_seconds = TWD_DEFAULT_TIMEOUT_SECONDS;
    cfg->dump_timeout_seconds = TWD_DEFAULT_DUMP_TIMEOUT_SECONDS;
    cfg->max_dump_bytes = TWD_DEFAULT_MAX_DUMP_BYTES;
    cfg->dump_tree = false;
    cfg->full_dump = true;
    cfg->dump_dir = NULL;
    cfg->child_exe = NULL;
    cfg->child_argc = 0;
    cfg->child_argv = NULL;

    int i = 1;
    while (i < argc)
    {
        const wchar_t* a = argv[i];
        if ((wcscmp(a, L"--") == 0))
        {
            i++;
            break;
        }
        else if ((wcscmp(a, L"-h") == 0) || (wcscmp(a, L"--help") == 0))
        {
            print_usage();
            return false;
        }
        else if (wcscmp(a, L"--timeout-seconds") == 0)
        {
            if (i + 1 >= argc) { TWD_LOG_ERROR(L"--timeout-seconds requires a value"); return false; }
            if (!parse_uint32(argv[i + 1], &cfg->timeout_seconds)) { TWD_LOG_ERROR(L"invalid --timeout-seconds value: %ls", argv[i + 1]); return false; }
            i += 2;
        }
        else if (wcscmp(a, L"--dump-timeout-seconds") == 0)
        {
            if (i + 1 >= argc) { TWD_LOG_ERROR(L"--dump-timeout-seconds requires a value"); return false; }
            if (!parse_uint32(argv[i + 1], &cfg->dump_timeout_seconds)) { TWD_LOG_ERROR(L"invalid --dump-timeout-seconds value: %ls", argv[i + 1]); return false; }
            i += 2;
        }
        else if (wcscmp(a, L"--max-dump-bytes") == 0)
        {
            if (i + 1 >= argc) { TWD_LOG_ERROR(L"--max-dump-bytes requires a value"); return false; }
            if (!parse_uint64(argv[i + 1], &cfg->max_dump_bytes)) { TWD_LOG_ERROR(L"invalid --max-dump-bytes value: %ls", argv[i + 1]); return false; }
            i += 2;
        }
        else if (wcscmp(a, L"--dump-dir") == 0)
        {
            if (i + 1 >= argc) { TWD_LOG_ERROR(L"--dump-dir requires a value"); return false; }
            free(cfg->dump_dir);
            cfg->dump_dir = dup_wstr(argv[i + 1]);
            i += 2;
        }
        else if (wcscmp(a, L"--dump-tree") == 0)
        {
            cfg->dump_tree = true;
            i++;
        }
        else if (wcscmp(a, L"--no-full-dump") == 0)
        {
            cfg->full_dump = false;
            i++;
        }
        else
        {
            TWD_LOG_ERROR(L"unknown option: %ls (did you forget '--' before child exe?)", a);
            return false;
        }
    }

    if (i >= argc)
    {
        TWD_LOG_ERROR(L"missing child executable after '--'");
        return false;
    }

    cfg->child_exe = dup_wstr(argv[i]);
    cfg->child_argc = argc - i;
    cfg->child_argv = &argv[i];
    return true;
}

static void twd_apply_env_overrides(TWD_CONFIG* cfg)
{
    wchar_t buf[1024];
    DWORD len;

    // TEST_WATCHDOG_TIMEOUT_SECONDS
    len = GetEnvironmentVariableW(L"TEST_WATCHDOG_TIMEOUT_SECONDS", buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if ((len > 0) && (len < (sizeof(buf) / sizeof(buf[0]))))
    {
        uint32_t v = 0;
        if (parse_uint32(buf, &v) && (v >= TWD_MIN_TIMEOUT_SECONDS) && (v <= TWD_MAX_TIMEOUT_SECONDS))
        {
            TWD_LOG_INFO(L"env override: timeout-seconds=%u (was %u)", v, cfg->timeout_seconds);
            cfg->timeout_seconds = v;
        }
        else
        {
            TWD_LOG_WARNING(L"TEST_WATCHDOG_TIMEOUT_SECONDS='%ls' rejected (must be %u..%u)",
                buf, TWD_MIN_TIMEOUT_SECONDS, TWD_MAX_TIMEOUT_SECONDS);
        }
    }

    // TEST_WATCHDOG_DUMP_DIR
    len = GetEnvironmentVariableW(L"TEST_WATCHDOG_DUMP_DIR", buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if ((len > 0) && (len < (sizeof(buf) / sizeof(buf[0]))))
    {
        TWD_LOG_INFO(L"env override: dump-dir=%ls", buf);
        free(cfg->dump_dir);
        cfg->dump_dir = dup_wstr(buf);
    }

    // TEST_WATCHDOG_FULL_DUMP
    len = GetEnvironmentVariableW(L"TEST_WATCHDOG_FULL_DUMP", buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if ((len > 0) && (len < (sizeof(buf) / sizeof(buf[0]))))
    {
        if (wcscmp(buf, L"0") == 0)
        {
            TWD_LOG_INFO(L"env override: full-dump disabled (triage dump only)");
            cfg->full_dump = false;
        }
    }
}

// Returns the base file name (no path, no extension) for log/dump naming.
static void compute_basename_no_ext(const wchar_t* path, wchar_t* out, size_t out_chars)
{
    const wchar_t* slash = path;
    for (const wchar_t* p = path; *p != L'\0'; p++)
    {
        if ((*p == L'\\') || (*p == L'/'))
        {
            slash = p + 1;
        }
    }
    (void)wcsncpy_s(out, out_chars, slash, _TRUNCATE);
    wchar_t* dot = wcsrchr(out, L'.');
    if (dot != NULL)
    {
        *dot = L'\0';
    }
}

static void compute_dump_path(const wchar_t* dump_dir, const wchar_t* child_exe, DWORD pid, wchar_t* out, size_t out_chars)
{
    wchar_t base[260];
    compute_basename_no_ext(child_exe, base, sizeof(base) / sizeof(base[0]));

    SYSTEMTIME st;
    GetLocalTime(&st);
    (void)swprintf_s(out, out_chars, L"%ls\\%ls_%lu_%04u%02u%02u_%02u%02u%02u.dmp",
        dump_dir, base, (unsigned long)pid,
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
}

static bool ensure_directory(const wchar_t* path)
{
    DWORD attrs = GetFileAttributesW(path);
    if ((attrs != INVALID_FILE_ATTRIBUTES) && ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0))
    {
        return true;
    }
    if (CreateDirectoryW(path, NULL))
    {
        return true;
    }
    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
    {
        return true;
    }
    TWD_LOG_ERROR(L"failed to create dump directory '%ls' (GetLastError=%lu)", path, (unsigned long)err);
    return false;
}

static uint64_t get_dir_size_bytes(const wchar_t* dir)
{
    wchar_t pattern[MAX_PATH * 2];
    (void)swprintf_s(pattern, sizeof(pattern) / sizeof(pattern[0]), L"%ls\\*", dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    uint64_t total = 0;
    do
    {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            uint64_t sz = ((uint64_t)fd.nFileSizeHigh << 32) | (uint64_t)fd.nFileSizeLow;
            total += sz;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return total;
}

// Build a Win32 command line from argv. Each argument is quoted per Win32
// CommandLineToArgvW rules. The caller takes ownership of the returned
// allocation (free with free()).
static wchar_t* build_command_line(int argc, wchar_t** argv)
{
    size_t total = 0;
    for (int i = 0; i < argc; i++)
    {
        // Worst case: every char becomes "\\\\\"" + 2 quotes + 1 space => 5x + 3
        total += wcslen(argv[i]) * 5 + 3;
    }
    total += 1; // null terminator
    wchar_t* buf = (wchar_t*)malloc(total * sizeof(wchar_t));
    if (buf == NULL)
    {
        return NULL;
    }
    size_t pos = 0;
    for (int i = 0; i < argc; i++)
    {
        if (i > 0)
        {
            buf[pos++] = L' ';
        }
        const wchar_t* a = argv[i];
        bool needs_quote = (a[0] == L'\0') || (wcspbrk(a, L" \t\v\n\"") != NULL);
        if (!needs_quote)
        {
            size_t n = wcslen(a);
            memcpy(buf + pos, a, n * sizeof(wchar_t));
            pos += n;
        }
        else
        {
            buf[pos++] = L'"';
            size_t backslashes = 0;
            for (const wchar_t* p = a; *p != L'\0'; p++)
            {
                if (*p == L'\\')
                {
                    backslashes++;
                }
                else if (*p == L'"')
                {
                    // Double the run of backslashes, then escape the quote.
                    for (size_t k = 0; k < backslashes; k++) { buf[pos++] = L'\\'; }
                    buf[pos++] = L'\\';
                    buf[pos++] = L'"';
                    backslashes = 0;
                }
                else
                {
                    for (size_t k = 0; k < backslashes; k++) { buf[pos++] = L'\\'; }
                    buf[pos++] = *p;
                    backslashes = 0;
                }
            }
            // Double trailing backslashes before the closing quote.
            for (size_t k = 0; k < backslashes * 2; k++) { buf[pos++] = L'\\'; }
            buf[pos++] = L'"';
        }
    }
    buf[pos] = L'\0';
    return buf;
}

// Returns TRUE on Ctrl-C handling success (so the system doesn't run the
// next handler). Sets g_cancelled and terminates the job to kill the child
// tree.
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        InterlockedExchange(&g_cancelled, 1);
        if (g_job_handle != NULL)
        {
            TWD_LOG_WARNING(L"received console control event %lu; terminating job",
                (unsigned long)ctrl_type);
            (void)TerminateJobObject(g_job_handle, (UINT)TWD_EXIT_CANCELLED);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

// --dump-helper <pid> <dump_path> <flags_hex>
// Runs MiniDumpWriteDump on the target pid into dump_path. Exits 0 on
// success, nonzero on failure. Designed to be killable: if dump itself
// hangs, the parent watchdog can TerminateProcess this helper.
static int dump_helper_mode(int argc, wchar_t** argv)
{
    if (argc < 5)
    {
        TWD_LOG_ERROR(L"--dump-helper requires <pid> <path> <flags_hex>");
        return TWD_EXIT_USAGE;
    }
    uint32_t pid32 = 0;
    if (!parse_uint32(argv[2], &pid32))
    {
        TWD_LOG_ERROR(L"--dump-helper: bad pid '%ls'", argv[2]);
        return TWD_EXIT_USAGE;
    }
    const wchar_t* path = argv[3];
    wchar_t* end = NULL;
    unsigned long flags = wcstoul(argv[4], &end, 16);
    if ((end == NULL) || (*end != L'\0'))
    {
        TWD_LOG_ERROR(L"--dump-helper: bad flags '%ls'", argv[4]);
        return TWD_EXIT_USAGE;
    }

    HANDLE proc = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
        FALSE,
        (DWORD)pid32);
    if (proc == NULL)
    {
        TWD_LOG_ERROR(L"--dump-helper: OpenProcess(%u) failed (GetLastError=%lu)",
            (unsigned)pid32, (unsigned long)GetLastError());
        return 1;
    }

    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        TWD_LOG_ERROR(L"--dump-helper: CreateFileW('%ls') failed (GetLastError=%lu)",
            path, (unsigned long)GetLastError());
        CloseHandle(proc);
        return 1;
    }

    BOOL ok = MiniDumpWriteDump(proc, (DWORD)pid32, file,
        (MINIDUMP_TYPE)flags, NULL, NULL, NULL);
    DWORD dump_err = ok ? 0 : GetLastError();
    CloseHandle(file);
    CloseHandle(proc);

    if (!ok)
    {
        TWD_LOG_ERROR(L"--dump-helper: MiniDumpWriteDump failed (GetLastError=0x%08lx)",
            (unsigned long)dump_err);
        (void)DeleteFileW(path);
        return 1;
    }

    LARGE_INTEGER fsize;
    fsize.QuadPart = 0;
    HANDLE fh = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh != INVALID_HANDLE_VALUE)
    {
        (void)GetFileSizeEx(fh, &fsize);
        CloseHandle(fh);
    }
    TWD_LOG_INFO(L"--dump-helper: wrote dump '%ls' (%lld bytes)", path,
        (long long)fsize.QuadPart);
    return 0;
}

// Spawn the helper for one pid; wait up to dump_timeout_seconds. Returns
// true on success (dump file should now exist). On helper timeout,
// terminates the helper and returns false.
static bool capture_dump_for_pid(const wchar_t* self_exe, DWORD pid,
    const wchar_t* dump_path, MINIDUMP_TYPE flags, uint32_t dump_timeout_seconds)
{
    wchar_t flags_hex[20];
    (void)swprintf_s(flags_hex, sizeof(flags_hex) / sizeof(flags_hex[0]),
        L"%lx", (unsigned long)flags);

    wchar_t pid_buf[16];
    (void)swprintf_s(pid_buf, sizeof(pid_buf) / sizeof(pid_buf[0]),
        L"%lu", (unsigned long)pid);

    wchar_t* helper_argv[5];
    helper_argv[0] = (wchar_t*)self_exe;
    helper_argv[1] = L"--dump-helper";
    helper_argv[2] = pid_buf;
    helper_argv[3] = (wchar_t*)dump_path;
    helper_argv[4] = flags_hex;

    wchar_t* cmdline = build_command_line(5, helper_argv);
    if (cmdline == NULL)
    {
        TWD_LOG_ERROR(L"out of memory building helper command line");
        return false;
    }

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL ok = CreateProcessW(
        self_exe, cmdline, NULL, NULL, TRUE,
        0, NULL, NULL, &si, &pi);
    free(cmdline);

    if (!ok)
    {
        TWD_LOG_ERROR(L"failed to spawn dump helper for pid %lu (GetLastError=%lu)",
            (unsigned long)pid, (unsigned long)GetLastError());
        return false;
    }

    DWORD wait = WaitForSingleObject(pi.hProcess, dump_timeout_seconds * 1000u);
    bool success = false;
    if (wait == WAIT_OBJECT_0)
    {
        DWORD exit_code = 1;
        (void)GetExitCodeProcess(pi.hProcess, &exit_code);
        success = (exit_code == 0);
    }
    else if (wait == WAIT_TIMEOUT)
    {
        TWD_LOG_ERROR(L"dump helper for pid %lu exceeded %u seconds; terminating it",
            (unsigned long)pid, dump_timeout_seconds);
        (void)TerminateProcess(pi.hProcess, 1);
        (void)WaitForSingleObject(pi.hProcess, 5000);
    }
    else
    {
        TWD_LOG_ERROR(L"WaitForSingleObject on dump helper returned %lu (GetLastError=%lu)",
            (unsigned long)wait, (unsigned long)GetLastError());
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return success;
}

// Returns a malloc'd array of pids belonging to the job. *out_count gets the
// number of pids. Caller must free *out_pids.
static bool enumerate_job_pids(HANDLE job, DWORD** out_pids, size_t* out_count)
{
    *out_pids = NULL;
    *out_count = 0;

    DWORD capacity = 32;
    for (;;)
    {
        size_t bytes = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (size_t)(capacity - 1) * sizeof(ULONG_PTR);
        JOBOBJECT_BASIC_PROCESS_ID_LIST* info = (JOBOBJECT_BASIC_PROCESS_ID_LIST*)malloc(bytes);
        if (info == NULL)
        {
            return false;
        }
        memset(info, 0, bytes);
        DWORD ret = 0;
        BOOL ok = QueryInformationJobObject(job, JobObjectBasicProcessIdList,
            info, (DWORD)bytes, &ret);
        if (!ok)
        {
            DWORD err = GetLastError();
            free(info);
            if (err == ERROR_MORE_DATA)
            {
                capacity *= 2;
                continue;
            }
            TWD_LOG_WARNING(L"QueryInformationJobObject failed (GetLastError=%lu)",
                (unsigned long)err);
            return false;
        }
        DWORD count = info->NumberOfProcessIdsInList;
        DWORD* pids = NULL;
        if (count > 0)
        {
            pids = (DWORD*)malloc(count * sizeof(DWORD));
            if (pids == NULL)
            {
                free(info);
                return false;
            }
            for (DWORD i = 0; i < count; i++)
            {
                pids[i] = (DWORD)info->ProcessIdList[i];
            }
        }
        free(info);
        *out_pids = pids;
        *out_count = count;
        return true;
    }
}

static int run_watchdog(TWD_CONFIG* cfg, const wchar_t* self_exe)
{
    if (cfg->dump_dir == NULL)
    {
        TWD_LOG_ERROR(L"--dump-dir is required");
        return TWD_EXIT_USAGE;
    }
    if ((cfg->timeout_seconds < TWD_MIN_TIMEOUT_SECONDS) ||
        (cfg->timeout_seconds > TWD_MAX_TIMEOUT_SECONDS))
    {
        TWD_LOG_WARNING(L"timeout %u out of range (%u..%u); clamping to default %u",
            cfg->timeout_seconds, TWD_MIN_TIMEOUT_SECONDS, TWD_MAX_TIMEOUT_SECONDS,
            TWD_DEFAULT_TIMEOUT_SECONDS);
        cfg->timeout_seconds = TWD_DEFAULT_TIMEOUT_SECONDS;
    }
    if (!ensure_directory(cfg->dump_dir))
    {
        return TWD_EXIT_SPAWN_FAILED;
    }

    TWD_LOG_INFO(L"effective config: child='%ls' timeout=%us dump_timeout=%us dump_dir='%ls' full_dump=%s dump_tree=%s",
        cfg->child_exe, cfg->timeout_seconds, cfg->dump_timeout_seconds,
        cfg->dump_dir,
        cfg->full_dump ? L"yes" : L"no",
        cfg->dump_tree ? L"yes" : L"no");

    // Create the job object. Set kill-on-close so an accidental wrapper
    // crash kills the child tree. Set breakaway-OK so we can be safely
    // nested inside ctest's / agent's job if any.
    HANDLE job = CreateJobObjectW(NULL, NULL);
    if (job == NULL)
    {
        TWD_LOG_ERROR(L"CreateJobObject failed (GetLastError=%lu)",
            (unsigned long)GetLastError());
        return TWD_EXIT_SPAWN_FAILED;
    }
    g_job_handle = job;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
    memset(&jeli, 0, sizeof(jeli));
    jeli.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
        JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
        &jeli, sizeof(jeli)))
    {
        TWD_LOG_WARNING(L"SetInformationJobObject failed (GetLastError=%lu); continuing",
            (unsigned long)GetLastError());
    }

    if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE))
    {
        TWD_LOG_WARNING(L"SetConsoleCtrlHandler failed (GetLastError=%lu); continuing",
            (unsigned long)GetLastError());
    }

    // Build the child command line (argv[0] = child exe path).
    wchar_t* cmdline = build_command_line(cfg->child_argc, cfg->child_argv);
    if (cmdline == NULL)
    {
        TWD_LOG_ERROR(L"out of memory building child command line");
        CloseHandle(job);
        return TWD_EXIT_SPAWN_FAILED;
    }

    STARTUPINFOW si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    // CREATE_SUSPENDED so we can assign to the job *before* the child runs
    // and can spawn any descendants. Inherit handles for stdio forwarding
    // (ctest -V / --output-on-failure depends on this).
    //
    // Pass NULL for lpApplicationName so Windows applies its standard
    // command-line resolution rules (PATH search, .exe extension, etc.).
    // ctest passes the absolute path via $<TARGET_FILE:...> so this is a
    // no-op in CI but makes local invocations like 'cmd.exe' work too.
    BOOL ok = CreateProcessW(
        NULL, cmdline, NULL, NULL, TRUE,
        CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
        NULL, NULL, &si, &pi);
    free(cmdline);

    if (!ok)
    {
        DWORD err = GetLastError();
        TWD_LOG_ERROR(L"CreateProcessW('%ls') failed (GetLastError=%lu)",
            cfg->child_exe, (unsigned long)err);
        CloseHandle(job);
        return TWD_EXIT_SPAWN_FAILED;
    }

    bool in_job = false;
    if (AssignProcessToJobObject(job, pi.hProcess))
    {
        in_job = true;
    }
    else
    {
        TWD_LOG_WARNING(L"AssignProcessToJobObject failed (GetLastError=%lu); falling back to immediate-child only (no tree kill)",
            (unsigned long)GetLastError());
    }

    if (ResumeThread(pi.hThread) == (DWORD)-1)
    {
        TWD_LOG_ERROR(L"ResumeThread failed (GetLastError=%lu); terminating",
            (unsigned long)GetLastError());
        (void)TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(job);
        return TWD_EXIT_SPAWN_FAILED;
    }

    TWD_LOG_INFO(L"spawned child pid=%lu in_job=%s",
        (unsigned long)pi.dwProcessId, in_job ? L"yes" : L"no");

    DWORD wait = WaitForSingleObject(pi.hProcess, cfg->timeout_seconds * 1000u);
    int rc = 0;
    if (wait == WAIT_OBJECT_0)
    {
        DWORD child_exit = 0;
        (void)GetExitCodeProcess(pi.hProcess, &child_exit);
        rc = (int)child_exit;
        TWD_LOG_INFO(L"child exited normally with code %d", rc);
    }
    else if (wait == WAIT_TIMEOUT)
    {
        TWD_LOG_ERROR(L"TIMEOUT after %us for '%ls' (pid=%lu)",
            cfg->timeout_seconds, cfg->child_exe, (unsigned long)pi.dwProcessId);

        MINIDUMP_TYPE flags = cfg->full_dump ? TWD_FULL_DUMP_FLAGS : TWD_TRIAGE_DUMP_FLAGS;

        // Budget check before any dump.
        uint64_t dir_size = get_dir_size_bytes(cfg->dump_dir);
        if (dir_size >= cfg->max_dump_bytes)
        {
            TWD_LOG_WARNING(L"dump directory already at %llu bytes (budget %llu); skipping dump",
                (unsigned long long)dir_size, (unsigned long long)cfg->max_dump_bytes);
        }
        else
        {
            wchar_t dump_path[MAX_PATH * 2];
            compute_dump_path(cfg->dump_dir, cfg->child_exe, pi.dwProcessId,
                dump_path, sizeof(dump_path) / sizeof(dump_path[0]));
            TWD_LOG_ERROR(L"capturing dump to '%ls' (flags=0x%lx, helper timeout=%us)",
                dump_path, (unsigned long)flags, cfg->dump_timeout_seconds);
            bool dumped = capture_dump_for_pid(self_exe, pi.dwProcessId, dump_path,
                flags, cfg->dump_timeout_seconds);
            if (!dumped)
            {
                TWD_LOG_ERROR(L"failed to capture dump for main child pid %lu",
                    (unsigned long)pi.dwProcessId);
            }

            // Optionally dump descendants.
            if (cfg->dump_tree && in_job)
            {
                DWORD* pids = NULL;
                size_t pid_count = 0;
                if (enumerate_job_pids(job, &pids, &pid_count))
                {
                    for (size_t k = 0; k < pid_count; k++)
                    {
                        if (pids[k] == pi.dwProcessId)
                        {
                            continue;
                        }
                        if (get_dir_size_bytes(cfg->dump_dir) >= cfg->max_dump_bytes)
                        {
                            TWD_LOG_WARNING(L"budget reached during tree dump; stopping after %zu of %zu descendants",
                                k, pid_count);
                            break;
                        }
                        wchar_t child_dump_path[MAX_PATH * 2];
                        (void)swprintf_s(child_dump_path, sizeof(child_dump_path) / sizeof(child_dump_path[0]),
                            L"%ls\\descendant_%lu_of_%lu.dmp",
                            cfg->dump_dir, (unsigned long)pids[k], (unsigned long)pi.dwProcessId);
                        TWD_LOG_ERROR(L"capturing descendant dump pid=%lu to '%ls'",
                            (unsigned long)pids[k], child_dump_path);
                        (void)capture_dump_for_pid(self_exe, pids[k], child_dump_path,
                            flags, cfg->dump_timeout_seconds);
                    }
                    free(pids);
                }
            }
        }

        // Tree kill via job (or direct if not in job).
        if (in_job)
        {
            TWD_LOG_ERROR(L"terminating job (process tree)");
            (void)TerminateJobObject(job, (UINT)TWD_EXIT_TIMEOUT);
        }
        else
        {
            TWD_LOG_ERROR(L"terminating child only (not in job)");
            (void)TerminateProcess(pi.hProcess, (UINT)TWD_EXIT_TIMEOUT);
        }
        (void)WaitForSingleObject(pi.hProcess, 10000);
        rc = TWD_EXIT_TIMEOUT;
    }
    else
    {
        DWORD err = GetLastError();
        TWD_LOG_ERROR(L"WaitForSingleObject returned %lu (GetLastError=%lu); terminating child",
            (unsigned long)wait, (unsigned long)err);
        (void)TerminateJobObject(job, TWD_EXIT_SPAWN_FAILED);
        rc = TWD_EXIT_SPAWN_FAILED;
    }

    if (InterlockedCompareExchange(&g_cancelled, 0, 0) != 0)
    {
        TWD_LOG_WARNING(L"watchdog was cancelled mid-run; reporting cancellation exit code");
        rc = TWD_EXIT_CANCELLED;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    g_job_handle = NULL;
    CloseHandle(job);
    return rc;
}

int wmain(int argc, wchar_t** argv)
{
    // Unbuffer stderr so banners survive a hard kill.
    setvbuf(stderr, NULL, _IONBF, 0);

    if ((argc >= 2) && (wcscmp(argv[1], L"--dump-helper") == 0))
    {
        return dump_helper_mode(argc, argv);
    }

    wchar_t self_exe[MAX_PATH * 2];
    DWORD self_len = GetModuleFileNameW(NULL, self_exe,
        (DWORD)(sizeof(self_exe) / sizeof(self_exe[0])));
    if ((self_len == 0) || (self_len >= sizeof(self_exe) / sizeof(self_exe[0])))
    {
        TWD_LOG_ERROR(L"GetModuleFileNameW failed (GetLastError=%lu)",
            (unsigned long)GetLastError());
        return TWD_EXIT_SPAWN_FAILED;
    }

    TWD_CONFIG cfg;
    if (!twd_parse_args(argc, argv, &cfg))
    {
        free(cfg.dump_dir);
        free(cfg.child_exe);
        return TWD_EXIT_USAGE;
    }
    twd_apply_env_overrides(&cfg);

    int rc = run_watchdog(&cfg, self_exe);

    free(cfg.dump_dir);
    free(cfg.child_exe);
    return rc;
}
