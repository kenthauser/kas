#ifndef KAS_CORE_KAS_SLEB_H
#define KAS_CORE_KAS_SLEB_H

#include "core_fixed.h"
#include "expr/expr_fits.h"
#include "expr/expr_leb.h"
#include <meta/meta.hpp>

#include <limits>
#include <functional>


namespace kas::core::opc
{

namespace detail
{

    // now declare the LEB128 `opc` interfaces

    template <template <typename> typename LEB, typename VALUE_T = uint32_t>
    struct opc_leb  : opc_data<opc_leb<LEB, VALUE_T>, uint8_t>
    {
        using leb_t     = LEB<VALUE_T>;
        using base_t    = opc_data<opc_leb<LEB, VALUE_T>, uint8_t>;
        using value_t   = VALUE_T;
        using op_size_t = typename opc::opcode::op_size_t;
        using NAME      = typename leb_t::NAME;


        template <typename CI>
        static op_size_t proc_one(CI& ci, expr_t&& e, kas_loc const& loc)
        {
            // if fixed, emit `bytes` into stream
            if (auto p = e.get_fixed_p()) {
                auto fn = [&ci](auto n) { *ci++ = n; };
                return leb_t::write(fn, *p);
            }
             
            // if variable, emit expression & calculate size later
            *ci++ = std::move(e);
            return { 1, leb_t::max_size() };
        }
       
        static op_size_t size_one(expr_t const& v, core_fits const& fits)
        {
            std::cout << "opc_leb::size_one" << std::endl;
            // calculate size min/max
            short min = 1;
            for (; min < leb_t::max_size(); ++min)
                if (fits.fits(v, leb_t::min_value(min), leb_t::max_value(min)) != fits.no)
                    break;

            short max = min;
            for (; max < leb_t::max_size(); ++max)
                if (fits.fits(v, leb_t::min_value(max), leb_t::max_value(max)) == fits.yes)
                    break;

            return {min, max};
        }

        
        static void emit_one(emit_base& base, expr_t const& e, core_expr_dot const *dot_p)
        {
            std::cout << "opc_leb::emit_one: " << e << std::endl;
#if 1  
            auto fn = [&base](value_t n) { base << core::byte << n; };
            // if resolved to fixed, emit data
            if (auto p = e.get_fixed_p())
                return (void)leb_t::write(fn, *p);
          
            // if expression resolves to constant offset, emit it
            if (auto p = e.template get_p<core_expr>())
                if (p->num_relocs() == 0)
                    return (void)leb_t::write(fn, p->get_offset()());
            // not resolved to fixed -> error
            base << emit_filler(1); 
#endif
        }
    };
}

// actual pseudo-op types
using opc_uleb128 = detail::opc_leb<expression::uleb128>;
using opc_sleb128 = detail::opc_leb<expression::sleb128>;

}

#endif
