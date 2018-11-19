#ifndef KAS_OPTIONS_PO_DEFN_IMPL_H
#define KAS_OPTIONS_PO_DEFN_IMPL_H

#include "po_option_impl.h"
#include "po_help.h"

namespace kas::options::detail
{
void defn_group::add_option(po_option_defn& d, OPTION_STORE_FN const& fn, void *cb_arg)
{
    auto add = [&](const char *opt, const char *yn_value, bool& do_init)
        {
            // no option string, no option
            if (!opt) return;

            //std::cout << "add_entry: " << opt << std::endl; 

            // add entry to option parsing map
            auto& entry = base.add_entry(opt, d, cb_arg).second;
            
            // if no call-back function, parse-only
            if (fn) {
                entry.cb_fn = d.get_cb(fn, yn_value);
                if (do_init)
                    entry.option.set_default(cb_arg, entry.cb_fn);
                do_init = false;
            }
        };
    
    // yes-no is only option where first & second "options" store different value.
    bool do_init{true};
    add(d.first_opt(),  "1", do_init);
    add(d.second_opt(), "0", do_init);
}

}

namespace kas::options
{
    
auto po_defns::get_options(int& argc, char **argv,
        OPTION_ERROR_FN_T err_fn) -> decltype(argv)
{
    bool is_short_option {};
    bool argv_on_stack   {};

    auto proc_option = [&](auto iter, const char *option_arg = {})
    {
        // if option requires an arg, supply one.
        if (iter->second.takes_arg() && !option_arg)
            if (argc-- > 0)
                option_arg = *++argv;

        // make sure options & args are matched
        if (iter->second.takes_arg() && !option_arg) {
            std::cout << "error: option requires argument: " << iter->first << std::endl;
            is_short_option = false;
        } else if (!iter->second.takes_arg() && option_arg) {
            std::cout << "error: option don't accept argument " << iter->first << std::endl;
            is_short_option = false;
        } else {
            // OK -- drive callback
            iter->second(iter->first, option_arg);
        }

    };

    // use lambda for sup-option processing
    // NB: very closely linked with `get_options` processing
    auto proc_subopt = [&](auto const& entry, auto arg, char *option_arg)
    {
        if (option_arg)
            *--option_arg = '=';        // undo "look for =arg"
        std::cout << "get_options::suboption: " << arg << std::endl;

        if (!arg[2])
            arg = std::string(entry.option.get_default()).data();

        if (!arg[2]) {
            std::cout << "error: suboption without options or default:" << arg << std::endl;
            is_short_option = false;
            return;
        }

        auto first = options.lower_bound(arg);
        auto end   = options.upper_bound(arg);
#if 0
        std::cout << "sub: lower_bound: " << first->first << std::endl;
        if (end != options.end())
            std::cout << "sub: upper_bound: " << end->first  << std::endl;
        else
            std::cout << "sub: upper_bound: end()" << std::endl;
#endif
        for(arg += 2; *arg; ++arg) {
            std::cout << "sub: matching: " << *arg << std::endl;
            auto iter = first;
            for (; iter != end; ++iter) {
                if (*arg == iter->first[2]) {
                    if (iter->second.takes_arg())
                       // consume remaining charcters: don't get next positional
                       return proc_option(iter, ++arg);
                    proc_option(iter);
                    break;
                }
                std::cout << "sub: no match: " << iter->first << std::endl;
            }
            if (iter == end) {
                std::cout << "error: suboption not found: " << *arg << std::endl;
                break;
            }  
        }
    };



    // don't pick up name if reentering get_options()
    if (!prog_name) { 
	    prog_name = *argv++;
        --argc;
    } else
        argv_on_stack = true;

	for (;argc > 0; ++argv, --argc) {
		auto arg = *argv;
        char *option_arg = {};      // for suboption must be non-const
            
		//std::cout << "parsing : " << arg << std::endl;

        // see if option parsing complete
        if (arg[0] != '-')
            break;

        // hardcode "--" as end-of-arguments
        if (eoa_string && !strcmp(arg, eoa_string))
            break;

        // allow "--option=arg" format
        if (auto s = strchr(arg, '=')) {
            *s++ = {};
            option_arg = s;
        }
        

        // loop because of short options
        // abosorb one per iteration
        is_short_option = {};
        for(;;) {
            auto iter = options.lower_bound(arg);
            auto end  = options.upper_bound(arg);
#if 0
            std::cout << "lower_bound: " << iter->first << std::endl;
            if (end != options.end())
                std::cout << "upper_bound: " << end->first  << std::endl;
            else
                std::cout << "upper_bound: end()" << std::endl;
#endif
            for (; iter != end; ++iter) {
                std::cout << "testing: " << iter->first << std::endl;
                // check for short arg
                if (strlen(iter->first) == 2) {
                    is_short_option = true;
                    break;

                // if previous loop found short option, only allow short options this loop thru
                } else if (!is_short_option) {
                    // check for actual match
                    if (!strcmp(iter->first, arg))
                        break;
                }

                std::cout << "no match: " << iter->first << std::endl;
            }
            
            if (iter == end) {
                if (is_short_option)
                    arg[2] = 0;
                std::cout << "error: not a valid option: " << arg << std::endl;
                break;
            }

            // special processing for suboptions
            if (iter->second.is_suboption()) {
                proc_subopt(iter->second, arg, option_arg);
                break;
            }
            
            // don't allow short options to use "=arg" as suffix
            // NB: suboption processing already broken off: `=` can be suboption.
            if (option_arg && is_short_option) {
                std::cout << "error: \"=arg\" format not allowed with short options: " << iter->first << std::endl;
                break;
            }

            proc_option(iter, option_arg);

            // only short options loop
            if (!is_short_option)
                break;

            // adjust arg to consume next short arg 
            option_arg = {};    // consumed
            *++arg = '-';       // consumed one option
            if (!arg[1])
                break;          // end of short option string
        }
    }
   
    if (argc < 0)
        argc = 0;

    return argv;
}

template <typename OS>
void po_defns::help(OS& os) const
{
    detail::po_help help(d_groups);

    // XXX a one-line usage message    
    help.print(os);
}

}
#endif
