import json
import os
import re
import sys
import argparse
import shutil
import platform
import getpass

def exit_app(msg, exit_code):
    print(f"\n!VCPRJ! '{msg}'")
    print(f"!VCPRJ! exit_app with code {exit_code}")
    sys.exit(exit_code)


def is_integer(num):
    try:
        int(num)
        return True
    except ValueError:
        return False

# line = 'par1,par2,value with commas\\,par3'
# Output: ['par1', 'par2', 'value with commas,par3']


def split_with_escape_char(line, sep):
    rts = []
    current_param = ''
    escape_next = False

    for char in line:
        if escape_next:
            current_param += char
            escape_next = False
        elif char == '\\':
            escape_next = True
        elif char == sep:
            rts.append(current_param)
            current_param = ''
        else:
            current_param += char

    rts.append(current_param)
    return rts


def include_file(file_to_include, vcprj_arg_list, io_out):
    print(f"[VCPRJ] Include file '{file_to_include}'")
    if os.path.exists(file_to_include):
        with open(file_to_include, "r") as io_include:
            lines_to_include = io_include.readlines()
        if vcprj_arg_list[0] != "":
            inc_with_var_mode = True
        else:
            inc_with_var_mode = False
        vcprj_arg_id = 0
        for var0 in vcprj_arg_list:
            vcprj_arg_id = vcprj_arg_id + 1
            lines = lines_to_include.copy()
            if len(vcprj_arg_list) > 1:
                # More than one entry
                # Last one do nothing
                if vcprj_arg_id < len(vcprj_arg_list):
                    # Not the last one: add close/open brace for next include block;
                    lines.append("},\n")
                    lines.append("{\n")
            for line in lines:
                line = line.lstrip()  # do not remove \n at the end of the line
                if (line.startswith("//")):  # comment
                    continue
                if inc_with_var_mode:
                    line = line.replace("$[0]", var0)
                # print(f"include :'{line}'")
                if (not line.endswith("\n")):
                    line = line + "\n"
                io_out.write(line)
    else:
        exit_app(f"Cannot find include file: '{file_to_include}'", 1)


def definition_file(file_to_include, default_dictionary_list):
    print(f"[VCPRJ] Define variable from file '{file_to_include}'")
    if os.path.exists(file_to_include):
        with open(file_to_include, "r") as io_include:
            vcprj_default = json.load(io_include)
            default_dictionary_list.append(vcprj_default)
    else:
        exit_app(f"Cannot find definition file: '{file_to_include}'", 2)
    return default_dictionary_list


def beautify_json(file_in, file_out):
    with open(file_in, 'r') as io_in:
        json_data = json.load(io_in)

    beautified_json = json.dumps(json_data, indent=4)  # , sort_keys=True)

    with open(file_out, 'w') as io_out:
        io_out.write(beautified_json)

    return beautified_json


