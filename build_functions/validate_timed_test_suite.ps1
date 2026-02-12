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

    When the -Fix switch is provided, the script will automatically replace the vanilla
    macros with their timed equivalents and add the required include if not present.

.PARAMETER TestFolder
    The path to the test folder to validate.

.PARAMETER Fix
    If specified, automatically replace TEST_SUITE_INITIALIZE/CLEANUP with timed versions.

.EXAMPLE
    .\validate_timed_test_suite.ps1 -TestFolder "C:\repo\tests\my_module_int"

    Validates integration test files in the folder and reports files using non-timed macros.

.EXAMPLE
    .\validate_timed_test_suite.ps1 -TestFolder "C:\repo\tests\my_module_int" -Fix

    Validates and automatically replaces non-timed macros with timed versions.

.NOTES
    Returns exit code 0 if all files are valid (or were fixed), 1 if validation fails.
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$TestFolder,

    [Parameter(Mandatory=$false)]
    [switch]$Fix
)

# Set error action preference
$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Timed Test Suite Validation" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Folder: $TestFolder" -ForegroundColor White
Write-Host "Fix Mode: $($Fix.IsPresent)" -ForegroundColor White
Write-Host ""

# Initialize counters
$totalFiles = 0
$filesWithViolations = @()
$fixedFiles = @()

# Get all integration test files in the folder
Write-Host "Searching for integration test files (*_int.c)..." -ForegroundColor White
$allFiles = @(Get-ChildItem -Path $TestFolder -Recurse -Filter "*_int.c" -ErrorAction SilentlyContinue)

Write-Host "Found $($allFiles.Count) integration test files to check" -ForegroundColor White
Write-Host ""

# Pattern to match non-timed TEST_SUITE_INITIALIZE/CLEANUP
# Uses negative lookbehind to exclude lines that already have TIMED_ prefix
$initPattern = '(?<!TIMED_)TEST_SUITE_INITIALIZE\s*\('
$cleanupPattern = '(?<!TIMED_)TEST_SUITE_CLEANUP\s*\('

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

        if ($Fix) {
            try {
                $fixedLines = @()
                $hasTimedInclude = $false
                $lastIncludeIndex = -1

                # First pass: check if timed_test_suite.h is already included and find last #include
                for ($i = 0; $i -lt $lines.Count; $i++) {
                    if ($lines[$i] -match '^\s*#\s*include') {
                        $lastIncludeIndex = $i
                        if ($lines[$i] -match 'timed_test_suite\.h') {
                            $hasTimedInclude = $true
                        }
                    }
                }

                # Second pass: fix the macros and add include if needed
                for ($i = 0; $i -lt $lines.Count; $i++) {
                    $currentLine = $lines[$i]

                    # Replace TEST_SUITE_INITIALIZE(name -> TIMED_TEST_SUITE_INITIALIZE(name, TIMED_TEST_DEFAULT_TIMEOUT_MS
                    if ($currentLine -match $initPattern) {
                        $currentLine = $currentLine -replace '(?<!TIMED_)TEST_SUITE_INITIALIZE\s*\(\s*(\w+)', 'TIMED_TEST_SUITE_INITIALIZE($1, TIMED_TEST_DEFAULT_TIMEOUT_MS'
                    }

                    # Replace TEST_SUITE_CLEANUP( -> TIMED_TEST_SUITE_CLEANUP(
                    if ($currentLine -match $cleanupPattern) {
                        $currentLine = $currentLine -replace '(?<!TIMED_)TEST_SUITE_CLEANUP', 'TIMED_TEST_SUITE_CLEANUP'
                    }

                    $fixedLines += $currentLine

                    # Add the timed_test_suite.h include after the last #include line
                    if ($i -eq $lastIncludeIndex -and -not $hasTimedInclude) {
                        $fixedLines += '#include "c_pal/timed_test_suite.h"'
                        $hasTimedInclude = $true
                    }
                }

                # Write back to file
                $utf8NoBom = New-Object System.Text.UTF8Encoding $false
                $fixedContent = ($fixedLines -join "`r`n") + "`r`n"
                [System.IO.File]::WriteAllText($file.FullName, $fixedContent, $utf8NoBom)

                Write-Host "         [FIXED] Replaced non-timed macros with timed versions" -ForegroundColor Green
                $fixedFiles += $file.FullName
            }
            catch {
                Write-Host "         [ERROR] Failed to fix: $_" -ForegroundColor Red
            }
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Validation Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Total integration test files checked: $totalFiles" -ForegroundColor White

if ($Fix -and $fixedFiles.Count -gt 0) {
    Write-Host "Files fixed successfully: $($fixedFiles.Count)" -ForegroundColor Green
}

if ($filesWithViolations.Count -gt 0 -and -not $Fix) {
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
    Write-Host "To fix these files automatically, run with -Fix parameter." -ForegroundColor Cyan
    Write-Host ""
}

$unfixedFiles = $filesWithViolations.Count - $fixedFiles.Count

if ($unfixedFiles -eq 0) {
    Write-Host "[VALIDATION PASSED]" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[VALIDATION FAILED]" -ForegroundColor Red
    if ($Fix) {
        Write-Host "$unfixedFiles file(s) could not be fixed automatically." -ForegroundColor Yellow
    }
    exit 1
}
