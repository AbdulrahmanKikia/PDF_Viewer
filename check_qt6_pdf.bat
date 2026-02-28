@echo off
setlocal

REM ---------------------------------------------------------------------------
REM Check that CMake finds Qt6 Pdf and PdfWidgets. Run this after installing
REM the Qt PDF component (Qt Maintenance Tool -> Add or remove components ->
REM Qt 6.10.2 -> Additional Libraries -> Qt PDF).
REM ---------------------------------------------------------------------------

cd /d "%~dp0"

set "BUILD_DIR=build-msvc"
set "QT_PREFIX=C:/Qt/6.10.2/msvc2022_64"

echo Configuring CMake to detect Qt6 modules...
echo.
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_PREFIX%" 2>&1 | findstr /I "Qt6 Pdf PdfWidgets enabled disabled found"

echo.
echo If you see "Qt6 Pdf: found" and "Qt6 PdfWidgets: found" and "enabled",
echo Qt6 PDF is integrated. Then run rebuild.bat to build.
echo.
echo If you see "not found" or "disabled", either:
echo   1. Install Qt PDF for your kit: Qt Maintenance Tool -> Add components -> Qt PDF
echo   2. Ensure QT_PREFIX matches your Qt install (this script uses %QT_PREFIX%)
echo   3. Delete the build folder and run this script again (cmake caches module detection)
pause
