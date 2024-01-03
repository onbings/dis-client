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
python3 ./vcprj.py --jsonin=./vcprj_code-workspace.json --jsonout=../.vscode/dis-client.code-workspace --verbose --sys=Lin
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_code-workspace."
    exit $exit_code
fi
mkdir -p $HOME/.config/Code/User
#if vscode launched from linux
python3 ./vcprj.py --jsonin=vcprj_keybindings.json --jsonout=$HOME/.config/Code/User/keybindings.json --verbose --sys=Lin
#if vscode launched from windows with a linux in a docker (does not work as C is not visibl)python3 ./vcprj.py --jsonin=vcprj_keybindings.json --jsonout=C:/Users/bha/AppData/Roaming/Code/User/keybindings.json --verbose --sys=Lin
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_keybindings."
    exit $exit_code
fi
