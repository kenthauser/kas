
#include "test_options.h"
#include "po_defn_impl.h"


int main(int argc, char **argv)
{
    
    kas::options::po_defns defns;

    //add_gen_args(defns);
    //add_target_args(defns);
    //po_help help(defns);

    //help.dump();

    kas_gen_options(defns);
    //kas_elf_options(defns);
    kas_listing_options(defns);
    kas_m68k_options(defns);
    
    int help_flag {};
    bool target_flag {};

    defns.add()
		("--help"        , "show this message and exit", help_flag)
		("--target-help" , "show the target specific options", target_flag)
        ;

    
    //help.print(std::cout);

    //defns.help(std::cout);


    argv = defns.get_options(argc, argv);

    std::cout << "positional args (count = " << argc << "): ";
    while (*argv) {
        std::cout << *argv++ << ", ";
    }

    std::cout << std::endl;

    std::cout << "help_flag = " << help_flag << std::endl;

    if (help_flag) {
        defns.help(std::cout);
        return 1;
    }
    else if (target_flag) {
        kas::options::po_defns target_defns;
        kas_m68k_options(target_defns);
        target_defns.help(std::cout);
        return 1;
    }
}

