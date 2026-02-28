@echo off
setlocal

REM Build folder + Qt location (edit if you move Qt)
set "BUILD_DIR=build-msvc"
set "QT_PREFIX=C:/Qt/6.10.2/msvc2022_64"

cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="%QT_PREFIX%"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
exit /b %errorlevel%
