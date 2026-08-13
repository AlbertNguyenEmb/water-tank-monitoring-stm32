@echo off
setlocal

echo ===============================================
echo Building STM32F103 Water Tank Monitor
echo ===============================================

REM ===============================================
REM Change to directory containing this .bat file
REM ===============================================
cd /d "%~dp0"

REM ===============================================
REM Clean build directory
REM ===============================================
if exist build (
    echo Cleaning build directory...
    rmdir /s /q build
)

REM ===============================================
REM Create build directory
REM ===============================================
mkdir build

REM ===============================================
REM Configure with CMake
REM ===============================================
echo.
echo [1/3] Configuring...

cmake -B build -G Ninja

if %ERRORLEVEL% neq 0 (
    echo.
    echo Configuration failed!
    pause
    exit /b 1
)

REM ===============================================
REM Build
REM ===============================================
echo.
echo [2/3] Building...

cmake --build build

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

REM ===============================================
REM Build Summary
REM ===============================================
echo.
echo [3/3] Build Summary
echo ===============================================

if exist build\*.hex (
    dir build\*.hex | find ".hex"
) else (
    echo No .hex file generated!
)

if exist build\*.bin (
    dir build\*.bin | find ".bin"
) else (
    echo No .bin file generated!
)

echo ===============================================
echo Build completed successfully!
echo.

REM ===============================================
REM Flash STM32 using ST-Link
REM ===============================================
echo Flashing STM32F103...

STM32_Programmer_CLI -c port=SWD -w build\water_tank_monitor.bin 0x08000000 -v -rst

if %ERRORLEVEL% neq 0 (
    echo.
    echo Flash failed!
    pause
    exit /b 1
)

echo.
echo ===============================================
echo FLASH COMPLETED SUCCESSFULLY!
echo ===============================================

pause
endlocal