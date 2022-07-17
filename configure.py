#!/usr/bin/python3

# set up symlinks to configure assembler

# eg: ./configure m68k
#
# At present only single configuration per target supported.
# Currently: pseudos are BSD, output is ELF
# eventually "arm-bsd-elf" may be supported, but for now, just "arm"

from pathlib import Path
import os

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

# create list of symlinked file pairs to implement configuration
def get_configure(target):
    # accumulate result in `pairs`
    pairs = []
    target_dir = top_dir / target
   
    # validate config target
    if not target_dir.is_dir():
        raise ValueError(target_dir, "not a directory")

    # validate linked to files are present
    for file in files:
        dest = top_dir    / file
        src  = target_dir / (target + '_' + file)
        if not src.exists():
            raise ValueError(src, "not found")
        pairs.append({'dest': dest, 'src': src})

    # return array
    return pairs


# symlink in config files
def do_configure(target):
    # remove existing configuration
    un_configure()
    
    # symlink file pairs from get_configure list
    for pair in get_configure(target):
        os.symlink(pair.get('src'), pair.get('dest'))

    # save configured target in status file (previosly deleted)
    f = open(config_status, 'w')
    f.write(target + '\n')
    f.close()
    

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(
        description="Configure kas for target architecture",
        epilog = "Default is to display current configuration")

    parser.add_argument("-d", help="unconfigure kas and exit", action="store_true")
    parser.add_argument('target', help="target architecture", nargs='?')
    args = parser.parse_args()

    # perform configuration operations
    if args.d:
        un_configure()
        exit(0)
    elif args.target:
        try:
            do_configure(args.target)
        except BaseException as err:
            print("configure failed: {0}".format(err))
    
    # report current configuration
    if config_status.is_file():
        f = open(config_status)
        print ("configured for " + f.read(), end='')
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
        exit(1)

