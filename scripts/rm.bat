@echo off
setlocal enabledelayedexpansion

:loop
if "%~1"=="" goto :eof

if exist "%~1\" (
    rem Directory
    rmdir /S /Q "%~1"
) else (
    rem File
    del /F /Q "%~1" >nul 2>&1
)

shift
goto loop
