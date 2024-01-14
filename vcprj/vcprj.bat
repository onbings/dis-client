@echo off
rem Set default values for sys and usr
set "sys=%1"
set "usr=%2"

rem Set default values if no arguments are specified
if not defined sys set "sys=Win"
rem if not defined usr set "usr=default_value"

rem Check if the number of arguments is not more than 2
if not "%3"=="" (
    call :displayHelp
    exit /b 1
)

rem Check if the value of sys is valid (Win, Lin, or Web)
if /i not "%sys%"=="Win" if /i not "%sys%"=="Lin" if /i not "%sys%"=="Web" (
    call :displayHelp
    exit /b 1
)

echo "sys: %sys%"
echo "usr: %usr%"

python vcprj.py --jsonin=vcprj_tasks.json  --jsonout=../.vscode/tasks.json --verbose --sys=%sys%
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_tasks.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_launch.json --jsonout=../.vscode/launch.json --verbose --sys=%sys%
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_launch.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_settings.json --jsonout=../.vscode/settings.json --verbose --sys=%sys%
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_settings.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_code-workspace.json --jsonout=../.vscode/bofstd.code-workspace --verbose --sys=%sys%
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_code-workspace.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_keybindings.json --jsonout=%USERPROFILE%/AppData/Roaming/Code/User/keybindings.json --verbose --sys=%sys%
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of keybindings.json.
    exit /b %ERRORLEVEL%
)

exit /b 0

:displayHelp
echo Usage: %~nx0 [sys] [usr]
echo.
echo This script takes two optional arguments.
echo Default values: sys=Lin, usr=none
echo Valid values for sys: Win, Lin, Web
echo.
echo Example: %~nx0 Web BHA
exit /b