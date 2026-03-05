# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

<#
.SYNOPSIS
    Validates that integration test files use TIMED_TEST_SUITE_INITIALIZE/CLEANUP macros.

.DESCRIPTION
    This script checks integration test files (*_int.c) in a given test folder to ensure
    they use the timed versions of TEST_SUITE_INITIALIZE and TEST_SUITE_CLEANUP macros.
    The timed macros (TIMED_TEST_SUITE_INITIALIZE / TIMED_TEST_SUITE_CLEANUP) wrap the
    vanilla versions with a process watchdog that crashes the process and produces a dump
    on timeout, allowing dumps to be collected if the test hangs.

.PARAMETER TestFolder
    The path to the test folder to validate.

.PARAMETER NumExpectedViolations
    The number of files expected to have violations. When the actual violation count matches
    this value, the script exits with code 0. Default is 0 (no violations expected).

.EXAMPLE
    .\validate_timed_test_suite.ps1 -TestFolder "C:\repo\tests\my_module_int"

    Validates integration test files in the folder and reports files using non-timed macros.

.NOTES
    Returns exit code 0 if violation count matches NumExpectedViolations, 1 otherwise.
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$TestFolder,

    [Parameter(Mandatory=$false)]
    [int]$NumExpectedViolations = 0
)

# Set error action preference
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Timed Test Suite Validation" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Folder: $TestFolder" -ForegroundColor White
if ($NumExpectedViolations -gt 0) {
    Write-Host "Expected Violations: $NumExpectedViolations" -ForegroundColor White
}
Write-Host ""

# Get all integration test files in the folder
Write-Host "Searching for integration test files (*_int.c)..." -ForegroundColor White
$allFiles = @(Get-ChildItem -Path $TestFolder -Recurse -Filter "*_int.c" -ErrorAction SilentlyContinue)

Write-Host "Found $($allFiles.Count) integration test files to check" -ForegroundColor White
Write-Host ""

# Pattern to match non-timed TEST_SUITE_INITIALIZE/CLEANUP
# Uses negative lookbehind to exclude lines that already have TIMED_ prefix
$initPattern = '(?<!TIMED_)TEST_SUITE_INITIALIZE\s*\('
$cleanupPattern = '(?<!TIMED_)TEST_SUITE_CLEANUP\s*\('

$totalFiles = 0
$filesWithViolations = @()

foreach ($file in $allFiles) {
    $totalFiles++

    # Read file content as lines
    try {
        $lines = Get-Content -Path $file.FullName -ErrorAction Stop
    }
    catch {
        Write-Host "  [WARN] Cannot read file: $($file.FullName)" -ForegroundColor Yellow
        continue
    }

    $matchingLines = @()
    $lineNumber = 0
    foreach ($line in $lines) {
        $lineNumber++
        if ($line -match $initPattern -or $line -match $cleanupPattern) {
            $matchingLines += [PSCustomObject]@{
                LineNumber = $lineNumber
                Content = $line.Trim()
            }
        }
    }

    if ($matchingLines.Count -gt 0) {
        Write-Host "  [FAIL] $($file.FullName)" -ForegroundColor Red
        Write-Host "         Contains $($matchingLines.Count) non-timed TEST_SUITE macro(s)" -ForegroundColor Yellow

        foreach ($match in $matchingLines) {
            Write-Host "         Line $($match.LineNumber): $($match.Content)" -ForegroundColor Yellow
        }

        $filesWithViolations += [PSCustomObject]@{
            FilePath = $file.FullName
            MatchCount = $matchingLines.Count
            Matches = $matchingLines
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Validation Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Total integration test files checked: $totalFiles" -ForegroundColor White

if ($filesWithViolations.Count -gt 0) {
    Write-Host "Files with non-timed macros: $($filesWithViolations.Count)" -ForegroundColor Red
    Write-Host ""
    Write-Host "The following files use TEST_SUITE_INITIALIZE/CLEANUP instead of timed versions:" -ForegroundColor Yellow

    foreach ($item in $filesWithViolations) {
        Write-Host "  - $($item.FilePath) ($($item.MatchCount) macro(s))" -ForegroundColor White
    }

    Write-Host ""
    Write-Host "Integration tests should use TIMED_TEST_SUITE_INITIALIZE and" -ForegroundColor Cyan
    Write-Host "TIMED_TEST_SUITE_CLEANUP from c_pal/timed_test_suite.h to allow" -ForegroundColor Cyan
    Write-Host "dumps to be collected if the test hangs." -ForegroundColor Cyan
    Write-Host ""
}

if ($filesWithViolations.Count -eq $NumExpectedViolations) {
    Write-Host "[VALIDATION PASSED]" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[VALIDATION FAILED] Expected $NumExpectedViolations violation(s), found $($filesWithViolations.Count)" -ForegroundColor Red
    exit 1
}
