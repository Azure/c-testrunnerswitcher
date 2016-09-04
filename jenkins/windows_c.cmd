@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rmdir /s /q %build-root%\cmake
mkdir %build-root%\cmake
if errorlevel 1 goto :eof

cd %build-root%\cmake
cmake ..

msbuild /m testrunnerswitcher.sln /p:Configuration=Release
if errorlevel 1 goto :eof
msbuild /m testrunnerswitcher.sln /p:Configuration=Debug
if errorlevel 1 goto :eof

ctest -C "debug" -V
if errorlevel 1 goto :eof

cd %build-root%