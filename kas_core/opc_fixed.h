#ifndef KAS_CORE_OPC_FIXED_H
#define KAS_CORE_OPC_FIXED_H

#include "utility/string_mpl.h"
#include "opcode.h"
#include "core_chunk_inserter.h"
#include "core_fixed.h"
#include "core_print.h"
#include "core_fits.h"
//#include "opc_leb.h"
#include <cstdint>

#if 0

Refactor as follows:

1. base class writes data into a `kas_chunk<N>`, where `N` is chunk size.

2. base class stores first elements into fixed

3. base class passed `expr2size` function. Takes `expr` & returns size of
   expression. (Really just to support leb128.) Most can return `N`

4. base class generates looped `write_n` for emit

5. base class yields inserter

6. base class takes type with two methods: first processes one element (`proc_one`),
   and the second takes expression to calculate size.

XXXXXX

7. may be better to have as (non constexpr) member. code not duplicated. except inserter.

8. CRTP mixin which uses member. Just define proc_one & size_expr.
   Both default to nullptr: ie `N`

#endif

namespace kas::core::opc
{

// generate name in format "m_name<2, "INT"> -> INT<2>
template <int N, typename NAME>
using m_name = string::str_cat<NAME, KAS_STRING("<"), i2s<N>, KAS_STRING(">")>;


//
// base `opcode` for variable number of fixed size elements
//  eg. `.byte`, `.long`, `.double`
//

template <typename T>
struct opc_fixed : opc_data<opc_fixed<T>, T>
{
    using base_t     = opc_data<opc_fixed<T>, T>;
    using value_type = T;
    using op_size_t  = typename opcode::op_size_t;
    using kas_position_tagged = typename base_t::kas_position_tagged;


    using NAME = m_name<sizeof(T), KAS_STRING("INT")>;
#if 1
    // primary entry is `e_fixed_t` arg
    template <typename CI>
    static op_size_t proc_one(CI& ci
                            , kas_position_tagged const& loc 
                            , e_fixed_t arg)
    {
        if (expression::expr_fits().ufits<T>(arg) == core_fits::yes)
            *ci++ = arg; 
        else
        {
            // must mark diagnostic here, as numbers don't carry `loc`
            *ci++ = e_diag_t::error("Value out-of-range", loc);
        }
        return sizeof(T);
    }

    // allow expressions
    template <typename CI>
    static op_size_t proc_one(CI& ci, kas_position_tagged const&, expr_t e)
    {
        ci++ = std::move(e);
        return sizeof(T);
    }

    // support other integral values (which fit in `e_fixed_t`)
    template <typename CI, typename ARG_T>
    std::enable_if_t<std::is_integral_v<std::remove_reference_t<ARG_T>>
                    && sizeof(ARG_T) <= sizeof(e_fixed_t), op_size_t>
    static proc_one(CI& ci, kas_position_tagged const& loc, ARG_T&& arg)
    {
        return proc_one(ci, loc, static_cast<e_fixed_t>(arg));
    }

    // non-matching args are errors
    // NB: integral values too big for `e_fixed_t` are to be processed as `expr_t`
    template <typename CI, typename ARG_T, typename = void>
    std::enable_if_t<!std::is_integral_v<std::remove_reference_t<ARG_T>>, op_size_t>
    static proc_one(CI& ci, kas_position_tagged loc, ARG_T)
    {
        print_type_name{"opc_fixed::ARG_T"}.name<ARG_T>();
        *ci++ = e_diag_t::error("Invalid value", loc);
        return sizeof(T);
    }

#else
    template <typename CI>
    static op_size_t proc_one(CI& ci, kas_position_tagged const& loc, kas_token const& tok)
    {
        // confirm token represents a `value` token, not `syntax` token
        if (!tok.expr_index())
            *ci++ = e_diag_t::error("Invalid value", tok);

        // if fixed argument, check if it fits
        else if (auto p = tok.get_fixed_p())
        {
            // if fits in "chunk", save as data
            if (expression::expr_fits().ufits<T>(*p) == core_fits::yes)
                *ci++ = *p; 
            else
            {
                // must mark diagnostic here, as numbers don't carry `loc`
                *ci++ = e_diag_t::error("Value out-of-range", tok);
            }
        } 

        // not fixed -- resolve at emit
        else
            *ci++ = tok.expr();
        
        // always fixed size
        return sizeof(T);
    }
    
