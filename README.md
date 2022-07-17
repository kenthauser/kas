
KAS : Kent's retargetable assembler

This project is a retargetable assembler written in C++

This project uses the SPIRIT parser which is part of the BOOST C++ libraries.
To compile this project, the boost library packing should be installed using
the host system's package manager. Alternative, typeing `make clone-boost`
will download the needed portions of boost from github. This download uses
approximately 2G of disk space.

To compile the system perform the following:

     % make      # activates submodules and creates './configure'
     % ./configure [target]
     % make      # builds default target

The system currently only supports ELF output and BSD pseudo ops.

The ./configure command can be used to display current target, or to
change targets.

NB: This assembler is incomplete and is provided for information.

