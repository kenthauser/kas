#!/usr/bin/python3

# set up links for assembler

# eg: configure m68k
#
# at present only single configuration per target supported
# eventually "arm-none-elf" may be supported, but for now, just "arm"

from pathlib import Path
import os, sys

def do_configure(target):
    
    # declare source and target roots
    top_dir    = Path('.')
    target_dir = top_dir / target

    # these are the configuration files to link in
    files = [ 'machine_makefile.inc'
            , 'machine_out.h'
            , 'machine_parsers.h'
            , 'machine_types.h']

    # create list of symbolic links
    pairs = []
   
    # validate config target
    if not target_dir.is_dir():
        raise ValueError(target_dir, "is not a directory")

    # validate linked to files are present
    for file in files:
        dest = top_dir    / file
        src  = target_dir / (target + '_' + file)
        if not src.is_file():
            raise ValueError(src, " not found: invalid target")
        pairs.append({'dest': dest, 'src': src})

    # all linked to files are present -- symlink in
    for pair in pairs:
        try:
            os.unlink(pair.get('dest'))
        except OSError:
            pass
        os.symlink(pair.get('src'), pair.get('dest'))

if __name__ == '__main__':
    import glob
    import argparse
    from os import path

    parser = argparse.ArgumentParser(
        description="Configure kas for target",
        epilog = "Default is to display current target")

    parser.add_argument('target')
    args = parser.parse_args()

 #   if args.error:
 #       print ("Display configuration")
 #   else:
    do_configure(args.target)


        

    

