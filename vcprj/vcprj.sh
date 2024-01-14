#!/bin/bash

# Set default values for sys and usr
sys=${1:-"Lin"}
usr=${2:-""}

# Check if the number of arguments is not more than 2
if [ "$#" -gt 2 ]; then
    displayHelp
    exit 1
fi

# Check if the value of sys is valid (Win, Lin, or Web)
if [ "${sys,,}" != "win" ] && [ "${sys,,}" != "lin" ] && [ "${sys,,}" != "web" ]; then
    displayHelp
    exit 1
fi

echo "sys: $sys"
echo "usr: $usr"

python3 ./vcprj.py --jsonin=./vcprj_tasks.json  --jsonout=../.vscode/tasks.json --verbose --sys=$sys
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_tasks."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_launch.json --jsonout=../.vscode/launch.json --verbose --sys=$sys
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_launch."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_settings.json --jsonout=../.vscode/settings.json --verbose --sys=$sys
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_settings."
    exit $exit_code
fi
python3 ./vcprj.py --jsonin=./vcprj_code-workspace.json --jsonout=../.vscode/bofstd.code-workspace --verbose --sys=$sys
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_code-workspace."
    exit $exit_code
fi
mkdir -p $HOME/.config/Code/User
#if vscode launched from linux
python3 ./vcprj.py --jsonin=vcprj_keybindings.json --jsonout=$HOME/.config/Code/User/keybindings.json --verbose --sys=$sys
#if vscode launched from windows with a linux in a docker (does not work as C is not visibl)python3 ./vcprj.py --jsonin=vcprj_keybindings.json --jsonout=C:/Users/bha/AppData/Roaming/Code/User/keybindings.json --verbose --sys=Lin
if [ $exit_code -ne 0 ]; then
    echo "Error: Failed to execute the parsing of vcprj_keybindings."
    exit $exit_code
fi

exit 0

displayHelp() {
    echo "Usage: $0 [sys] [usr]"
    echo
    echo "This script takes two optional arguments."
    echo "Default values: sys=Lin, usr=default_value"
    echo "Valid values for sys: Win, Lin, Web"
    echo
    echo "Example: $0 Web my_custom_value"
}
