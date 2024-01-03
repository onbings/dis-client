@echo off
python vcprj.py --jsonin=vcprj_tasks.json  --jsonout=../.vscode/tasks.json --verbose --sys=Win
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_tasks.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_launch.json --jsonout=../.vscode/launch.json --verbose --sys=Win
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_launch.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_settings.json --jsonout=../.vscode/settings.json --verbose --sys=Win
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_settings.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_code-workspace.json --jsonout=../.vscode/dis-client.code-workspace --verbose --sys=Win
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_code-workspace.
    exit /b %ERRORLEVEL%
)
python vcprj.py --jsonin=vcprj_keybindings.json --jsonout=%USERPROFILE%/AppData/Roaming/Code/User/keybindings.json --verbose --sys=Win
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of keybindings.json.
    exit /b %ERRORLEVEL%
)