if __name__ == "__main__":
    cli_parser = argparse.ArgumentParser(
        description="This script is used with Visual Code to manage compile/deploy/debug functionnalities in the project file")
    # Mandatory arguments
    requiredArguments = cli_parser.add_argument_group("Mandatory arguments")
    requiredArguments.add_argument("--jsonin", dest="jsonin", help="The path to the input file to be processed", required=True)
    requiredArguments.add_argument("--jsonout", dest="jsonout", help="The path to the output file which will be generated (Should be .vscode/launch.json or .vscode/tasks.json)", required=True)
    # Optional arguments
    cli_parser.add_argument("--sys", dest="sys", default="Lin", help="Define the value which is used to replace ??? in tag like $[???.param] such as Lin or Win for selecting var.")
    cli_parser.add_argument("--usr", dest="usr", default="", help="Define the value which is used to replace ??? in Vc_Def cmd. It is used to select the 'good' ...def.'user'.json file among various version defined by each user of the project")
    cli_parser.set_defaults(verbose=False)
    cli_parser.add_argument("--verbose", dest="verbose", action="store_true", help="Activate verbose mode.")
    cli_parser.set_defaults(verbose=False)
    cli_parser.add_argument("-d", "--dryrun", dest="dryrun", action="store_true", help="Activate dry run.")
    cli_parser.set_defaults(dryrun=False)
    # cli_parser.add_argument("--version",   dest="version",   help="The version to override")

    print(f"[VCPRJ] (v) 2.1.1 (d) 01/08/24 (a) B.harmel\n")
    argc = len(sys.argv)
    print(f"{argc-1} command line arguments passed to '{sys.argv[0]}' (cwd '{os.getcwd()}')")
    for i in range(1, argc):
        print(sys.argv[i])
    if len(sys.argv) == 1:
        cli_parser.print_help(sys.stderr)
        exit_app("Incomplete argument", 3)
    args = cli_parser.parse_args()
    if args.sys == "":
        exit_app(f"No sys arg specified, must be Win, Lin or Web", 4)        
        #if platform.system() == 'Windows':
        #    args.sys = "Win" 
        #else:
        #    args.sys = "Lin"  # for $[???.cmake_generator]
    if args.usr == "":            
        args.usr = getpass.getuser()
    print(f"Running for user '{args.usr}' target '{args.sys}'")    
    if not os.path.exists(args.jsonin):
        exit_app(f"Cannot find '{args.jsonin}'", 5)

    default_dictionary_list = []
    # regex to decode vcprj command embedded as json var such as "!Vc_Gen(Name)" : "arg1,arg2,..."
    vcprj_cmd_re = re.compile(r"\s*\"!Vc_(\w*).*\((.*)\).*:.*\"(.*)\"")
    # regex to find text between $[] such as $[2] (Gen Arg) or $[VcprjWinParam.dbg_local_ut_BofStd]
    vcprj_var_re = re.compile(r"\$\[(.*?)\]")
    pre_processed_file = os.path.splitext(args.jsonin)[0] + ".PreProcessed.json"
    backup_file = os.path.splitext(args.jsonout)[0]+".back.json"
    print(f"[VCPRJ] ------- Make a backup of '{args.jsonout}' into '{backup_file}' -------")
    if os.path.exists(args.jsonout):
        shutil.copy2(args.jsonout, backup_file)

    print(
        f"[VCPRJ] ------- Pass 1: Preprocess include, definition and comment of '{args.jsonin}' into '{pre_processed_file}' -------")
    # Parse file line by line to process special command such as comment or include
    with open(args.jsonin, "r") as io_json_in:
        with open(pre_processed_file, "w") as io_pre_processed_file_out:
            for line in io_json_in:
                line = line.lstrip()  # do not remove \n at the end of the line
                # print(f"Input: '{line}")
                if (line.startswith("//")):  # comment
                    continue
                matches = vcprj_cmd_re.search(line)
                if (matches) and (len(matches.groups()) == 3):
                    file_to_include = ""
                    vcprj_cmd = matches.group(1).strip()
                    vcprj_name_list = matches.group(2).strip().split(",")
                    vcprj_arg_list = matches.group(3).strip().split(",")
                    file_to_include = vcprj_name_list[0]
                    if vcprj_cmd == "Inc":
                        include_file(file_to_include, vcprj_arg_list, io_pre_processed_file_out)
                    elif vcprj_cmd == "Sys":
                        if len(vcprj_name_list)==1 and len(vcprj_arg_list)==1 and "???" in vcprj_arg_list[0]:
                            arg_sys = vcprj_arg_list[0].replace("???", args.sys)
                            line = f'"{vcprj_name_list[0]}":"{arg_sys}"\n'
                        io_pre_processed_file_out.write(line)                       
                    elif vcprj_cmd == "Def":
                        if "???" in file_to_include:
                            file_to_include = file_to_include.replace("???", args.usr)
                        default_dictionary_list = definition_file(file_to_include, default_dictionary_list)
                    else:
                        io_pre_processed_file_out.write(line)
                else:
                    io_pre_processed_file_out.write(line)