    // for internal use by dwarf: no error checking
    template <typename CI>
    static op_size_t proc_one(CI& ci, kas_position_tagged const& loc, T value)
    {
        *ci++ = value;
        return sizeof(T);
    }

    template <typename CI, typename ARG_T>
    std::enable_if_t<!std::is_integral_v<ARG_T>, op_size_t>
    static proc_one(CI& ci, kas_position_tagged const& loc, ARG_T const& arg)
    {
        if constexpr (std::is_same_v<ARG_T, const char *>)
            throw std::logic_error {"opc_fixed: const char *"};
        else 
        {
            if (auto p = arg.get_fixed_p())
                *ci++ = *p;
            else
                *ci++ = arg;    // convert to arg
        }
        return sizeof(T);
    }
#endif
        
    static void emit_one(core_emit& base
                       , expr_t const& value
                       , core_expr_dot const *dot_p
                       )
    {
        // evaluate expression & emit
        base << set_size(sizeof(value_type));
        base << value;
    }
};
#if 1
//
// XXX
// Rework to store *each* arg as a `float` expression.
// In `emit_one` &/or core_emit &/or backend work on serialize.
// Use #bits for `size_one`

template <unsigned NBits>
struct opc_float : opc_data<opc_float<NBits>, expression::e_float_t>
{
    using value_type = expression::e_float_t;
    using op_size_t  = typename opcode::op_size_t;

    using NAME = m_name<NBits, KAS_STRING("FLT")>;

    // XXX need bits->float size macro
    // for now, round up to multiple of 32bits
    static constexpr unsigned words_one = (NBits + 31)/32;
    static constexpr unsigned size_one  = words_one * (32/8);

    template <typename CI>
    static op_size_t proc_one(CI& ci, kas_position_tagged const& loc, kas_token const& tok)
    {
        *ci++ = tok.expr();
        return size_one;
    }

    static constexpr unsigned sizeof_diag = size_one;
    
    static void emit_one(core_emit& base
                       , expr_t const& value
                       , core_expr_dot const *dot_p
                       )
    {
        // evaluate expression & emit
#if 0
        std::cout << "opc_fixed::emit_one: " << value << std::endl;
        base << set_size(sizeof(value_type));
        base << value;
#endif
    }
};
#endif

// have two opc_data string types (ascii/asciz)
template <typename ZTerm, typename char_type = char>
struct opc_string : opc_data<opc_string<ZTerm, char_type>, char_type>
{
    using value_type = char_type;
    using op_size_t  = typename opcode::op_size_t;

    using NAME = m_name<ZTerm::value, KAS_STRING("STR")>;

    template <typename CI>
    static op_size_t proc_one(CI& ci
                            , kas_position_tagged const& loc
                            , std::basic_string<char_type> const& str)
    {
        for (auto& c : str)
            *ci++ = c;
        if (ZTerm::value)
            *ci++ = 0;
        
        return str.size() + ZTerm::value;
    }

    // dwarf passes null-terminated strings
    template <typename CI>
    static op_size_t proc_one(CI& ci, kas_position_tagged const&, char_type const *p)
    {
        auto size = std::strlen(p) + ZTerm::value;
        while (*p)
            *ci++ = *p++;
        if (ZTerm::value)
            *ci++ = 0;
        return size * sizeof(char_type);
    }
    
    // non-matching args are errors
    template <typename CI, typename ARG_T>
    static op_size_t proc_one(CI& ci, kas_position_tagged loc, ARG_T)
    {
        print_type_name{"opc_string::ARG_T"}.name<ARG_T>();
        *ci++ = e_diag_t::error("Invalid value", loc);
        return 0;
    }

};
}
#endif
