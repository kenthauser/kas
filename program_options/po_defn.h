#ifndef KAS_OPTIONS_PO_DEFN_H
#define KAS_OPTIONS_PO_DEFN_H

#if 0

***********************************************************************************

Program Option Notes:

KAS options have *many* variations and require a special "program options" parser.

Handled option strings are:

-s                      // short
-s arg                  // short (immed arg or after short args)
--long                  // long
--long arg              // long immed arg
-s, --long              // short or long
-s, --long arg          // short or long followed by arg (using short or long fmts)
--long=arg              // long option only: arg follows `=`
-{pfx}[no-]ynshort      // short {optional prefix} with yn arg
--{pfx}[no-]ynlong      // long {optional prefix} with  yn arg

-s, sub                 // short followed by short "suboptions"

-opt=[list]             // choose from list
--opt=[list]            // choose from list...

In addition, short single character options may be combined. 
e.g. `tar -xvzf kas.tgz` is the same as `tar -x -v -z -f kas.tgz`

***********************************************************************************

The KAS program option 

Program options are defined using one of three overloads for the `operator()` method of 
`po_XXXX`. These overloads are as follows:

auto& operator()(const char *desc, const char *help = {})

template <typename T>
auto& operator()(const char *desc, const char *help, T& flag)

template <typename T, typename CB_OBJ>
auto& operator()(const char *desc, const char *help, T& flag, CB_OBJ cb)

The first overload declares options, which are accepted and ignored.

The second overload stores option resulton "flag" using built-in callback overloads
for type `T`.  Overloads exist for integral types, const char *, std::basic_string<T>, 
and standard containers.

The final overload accepts a function object to handle callback processing.

If a `function object` is supplied, it must be support the following signature:
void(const char *name, void *cb_arg, const char *value)

NB: the `cb_arg` is passed as `void *`. The function object must be pre-configured
for cb_arg type `T`.


***********************************************************************************

#endif

#include "po_option_fmt.h"

#include <vector>
#include <string>
#include <exception>
#include <iostream>
#include <cinttypes>
#include <forward_list>
#include <map>
#include <deque>


namespace kas::options::detail
{

using namespace std::literals::string_literals;


// base structure holds all option definitons
using HELP_EPILOGUE_FN = void(*)(std::ostream&);

struct defn_group
{
    defn_group(po_defns& base, const char *name = {})
                    : base(base), group_name(name) {}

    template <typename T, typename FN>
    auto& operator()(const char *opt, const char *help, T& cb_arg, FN const& cb_obj)
    {
       auto& d = defns.emplace_back(base, opt, help);
       add_option(d, cb_obj, &cb_arg);
       return *this;
    }
    
    template <typename T>
    auto& operator()(const char *opt, const char *help, T& cb_arg)
    {
       auto& d = defns.emplace_back(base, opt, help);
       add_option(d, d.store_fn(cb_arg), &cb_arg);
       return *this;
    }
    
    auto& operator()(const char *opt, const char *help = {})
    {
       auto& d = defns.emplace_back(base, opt, help);
       add_option(d, {});
       return *this;
    }

    // XXX add CB_FN overload
    
    // XXX add "help" methods

    void add_option(po_option_defn& d, OPTION_STORE_FN const& fn, void *cb_arg = {});
    
    const char *                        group_name; // group title
    po_defns                           &base;       // base
    std::deque<po_option_defn>          defns;      // save for help message
    std::forward_list<HELP_EPILOGUE_FN> epilogues;  // append to help message
};

}


namespace kas::options
{
using OPTION_ERROR_FN_T = void(*)(const char *option, const char *msg);

struct po_defns
{
    // one po_entry per option
    struct po_entry
    {
        detail::po_option_defn const &option;
        void *cb_arg;
        detail::OPTION_STORE_FN cb_fn;

        void operator()(const char *arg, const char *value)
        {
            std::cout << "set_option: " << arg << " = ";
            std::cout << (value ? value : "null") << std::endl;

            if (cb_fn)
                cb_fn(cb_arg, value);
        }

        bool takes_arg() const
        {
            return option.takes_arg();
        }

        bool is_suboption() const
        {
            return option.is_suboption();
        }

    };

    // map compare function compares substrings equal
    struct defn_compare
    {
        bool operator()(const char *lhs, const char *rhs) const
        {
            for (;*lhs && *rhs; ++lhs, ++rhs)
            {
                if (*lhs < *rhs)
                    return true;
                if (*lhs > *rhs)
                    return false;
            }

            // substrings compare same
            // needed for "short option" support
            return false;
        }
    };

    po_defns() = default;

    // return appropriate group to add option to
    auto& add(const char *group_name = {})
    {
        // return existing group which matches name
        for (auto& d : d_groups)
        {
            if (group_name == d.group_name)
                return d;
            if (group_name && d.group_name)
                if (!strcmp(group_name, d.group_name))
                    return d;
        }

        // create new group at front
        // NB: reverse order for help message
        return d_groups.emplace_front(*this, group_name);
    }

    // create objects with same lifetime as `po_defns`
    template <typename T, typename...Ts>
    const char *new_string(std::string data, T&& arg, Ts&&...args)
    {
        return new_string(std::move(data.append(arg)), std::forward<Ts>(args)...);
    }

    const char *new_string(std::string data)
    {
        return d_extra_strings.emplace_front(std::move(data)).data();
    }

    // typically pass iterator pair or nothing.
    template <typename...Ts>
    auto& new_list(Ts&&...args)
    {
        return d_extra_values.emplace_front(std::forward<Ts>(args)...);
    }

    // returns new argv. drive error function for bad options
    auto get_options(int& argc, char **argv, OPTION_ERROR_FN_T err_fn = {})
                -> decltype(argv);

    // display help message
    template <typename OS> void help(OS& os) const;

    template <typename...Ts>
    auto& add_entry(const char *option, Ts&&...args)
    {
        return *options.emplace(option, po_entry{std::forward<Ts>(args)...});
    }

    auto name() const { return prog_name; }

private:
    std::forward_list<detail::defn_group>     d_groups;
    std::multimap<const char *, po_entry, defn_compare> options;
    //std::multimap<const char *, po_entry> options;
    std::forward_list<detail::EXTRA_VALUES_T> d_extra_values;
    std::forward_list<std::string>            d_extra_strings;
    const char *prog_name {};
    static constexpr const char *eoa_string = "--";
};

}
#endif
