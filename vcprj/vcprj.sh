#!/bin/bash
python3 ./vcprj.py --jsonin=./vcprj_tasks.json  --jsonout=../.vscode/tasks.json --verbose --sys=Lin
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_tasks."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_launch.json --jsonout=../.vscode/launch.json --verbose --sys=Lin
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_launch."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_settings.json --jsonout=../.vscode/settings.json --verbose --sys=Lin
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_settings."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_settings.json --jsonout=../.vscode/settings.json --verbose --sys=Lin
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_settings."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_code-workspace.json --jsonout=../.vscode/dis-client.code-workspace --verbose --sys=Lin
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_code-workspace.
    exit /b %ERRORLEVEL%
)
python3 ./vcprj.py --jsonin=./vcprj_keybindings.json --jsonout=../.vscode/vcprj_keybindings.json --verbose --sys=Lin
IF %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to execute the parsing of vcprj_keybindings.json.
    exit /b %ERRORLEVEL%
)