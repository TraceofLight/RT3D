@echo off
REM Visual Studio Pre-Build Event용 코드 생성 스크립트

setlocal

set "EMBEDDED_PYTHON=%~dp0..\Python\python.exe"

REM 1. 임베디드 Python 체크
if exist "%EMBEDDED_PYTHON%" (
    "%EMBEDDED_PYTHON%" "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

REM 2. 임베디드 Python 자동 다운로드/설치
echo [CodeGen] Embedded Python not found. Attempting automatic setup...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0EnsurePython.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo [CodeGen] Auto-install failed. Falling back to system Python.
    goto :SystemPython
)

if exist "%EMBEDDED_PYTHON%" (
    "%EMBEDDED_PYTHON%" "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

:SystemPython
REM 3. 시스템 Python 체크
py --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    py "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

REM 4. Python 없음
echo [ERROR] Python not found!
echo Please install Python to Tools\Python\ or system PATH
echo See Tools\PYTHON_SETUP.md for instructions
exit /b 1
