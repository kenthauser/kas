#ifndef KAS_OPTIONS_PO_OPTION_FMT_H
#define KAS_OPTIONS_PO_OPTION_FMT_H

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

Options Description field syntax.

Describe options in a single comma-separated "string". Leading & trailing spaces ignored
in each field. The string fields are as follows:

First field:  (required) program option (also used as sort field for help)
Second field: (optional) pseudonum for program option
Next field:   (optional) option type (eg: flag, takes option, increment) [default flag]
Additional :  (optional) option type dependent: eg: default argument(s), valid option list


Details of the foremat of each field are detailed below.

* First argument: 

Declare option. Must be non-zero. First alphanum used to sort the help message.

Optional prefixes supported:

Begins with '#' : sort as option, but dont install arg
Begins with '!' : yn-option w/o prefix

* Second argument:

If (sort_arg && !first_arg), may begin with any character (support suboptions) 
Begins with `-` or `--` : option pseudonum
Begins with `!`         : yn-option suffix (arg1 is prefix)
If (suboption) following type *should* use "suboption" prefix `s` in "type" field

* Third argument:

(NB: default is `=`)

Begins with `s`: Suboption prefix. Absorbed.
Begins with `H`: Help message prefix. Absorbed.

Begins with `=` : set to value
                - text after `=` is value to be stored on match
                     default value to store is `1`                        
                - if single "additional args" follow: default

Begins with `+` : increment value (eg -v)
                - text after `+` is increment value for match
                     default value to store is `1`                        
                - if single "additional args" follow: initial value
                                     
Begins with `:` : arg expected,
                - remaining after `:` arg name in help message
                - if "additional args" follow, choose from list
                - if single "additional args" follow: default
                

Begins with `@` : arg expected, append arg to list (eg search dirs), 
                - remaining after `@` is arg name
                - if "additional args" follow, install as defaults

Begins with `Z` : clear container


Additional args:

Default values      : example: ("-o,--output,:FILE,a.out")
Multiple defaults   : example: include search path
Option list         : example: ("-compress,:,none,zlib,zlib-gnu,zlib-gabi")

***********************************************************************************

#endif

#include "is_container.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cinttypes>
#include <cstring>
#include <functional>


// define namespace
namespace kas::options
{

// forward declare public base class
struct po_defns;

}

// work in included namespace
namespace kas::options::detail
{

using namespace std::literals::string_literals;

// saved function object type
using OPTION_STORE_FN = std::function<void(void *cb_arg, const char *value)>;

// throw when unable to save argument
using STORE_ERROR_T   = std::invalid_argument;

// holds "additional args" as described above
using EXTRA_VALUES_T  = std::vector<const char *>;


struct po_option_defn
{
    po_option_defn(po_defns& defns, const char *opt, const char *help)
        : opt_string(opt), help_str(help), all_flags(0)
    {
        parse_option_defn(defns);
        //show_parse(std::cout, opt);
        //std::cout << "help_opt = " << help_opt() << std::endl;
        //std::cout << "help_msg = " << help_msg() << std::endl;
    }
    

    // standard callback functions
    // (here for convenience. not really member functions) 
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static auto store_fn(T&, T = {})
    {
        return [](void *cb_arg, const char *data)
            {
                char *end_p {};
                T n = std::strtoimax(data, &end_p, 0);
                if (*end_p)
                    throw STORE_ERROR_T(data);
                *static_cast<T *>(cb_arg) = n;
            };
    }
    
    static auto store_fn(const char *)
    {
        return [](void *cb_arg, const char *data)
            {
                // XXX copy `data` to heap.
                auto p = new std::string(data);
                *static_cast<const char **>(cb_arg) = p->data();
            };
    }

    static auto store_fn(std::string&)
    {
        return [](void *cb_arg, const char *data)
            {
                *static_cast<std::string *>(cb_arg) = data;
            };
    }
    
    template <typename T, typename = std::enable_if_t<is_container_v<T>>>
    static auto store_fn(T&)
    {
        return [](void *cb_arg, const char *data)
            {
                if (data)
                    static_cast<T *>(cb_arg)->push_back(data);
                else
                    static_cast<T *>(cb_arg)->clear();
            };
    }
    
    
    static int multi_to_t(const char *arg, EXTRA_VALUES_T& opts, int)
    {
        int n{};
        for (auto& value : opts) {
            if (!std::strcmp(arg, value))
                return n;
            ++n;
        }
        
        throw STORE_ERROR_T(arg);
    }

    static const char *multi_to_t(const char *arg, EXTRA_VALUES_T& opts, const char *)
    {
        auto n = multi_to_t(arg, opts, 1);

        for (auto& value : opts)
            if (!n--)
                return value;
        return {};
    }

    // declare trivial getters/setters for flags
    bool takes_arg()    const { return flags.takes_arg;    }
    bool is_yn_opt()    const { return flags.is_yn_opt;    }
    bool is_ignored()   const { return flags.is_ignored;   }
    bool is_suboption() const { return flags.is_suboption; }
    void set_ignored()        { flags.is_ignored = true;   }

    // trivial getters
    const char *sort_opt()        const { return d_sort_opt; }
    EXTRA_VALUES_T *data_values() const { return opt_list;   }

    // other getters are not trivial
    const char *first_opt()   const;
    const char *second_opt()  const;
    const char *opt_value()   const;
    const char *get_default() const;

    // support for help messages
    std::string help_opt() const;
    std::string help_msg() const;
    
    OPTION_STORE_FN get_cb(OPTION_STORE_FN const& fn, const char *arg);
    void set_default(void *cb_arg, OPTION_STORE_FN const& fn) const;
private:
    OPTION_STORE_FN save_opt_fixed(OPTION_STORE_FN const& fn, const char *arg);
    OPTION_STORE_FN save_opt_multi(OPTION_STORE_FN const& fn);
    enum OPT_TYPE : uint8_t { OPT_SET, OPT_INCR, OPT_ARG, OPT_APPEND, OPT_CLEAR };

    // implementation methods
    void parse_option_defn(po_defns&);
    std::vector<const char*> split_defn_string(char *p);
    
    // templated to omit object code if not referenced
    template <typename OS> void show_parse(OS&, const char *arg) const;

    std::string opt_string;         // mutable copy of original defn string
    const char *help_str      {};
    const char *d_first_opt   {};
    const char *d_second_opt  {};
    const char *d_sort_opt    {};
    const char *arg_name      {};
    EXTRA_VALUES_T  *opt_list {};

    OPT_TYPE opt_type {OPT_SET};

    // can't init bitfields until c++20 ?!?
    using FLAG_BASE_T = uint8_t;
    struct flag_t {
        FLAG_BASE_T takes_arg    : 1;
        FLAG_BASE_T has_default  : 1;
        FLAG_BASE_T is_yn_opt    : 1;
        FLAG_BASE_T is_ignored   : 1;
        FLAG_BASE_T is_suboption : 1;
        FLAG_BASE_T is_help_only : 1;
        FLAG_BASE_T is_pointer   : 1;
    };

    union {
        flag_t      flags;
        FLAG_BASE_T all_flags;
    };
};
}
#endif
