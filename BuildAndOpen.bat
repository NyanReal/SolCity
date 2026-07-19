@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "PROJECT_FILE=%PROJECT_DIR%SolCity.uproject"
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.7"
set "BUILD_SCRIPT=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "EDITOR_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT_FILE%" (
    echo [ERROR] Project file not found: "%PROJECT_FILE%"
    pause
    exit /b 1
)

if not exist "%BUILD_SCRIPT%" (
    echo [ERROR] Unreal Engine build script not found: "%BUILD_SCRIPT%"
    pause
    exit /b 1
)

if not exist "%EDITOR_EXE%" (
    echo [ERROR] Unreal Editor not found: "%EDITOR_EXE%"
    pause
    exit /b 1
)

echo [1/2] Building SolCityEditor...
call "%BUILD_SCRIPT%" SolCityEditor Win64 Development -Project="%PROJECT_FILE%" -WaitMutex -NoHotReloadFromIDE

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed. Unreal Editor was not started.
    pause
    exit /b 1
)

echo.
echo [2/2] Build succeeded. Starting Unreal Editor...
start "SolCity Unreal Editor" "%EDITOR_EXE%" "%PROJECT_FILE%"

exit /b 0
