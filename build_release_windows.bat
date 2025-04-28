@echo off
:: RandomWalkSequencer Windows Build Script
:: ========================================
:: This script builds a release version of the RandomWalkSequencer plugin for Windows
:: Supports VST3 format
:: Requires CMake 3.16 or higher

echo ========================================
echo Building RandomWalkSequencer for Windows
echo ========================================

set PLUGIN_NAME=RandomWalkSequencer
set BUILD_DIR=cmake-build-release
set VST3_DIR=%USERPROFILE%\Documents\VST3

:: Check CMake version
for /f "tokens=3" %%i in ('cmake --version ^| findstr /B /C:"cmake version"') do set CMAKE_VERSION=%%i
echo Detected CMake version: %CMAKE_VERSION%

for /f "tokens=1 delims=." %%a in ("%CMAKE_VERSION%") do set CMAKE_MAJOR=%%a
for /f "tokens=2 delims=." %%a in ("%CMAKE_VERSION%") do set CMAKE_MINOR=%%a

if %CMAKE_MAJOR% LSS 3 (
    echo Error: CMake version 3.16 or higher is required.
    echo Current version: %CMAKE_VERSION%
    exit /b 1
)

if %CMAKE_MAJOR% EQU 3 (
    if %CMAKE_MINOR% LSS 16 (
        echo Error: CMake version 3.16 or higher is required.
        echo Current version: %CMAKE_VERSION%
        exit /b 1
    )
)

echo CMake version %CMAKE_VERSION% detected - OK

:: Remove existing plugin installations
echo Removing existing plugin installations...
if exist "%VST3_DIR%\%PLUGIN_NAME%.vst3" (
    rmdir /s /q "%VST3_DIR%\%PLUGIN_NAME%.vst3"
)

:: Clean build directory
echo Cleaning build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)

:: Create build directory
echo Creating build directory...
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: Find Visual Studio installation
:: Note: Adjust the VS version as needed
echo Detecting Visual Studio...
set VS_WHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`%VS_WHERE% -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  set VS_PATH=%%i
)

if not defined VS_PATH (
    echo Error: Visual Studio not found!
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%

:: Run CMake with Visual Studio generator
echo Running CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..

:: Build VST3 plugin
echo Building VST3 plugin...
cmake --build . --target %PLUGIN_NAME%_VST3 --config Release

:: Create VST3 directory if it doesn't exist
if not exist "%VST3_DIR%" (
    mkdir "%VST3_DIR%"
)

:: Copy built plugin to VST3 directory
echo Copying plugin to VST3 directory...
set VST3_BUILD_PATH=.\VST3\%PLUGIN_NAME%.vst3
if exist "%VST3_BUILD_PATH%" (
    xcopy /E /I /Y "%VST3_BUILD_PATH%" "%VST3_DIR%\%PLUGIN_NAME%.vst3\"
    echo VST3 plugin installed to %VST3_DIR%
) else (
    echo Warning: VST3 plugin not found at %VST3_BUILD_PATH%
)

:: Validate installation
echo Validating plugin installation...
if exist "%VST3_DIR%\%PLUGIN_NAME%.vst3" (
    echo ✓ VST3 plugin installed successfully
) else (
    echo ✗ VST3 plugin installation failed
)

:: Return to original directory
cd ..

echo Build completed!
echo Plugin is ready to use in your DAW
pause
