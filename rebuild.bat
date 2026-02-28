@echo off
REM ---------------------------------------------------------------------------
REM rebuild.bat — One-click clean build for PDFViewer
REM
REM Sets up the MSVC x64 environment automatically, then configures and builds.
REM Works from any terminal (cmd, PowerShell, double-click). No need to open a
REM Developer Command Prompt first.
REM ---------------------------------------------------------------------------

cd /d "%~dp0"

set "BUILD_DIR=build-msvc"
set "QT_PREFIX=C:/Qt/6.10.2/msvc2022_64"
set "CONFIG=Release"

REM ---- Locate vcvars64.bat (VS 2022 Build Tools or full VS) ----
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCVARS%"=="" (
    echo ERROR: Could not find vcvars64.bat for Visual Studio 2022.
    echo Install "Desktop development with C++" via the VS Installer.
    pause
    exit /b 1
)

REM ---- Set up MSVC compiler environment ----
echo Setting up MSVC x64 environment...
call "%VCVARS%" >nul 2>&1
if errorlevel 1 (
    echo ERROR: vcvars64.bat failed.
    pause
    exit /b 1
)

echo MSVC environment ready (cl.exe: %VCToolsVersion%)
echo.

REM ---- Step 1: Clean ----
echo [1/3] Cleaning previous build...
if exist "%BUILD_DIR%" (
    cmake --build "%BUILD_DIR%" --config %CONFIG% --target clean 2>nul
    if errorlevel 1 echo   Clean target failed - will do full rebuild anyway.
) else (
    echo   No build directory yet - skipping clean.
)

REM ---- Step 2: Configure ----
echo.
echo [2/3] Configuring CMake...
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_PREFIX%"
if errorlevel 1 (
    echo.
    echo ERROR: CMake configure failed. Check the output above.
    pause
    exit /b 1
)

REM ---- Step 3: Build ----
echo.
echo [3/3] Building %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 (
    echo.
    echo ERROR: Build failed. Check the compiler errors above.
    pause
    exit /b 1
)

echo.
echo =============================================
echo   BUILD SUCCEEDED
echo   Executable: %CD%\%BUILD_DIR%\%CONFIG%\PDFViewer.exe
echo =============================================
echo.
pause
exit /b 0