# at this step, we have processed //, Def and Inc command but perhaps
# an Inc command has re-inserted //, Def or Inc command. We only manage 2 level of Inc
    # with open(pre_processed_file, "r") as io_include:
    #    lines = io_include.readlines()
    # beautify_json(pre_processed_file, pre_processed_file)
    gen_file = os.path.splitext(args.jsonin)[0] + ".Generation.json"
    print(f"[VCPRJ] ------- Pass 2: Process '{pre_processed_file}' generation into '{gen_file}' -------")
    with open(pre_processed_file, "r") as io_pre_processed_file_in:
        with open(gen_file, "w") as io_gen_file_out:
            for line in io_pre_processed_file_in:
                line = line.lstrip()  # do not remove \n at the end of the line
                if (line.startswith("//")):  # comment
                    continue
                matches = vcprj_cmd_re.search(line)
                if (matches) and (len(matches.groups()) == 3):
                    file_to_include = ""
                    vcprj_cmd = matches.group(1).strip()
                    # vcprj_name_list = matches.group(2).strip().split(",")
                    vcprj_name_list = split_with_escape_char(
                        matches.group(2).strip(), ',')
                    # vcprj_arg_list = matches.group(3).strip().split(",")
                    vcprj_arg_list = split_with_escape_char(
                        matches.group(3).strip(), ',')

                    file_to_include = vcprj_name_list[0]
                    if vcprj_cmd == "Inc":
                        include_file(file_to_include,
                                     vcprj_arg_list, io_gen_file_out)
                    elif vcprj_cmd == "Def":
                        default_dictionary_list = definition_file(file_to_include, default_dictionary_list)
                    elif vcprj_cmd == "Gen":
                        if (len(vcprj_name_list) == 3):
                            action_name = vcprj_name_list[0]
                            file_to_include = vcprj_name_list[1]
    # launch:   0:"name"    1:"preLaunchTask"
    # tasks:    0:"label"   1:"detail" rem: "dependsOn" is managed by argument
    # settings: 0: user tag 1: user value
                            vcprj_arg_list.insert(0, action_name)
                            vcprj_arg_list.insert(1, vcprj_name_list[2])
                            print(
                                f"[VCPRJ] Process generation of '{action_name}' based on {file_to_include} using {vcprj_arg_list[1:]}")
                            if os.path.exists(file_to_include):
                                with open(file_to_include, "r") as io_include:
                                    lines = io_include.readlines()
                                for line in lines:
                                    # make a copy of line as the value used in 'for match in re.finditer(vcprj_var_re, line_to_parse):' cannot modify it
                                    line_to_parse = line
                                    for match in re.finditer(vcprj_var_re, line_to_parse):
                                        index = match.group(1)
                                        # Get the starting index of the match
                                        start_match_index = match.start()
                                        # Check if the character before the match is '$'
                                        # if (start_match_index > 0) and (line[start_match_index - 1] == '$'):
                                        #    match_empty_mode = True
                                        #    line = line[:start_match_index] + line[start_match_index + 1:]
                                        # else:
                                        #    match_empty_mode = False

                                        if is_integer(index):
                                            arg_index = int(index)
                                            if arg_index < 0:
                                                match_empty_mode = True
                                                line = line[:start_match_index+2] + \
                                                    line[start_match_index + 3:]
                                                arg_index = -arg_index
                                            else:
                                                match_empty_mode = False
                                            if (arg_index >= 0) and (arg_index < len(vcprj_arg_list)):
                                                arg_list = vcprj_arg_list[arg_index].split(
                                                    ";;")
                                                if len(arg_list) > 1:
                                                    if ((len(arg_list) % 2) == 0):
                                                        colon_mode = True
                                                    else:
                                                        exit_app(f"List length ({len(arg_list)}) delimitted with ';;' must be even: {arg_list}", 6)
                                                else:
                                                    colon_mode = False
                                                    # arg_list = vcprj_arg_list[arg_index].split(";")
                                                    arg_list = split_with_escape_char(
                                                        vcprj_arg_list[arg_index], ';')
                                                if len(arg_list) > 1:
                                                    json_arg = ""
                                                    if colon_mode:
                                                        for i in range(0, len(arg_list), 2):
                                                            json_arg = (
                                                                json_arg + '"' + arg_list[i] + '":"' + arg_list[i + 1] + '",\n')
                                                    else:
                                                        for arg in arg_list:
                                                            json_arg = (
                                                                json_arg + '"' + arg + '",\n')
                                                    # replace "$[n]" (with the quote !)
                                                    line = line.replace(
                                                        f'"$[{arg_index}]"', json_arg[:-2])
                                                else:
                                                    # print(f"line '{line}' rep '$[{arg_index}]' by '{vcprj_arg_list[arg_index]}'")
                                                    if match_empty_mode and (vcprj_arg_list[arg_index] == ""):
                                                        # replace "$[n]" (with the quote !)
                                                        line = line.replace(
                                                            f'"$[{arg_index}]"', "")
                                                    else:
                                                        # replace $[n] (without the quote !)
                                                        line = line.replace(
                                                            f'$[{arg_index}]', vcprj_arg_list[arg_index])
                                                    # print(f"final line '{line}'")
                                            else:
                                                exit_app(f"Arg index ({arg_index}) is out of range", 7)
                                        else:
                                            exit_app(f"Arg index ({index}) is not a number", 8)
                                    io_gen_file_out.write(line)
                            else:
                                exit_app(f"Cannot find include file: '{file_to_include}'", 9)
                        else:
                            exit_app(f"Bad number of paramter: cmd '{vcprj_cmd}' name '{vcprj_name_list}' arg '{vcprj_arg_list}'", 10)
                    else:
                        exit_app(f"Unknown command: cmd '{vcprj_cmd}' name '{vcprj_name_list}' arg '{vcprj_arg_list}'", 11)
                else:
                    if "???" in line:
                        line_key = line.replace("???", args.sys)                    
                        for json_def in default_dictionary_list:
                            print(f"aa={json_def[args.sys]}")
                            if line_key in json_def[args.sys]:
                                json_key = json_def[args.sys]
                    io_gen_file_out.write(line)

    print(f"[VCPRJ] ------- Pass 3: Build result from '{gen_file}' -------")
    new_lines = []
    with open(gen_file, "r") as io_gen_file_in:
        lines = io_gen_file_in.readlines()

    for line in lines:
        if args.verbose:
            print(f"[VCPRJ] Process line: {line}", end="")
        for match in re.finditer(vcprj_var_re, line):
            key = match.group(1)
            keys = key.split(".")
            if len(keys) == 2:
                if args.verbose:
                    print(f"[VCPRJ] Look for Key '{key}' in def file")
                json_value_found = False
                for json_def in default_dictionary_list:
                    if keys[0] == "???":
                        keys[0] = args.sys
                    if keys[0] in json_def:
                        json_key = json_def[keys[0]]
                        if keys[1] in json_key:
                            json_value = json_key[keys[1]]
                            if args.verbose:
                                print(
                                    f"Key '{key}' found, value is '{json_value}'")
                            # replace the text with the value of the environment variable
                            line = line.replace(f"$[{key}]", json_value)
                            json_value_found = True
                            if args.verbose:
                                print(f"[VCPRJ] Line is now:  {line}", end="")
                if json_value_found == False:
                    exit_app(f"Key '{key}': not found", 12)
            else:
                exit_app(f"Key '{key}': bad synthax", 13)
        new_lines.append(line)

    if args.verbose:
        print(f"[VCPRJ] Write and beautify result in '{args.jsonout}'")
    if not os.path.exists("../.vscode"):
        os.mkdir("../.vscode")
    with open(args.jsonout, "w") as io_jsonout_out:
        for line in new_lines:
            io_jsonout_out.write(line)
    beautified_json = beautify_json(args.jsonout, args.jsonout)

    if args.verbose:
        print(
            f"\n-------------------------------------------------\nFinal output in '{args.jsonout}':")
        for line in beautified_json:
            print(f"{line}", end="")
        print(f"\n-------------------------------------------------")

    if args.dryrun == False:
        # all is good, clean up
        os.remove(pre_processed_file)
        os.remove(gen_file)
    print(f"[VCPRJ] exit_app(0)...")
