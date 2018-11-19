#ifndef KAS_EXEC_KAS_OPTIONS_H
#define KAS_EXEC_KAS_OPTIONS_H

// handle assembler program options

#include "machine_parsers.h"        // configured headers
#include "exec_options.h"
#include "kas_core/core_options.h"  // core modules
#include "program_options/po_defn_impl.h"

namespace kas::exec
{

// XXX also needs error fn
auto get_file_options(options::po_defns &defns, const char *file)
{
    std::cout << "read options from file: " << file << std::endl;
}

auto get_env_options(options::po_defns &defns, const char *env)
{
    std::cout << "read options from environment: " << env << std::endl;
}

struct add_options
{
    options::po_defns& defns;

    template <typename T>
    std::void_t<decltype(std::declval<T>()(defns))>
    operator()(T const& options)
    {
        options(defns);
    }

    void operator()(...) {}
};


auto get_options(int& argc, char **argv, options::OPTION_ERROR_FN_T warn_fn = {})
                -> decltype(argv)
{
    static constexpr auto EXIT_HELP = 1;        // exit code for help messages
    static constexpr auto KAS_ENVIRON = "xxx";  // environment variable for options
    
    options::po_defns defns;

    // add all program defns
    using all_options = all_defns_flatten<expression::detail::options_types_v>;
    meta::for_each(all_options{}, add_options{defns});
    add_exec_options(defns);

    // define & install options handled by options parser
    bool help_flag {};
    bool target_flag {};
    bool version_flag {};

    // implemented in this function
    // add these last to sort @FILE to bottom
    defns.add()
		("--help"        , "show this message and exit"             , help_flag)
		("--target-help" , "show the target specific options"       , target_flag)
        ("--version"     , "print assembler version number and exit", version_flag)
        ("#-z,#@FILE,H"  , "read options from FILE")
        ;

    // environment options: after default, before command line
    if (auto p = std::getenv(KAS_ENVIRON))
        get_env_options(defns, p);

    // process command line options (including @FILE)
    for(; argc; --argc, ++argv) {
        argv = defns.get_options(argc, argv, warn_fn);

        if (!*argv || **argv != '@')
            break;

        get_file_options(defns, *argv + 1);
    }

    // execute help & version commands
    if (help_flag) {
        defns.help(std::cout);
        std::exit(EXIT_HELP);
    }
    else if (target_flag) {
        // create no `po_defns` with only target options
        kas::options::po_defns target_defns;
        using target_options = meta::_t<expression::detail::options_types_v<defn_cpu>>;
        target_options{}(target_defns);
        target_defns.help(std::cout);
        std::exit(EXIT_HELP);
    }
    else if (version_flag) {
        std::cout << defns.name() << ": version = xxx.xxx" << std::endl;
        std::exit(EXIT_HELP);
    }

    // return positional options
    return argv;
}
}
#endif
