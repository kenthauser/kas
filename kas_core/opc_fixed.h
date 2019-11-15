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
    using value_type = T;
    using op_size_t  = typename opcode::op_size_t;


    using NAME = m_name<sizeof(T), KAS_STRING("INT")>;
    
    template <typename CI>
    static op_size_t proc_one(CI& ci, expr_t const& e, kas_loc const& loc)
    {
        // if fixed argument, check if it fits
        std::cout << "opc_size::proc_one: " << e << std::endl;
        if (auto p = e.get_fixed_p()) {
            if (expression::expr_fits().ufits<T>(*p) == core_fits::yes)
                *ci++ = *p; 
            else {
                std::cout << "opc_size: doesn't fit, value = " << *p;
                std::cout << " sizeof(T) == " << sizeof(T) << std::endl;
                // XXX
                //*ci++ = (expr_t)parser::kas_diag::error("Invalid argument", e.get_loc_p());
            }
        } else
        {
            // not fixed -- resolve at emit
            *ci++ = e;
        }
        
        // always fixed size
        return sizeof(T);
    }
        
    static void emit_one(emit_base& base, expr_t const& value, core_expr_dot const *dot_p)
    {
        // evaluate expression & emit
#if 0
        std::cout << "opc_fixed::emit_one: " << value << std::endl;
#endif
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
    static op_size_t proc_one(CI& ci, expr_t const& e, kas_loc const& loc)
    {
        *ci++ = e;
        return size_one;
    }

    static constexpr unsigned sizeof_diag = size_one;
    
    static void emit_one(emit_base& base, expr_t const& value, core_expr_dot const *dot_p)
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

    using string_t = typename expression::e_string<>::type;

    template <typename CI>
    static op_size_t proc_one(CI& ci, expr_t const& e, kas_loc const& loc)
    {
        std::size_t size = 0;

        // XXX need to add support for additional e_string<> types...
        // XXX coordinate with kas_string definitions...
        if (auto p = e.template get_p<string_t>()) {
            auto& ks = p->get();
            size += ks.size() + ZTerm::value;
            for (auto& c : ks)
                *ci++ = c;
            if (ZTerm::value)
                *ci++ = 0;
        } else {
            // XXX express all others args as nullptr...
            *ci++ = 0;
            size += 1;
        }
        return size;
    }

};
}
#endif
