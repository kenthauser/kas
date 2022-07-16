#!/usr/bin/python3

# set up links for assembler

# eg: configure m68k
#
# at present only single configuration per target supported
# eventually "arm-none-elf" may be supported, but for now, just "arm"

from pathlib import Path
import os, sys

# declare source and target roots
top_dir       = Path('.')
config_status = Path('config.status')

# these are the configuration files to symlink in
files = [ 'machine_makefile.inc'
        , 'machine_out.h'
        , 'machine_parsers.h'
        , 'machine_types.h'
        , 'test_files'
        ]

# remove all configuration files
def un_configure():
    try:
        os.unlink(config_status)
    except OSError:
        pass

    for file in files:
        try:
            os.unlink(top_dir / file)
        except OSError:
            pass

# validate linked files are present and return as array
def get_configure(target):
    
    # create list of symbolic links
    target_dir = top_dir / target
    pairs = []
   
    # validate config target
    if not target_dir.is_dir():
        raise ValueError(target_dir, "is not a directory")

    # validate linked to files are present
    for file in files:
        dest = top_dir    / file
        src  = target_dir / (target + '_' + file)
        if not src.exists():
            raise ValueError(src, " not found: invalid target")
        pairs.append({'dest': dest, 'src': src})

    # return array
    return pairs


# link in config files
def do_configure(target):
    
    # remove existing configuration
    un_configure()
    
    # symlink in files from config array
    for pair in get_configure(target):
        os.symlink(pair.get('src'), pair.get('dest'))

    # save configured target in status file
    f = open(config_status, 'w')
    f.write(target + '\n')
    f.close()
    

if __name__ == '__main__':
    import glob
    import argparse
    from os import path

    parser = argparse.ArgumentParser(
        description="Configure kas for target",
        epilog = "Default is to display current target")

    parser.add_argument("-d", help="unconfigure kas and exit", action="store_true")
    parser.add_argument('target', help="target architecture", nargs='?')
    args = parser.parse_args()

    if args.d:
        un_configure()
        exit(0)
        
    if args.target:
        try:
            do_configure(args.target)
        except BaseException as err:
            print("configure failed: {0}".format(err))
            exit(1)
            
    elif config_status.is_file():
        f = open(config_status)
        print ("current configuration: " + f.read())
    else:
        print ("kas is unconfigured")
        print ("valid configurations are: ", end='')
        sep = ''
        for path in os.listdir(top_dir):
                try:
                    get_configure(path)
                    print(sep + path, end='')
                    sep = ", "
                except:
                    pass
        print ('')



        

    

