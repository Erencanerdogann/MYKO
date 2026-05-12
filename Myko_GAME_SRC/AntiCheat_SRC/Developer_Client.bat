@echo off
chcp 65001 >nul
title Pearl Guard Build (Release)
color 0A

echo ============================================
echo   PEARL GUARD — RELEASE BUILD
echo ============================================
echo.

set "STDAFX=%~dp02AntiCheat\Pearl Guard\stdafx.h"
set "SLN=%~dp02AntiCheat\CodeGuardAnticheat.sln"
set "OUTPUT=%~dp02AntiCheat\Release\code.guard"
set "DEST=C:\temp\MYKO\NEW_CLIENT\code.guard"

:: --- 1) BAKIM_MODU = 0 yap ---
echo [1/5] BAKIM_MODU = 0 yapiliyor...
powershell -Command "(Get-Content '%STDAFX%') -replace '#define BAKIM_MODU 1', '#define BAKIM_MODU 0' | Set-Content '%STDAFX%'"
findstr "BAKIM_MODU" "%STDAFX%" | findstr /C:"#define"
echo.

:: --- 2) Build ---
echo [2/5] Build basliyor (Release x86)...
echo.

:: VS2022 ortamini yukle
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

MSBuild "%SLN%" -p:Configuration=Release -p:Platform=x86 -t:Build -v:minimal
if %ERRORLEVEL% NEQ 0 (
    color 0C
    echo.
    echo [HATA] Build BASARISIZ! BAKIM_MODU geri 1 yapiliyor...
    powershell -Command "(Get-Content '%STDAFX%') -replace '#define BAKIM_MODU 0', '#define BAKIM_MODU 1' | Set-Content '%STDAFX%'"
    pause
    exit /b 1
)

echo.
echo [3/5] Build BASARILI!
echo.

:: --- 3) BAKIM_MODU = 1 geri yap (developer icin) ---
echo [4/5] BAKIM_MODU = 1 geri yapiliyor (developer)...
powershell -Command "(Get-Content '%STDAFX%') -replace '#define BAKIM_MODU 0', '#define BAKIM_MODU 1' | Set-Content '%STDAFX%'"
findstr "BAKIM_MODU" "%STDAFX%" | findstr /C:"#define"
echo.

:: --- 4) NEW_CLIENT'a kopyala ---
echo [5/5] code.guard → NEW_CLIENT kopyalaniyor...
copy /Y "%OUTPUT%" "%DEST%"
echo.

echo ============================================
echo   TAMAM! code.guard hazir:
echo   %DEST%
echo ============================================
echo.
pause
