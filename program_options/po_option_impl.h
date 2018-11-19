#ifndef KAS_OPTIONS_PO_OPTION_IMPL_H
#define KAS_OPTIONS_PO_OPTION_IMPL_H

// implement methods in `po_option_defn`

#include "po_defn.h"

#include <vector>
#include <string>
#include <exception>
#include <iostream>
#include <cinttypes>


namespace kas::options::detail
{

void po_option_defn::parse_option_defn(po_defns& defns)
{
    // NB: c++17 specifies `std::string::data()` is mutable (except trailing null)
    auto args = split_defn_string(opt_string.data());

    // iter needs to outlive "for" loop
    auto iter = args.begin();
    while (iter != args.end())
    {
        auto arg = *iter++;

        // see if testing for "options"
        // must always have at least one "option"
        if (!d_first_opt && !d_sort_opt) {
            // test for yn-flag
            if (arg[0] == '!') {
                flags.is_yn_opt = true;
                ++arg;      // skip flag & calculate prefix
                
                if (arg[0] == '-' && arg[1] == '-') {
                    d_first_opt  = "--";
                    d_second_opt = arg + 2;
                } else {
                    if (arg[0] == '-')
                        ++arg;
                    d_first_opt = "-";
                    d_second_opt = arg;
                }

                d_sort_opt = d_second_opt;
                if (!*d_second_opt)
                    throw std::invalid_argument("first option: bad yn_option");
                continue;
            }
            // test for sub-option flag
            if (arg[0] == '&') {
                flags.is_suboption = true;
                d_first_opt = ++arg;
            } 
            // test for sort-only flag
            else if (arg[0] == '#')
                ++arg;
            else 
                d_first_opt = arg;

            // not special. Skip over `-` or `--` for sort
            // allow "first arg" to be anything (ie support @FILE)
            while (!std::isalnum(*arg))
                ++arg;
            
            if (arg[0] != 0) {
                d_sort_opt = arg;
                continue;
            }
                
            throw std::invalid_argument("first option: "s + arg);
        }
        
        // second option has less flexibilty
        if (!d_second_opt) {
            // suboption second character (flag only)
            if (flags.is_suboption) {
                if (strlen(arg) != 1)
                    throw std::invalid_argument("sub option not flag");
                // save character as second arg (used in help message)
                d_second_opt = arg;
                d_first_opt = defns.new_string(d_first_opt, d_second_opt);
                continue;
            }

            if (arg[0] == '!') {
                if (arg[1] == 0)
                    throw std::invalid_argument("second option: "s + arg);
                flags.is_yn_opt = true;
                d_second_opt = ++arg;

                // put sort option in persistent memory
                // can also server as "--{pfx}{suffix}" option string
                auto sort_str_p = new std::string(d_sort_opt);
                *sort_str_p += d_second_opt;
                d_sort_opt = sort_str_p->data();
                continue;
            }

            // if starts with `-`, it's an option
            if (arg[0] == '-') {
                d_second_opt = arg;
                
                // validate '-' or '--' followed by option
                ++arg;
                if (arg[0] == '-')
                    ++arg;
                if (arg[0] != 0)
                    continue;       // don't allow `-` with no option

                // didn't start with "-" or "--"
                throw std::invalid_argument("second option: "s + arg);
                }
            
            // if starts with `#` pairs with sort-only first option to define
            // "help-only" entry.
            if (arg[0] == '#') {
                d_second_opt = ++arg;
                flags.is_help_only = true;
                continue;
            }

        }

        // not first nor second arg. Must specify action
        // loop to facilitate "prefixes"
        for(; *arg; ++arg) {
            switch (*arg) {
                // prefixes:
                case 'H':
                    flags.is_help_only = true;
                    continue;
                case '&':
                    flags.is_suboption = true;
                    continue;

                // regular type options
                case ':':       // arg follows. rest of arg is "arg name"
                    opt_type = OPT_ARG;
                    flags.takes_arg = true;
                    break;
                case '=':       // set to value. rest of arg is "default value"
                    opt_type = OPT_SET;
                    break;
                case '@':       // append arg to container
                    opt_type = OPT_APPEND;
                    flags.takes_arg = true;
                    break;
                case '+':       // increment value
                    opt_type = OPT_INCR;
                    break;
                default:
                    throw std::invalid_argument("option action "s + arg);
            }
            break;
        }

        // save remaining part of "arg_type" as "arg_name"
        if (*arg && *++arg)
            arg_name = arg;

        // done processing
        break;
    }

    // if remaining args, store as "opt_list"
    auto n = std::distance(iter, args.end());
    if (n) {
        opt_list = &defns.new_list(iter, args.end());
        if (n == 1 || opt_type == OPT_APPEND)
            flags.has_default = true;
    }

    // precalculate "yn_option" names into lifetime static memory in `defns`
    if (flags.is_yn_opt) {
        // need opt-list for our own purpose
        if (opt_list)
            throw std::invalid_argument("yn_option with extra args"s + opt_string);

        const char *names[2];
        names[0] = defns.new_string(d_first_opt, d_second_opt);
        names[1] = defns.new_string(d_first_opt, "no-", d_second_opt);
        opt_list = &defns.new_list(std::begin(names), std::end(names));
    }
}

// split string into comma-separated tokens, skipping leading/trailing whitespace
std::vector<const char*> po_option_defn::split_defn_string(char *p)
{
    // return pointer to next arg. Terminate current on white-space or delmiter
    auto next_arg = [&](char *p) -> decltype(p)
        {
            constexpr auto delim = ',';

            // skip argument chars until delim or whitespace
            for(; std::isgraph(*p); ++p)
                if (*p == delim) {
                    *p = 0;         // clear delim (end current arg)
                    return ++p;     // next arg starts after delim
                }

            // if arg ended by whitespace, trim arg
            if (std::isspace(*p))
                *p++ = 0;

            // skip trailing white-space until non-space character
            while (std::isspace(*p))
                ++p;

            // check end condition:
            if (!*p)                // at end-of-string
                return nullptr;     // all done
            if (*p == delim)        // delim following whitespace
                return ++p;         // arg previously terminated by whitespace
            
            throw std::invalid_argument("option specification: " + opt_string);
        };

    std::vector<const char*> v;
    while (p)
    {
        // skip leading whitespace
        while (std::isspace(*p))
            ++p;

        v.push_back(p);     // save pointer to start of arg
        p = next_arg(p);    // get pointer to next arg
    }

    return v;
}


const char *po_option_defn::first_opt() const
{   
    // yn-option names pre-caluclated in extra args
    if (flags.is_yn_opt) {
        auto iter = opt_list->begin();
        return *iter;
    }

    return d_first_opt;
}

const char *po_option_defn::second_opt() const
{   
    // yn-option names pre-caluclated in extra args
    if (flags.is_yn_opt) {
        auto iter = opt_list->begin();
        return *++iter;
    }

    return d_second_opt;
}

OPTION_STORE_FN 
po_option_defn::get_cb(OPTION_STORE_FN const& fn, const char *yn_value)
{
// update yn_value to hold target value
    if (flags.takes_arg)
        yn_value = {};
    else if (!flags.is_yn_opt)
        yn_value = "1";     // need: get_value()
    // here actully is YN -- everything OK

#if 0
    // see if builtin call-back writes to const char * or string
    if (fn.target() == store_fn(std::decl_val<const char *>{}).target()) {
        std::cout << "const char * fn" << std::endl;
    }
#endif

    // select proper call-back function
    if (!flags.takes_arg)
        return save_opt_fixed(fn, yn_value);
    else if (opt_list && !flags.has_default && !flags.is_yn_opt)
        return save_opt_multi(fn);
    else
        return fn;
}

OPTION_STORE_FN
po_option_defn::save_opt_fixed(OPTION_STORE_FN const& fn, const char *arg)
{
    // copy fn & arg into lambda object.
    return [fn, arg](void *cb_arg, auto value)
        {
            if (!value)
                value = arg;

            fn(cb_arg, value);
        };
}

OPTION_STORE_FN
po_option_defn::save_opt_multi(OPTION_STORE_FN const& fn)
{
    //std::cout << "found multi-opt" << std::endl;
    return [fn, opt_list=opt_list](void *cb_arg, auto value)
        {
            int n = multi_to_t(value, *opt_list, 1);
            fn(cb_arg, std::to_string(n).c_str());
        };
}

const char *po_option_defn::get_default() const
{
    if (!flags.has_default)
        return {};
    if (!flags.is_yn_opt && !flags.takes_arg)
        return arg_name;
    return *opt_list->begin();
}

void po_option_defn::set_default(void *cb_arg, OPTION_STORE_FN const& fn) const 
{
    auto opt = first_opt();
    if (!opt)
        opt = second_opt();

    if (!flags.has_default)
        return;
   
    if (flags.is_suboption)
        return;     // suboption defaults not handled here
        
    //std::cout << "setting default for " << opt << std::endl;

    // append is special: can have multiple defaults
    if (opt_type == OPT_APPEND) {
        for (auto& value : *opt_list)
            fn(cb_arg, value);
        return;
    }

#if 0
    if (auto value = get_default())
        std::cout << "default value = " << value << std::endl;
#endif   
    if (auto value = get_default())
        fn(cb_arg, value);

}

// templated so no object code emitted when not used
template <typename OS>
void po_option_defn::show_parse(OS& os, const char *arg) const
{
    auto pr_arg = [](const char *value, const char *name = {})
        {
            std::string msg;
            if (name)
                msg = name + " = "s;
            if (value)
                msg += "\""s + value + "\"";
            else
                msg += "null";
            return msg;
        };

    auto pr_bool = [](bool value, const char *name)
        {
            std::string msg(name);
            if (value)
                msg += " = true";
            else
                msg += " = false";
            return msg;
        };

    os << "option parsing: \"" << arg << "\"" << std::endl;
    os << pr_arg(d_sort_opt, "sort") << ", ";
    os << pr_arg(d_first_opt, "first") << ", ";
    os << pr_arg(d_second_opt, "second") << std::endl;

    os << pr_arg(arg_name, "arg_name") << ", ";

    os << pr_bool(flags.is_yn_opt, "YN_opt") << ", type " << "=+:@Z"[opt_type];
    os << ", extra = ";
    if (!opt_list)
        os << "none";
    else {
        auto pfx = "[";
        for (auto xtra : *opt_list) {
            os << pfx << pr_arg(xtra);
            pfx = ", ";
        }
        os << "]";
    }
    os << '\n' << std::endl;
}

std::string po_option_defn::help_opt() const
{
    std::string msg;

    if (flags.is_yn_opt) {
        msg = d_first_opt + "[no-]"s + d_second_opt;
    } else {
        if (d_first_opt && d_second_opt)
            msg = d_first_opt + ","s + d_second_opt;
        else
            msg = d_first_opt ? d_first_opt : d_second_opt;
    }

    if (flags.is_suboption && d_second_opt)
        return {};
        
    if (flags.takes_arg)
        msg += " "s + (arg_name ? arg_name : "arg");

    if (opt_list && !flags.has_default && !flags.is_yn_opt) {
        auto delim = "={";
        for (auto arg : *opt_list) {
            msg += delim;
            msg += arg;
            delim = "|";
        }
        msg += "}";
    }

    return msg;
}

std::string po_option_defn::help_msg() const
{
    if (!help_str)
        return "[ignored]";
    
    std::string msg(help_str);

    if (flags.is_help_only)
        return msg;

    if (flags.is_suboption && d_second_opt)
        msg = d_second_opt + "\t"s + msg;

    if (flags.is_suboption && !d_second_opt)
        msg += "\nSub-options";

    if (flags.is_ignored) 
        msg += " [ignored]";

    if (!flags.has_default)
        return msg;

    // help string is a little complicated.
    msg += " [default";
    auto delim = ": ";
    if (flags.is_yn_opt && arg_name) {
        msg += delim;
        msg += (std::atoi(arg_name) != 0) ? "yes" : "no";
    } else if (!flags.takes_arg && arg_name && !strcmp(arg_name, *opt_list->begin())) {
        /* nothing */
    } else for (auto arg : *opt_list) {
        msg += delim;
        if (arg)
            msg += arg;
        else
            msg += "none";
        delim = ",";
    }

    msg += "]";
    return msg;
}
}
#endif
