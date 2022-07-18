
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

The `./configure` command can be used to display current target, or to
change targets.

NB: 2022/07/18 at present, g++ (11.3.0)  doesn't like technique used
to instantiate type with virtual bases. From `kas/init_from_list.h:95`

    template <typename DFLT, typename NAME, typename T, typename...Ts>
    struct vt_ctor_impl<DFLT, meta::list<NAME, T, Ts...>, void>
    {
        using type = vt_ctor_impl;
        static const inline T instance{Ts::value...};
        static constexpr auto value = &instance;
    };

g++ won't allow `constexpr value` to be inited with address of
`static const instance`.
To work around this, use clang++ to compile. ie

    make CXX=clang++ 

or change the `CXX` variable in Makefile

If there are boost related link errors, make `clone-boost clean all`. 
The parts of `boost` used by `kas` are header only.
Older versions of `boost` may require linking with various libraries.

debian requires `LIB = -lstdc++fs`


NB: This assembler is incomplete and is provided for information.

