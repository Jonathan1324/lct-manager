@echo off
setlocal enabledelayedexpansion

set ROOT=%~1
set PATTERN=%~2

set BASE=%CD%

for /R "%ROOT%" %%f in (%PATTERN%) do (
    set "FILE=%%f"
    set "REL=!FILE:%BASE%\=!"
    echo !REL:\=/!
)
