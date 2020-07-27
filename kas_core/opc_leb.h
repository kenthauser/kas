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
    template <template <typename> typename LEB, typename MAX_T = uint32_t>
    struct opc_leb  : opc_data<opc_leb<LEB, MAX_T>, uint8_t>
    {
        using value_t   = uint8_t;      // leb output always `uint8_t`
        using leb_t     = LEB<MAX_T>;
        using base_t    = opc_data<opc_leb<LEB, MAX_T>, value_t>;
        using op_size_t = typename opc::opcode::op_size_t;
        using NAME      = typename leb_t::NAME;

        template <typename CI>
        static op_size_t proc_one(CI& ci, kas_token const& tok)
        {
            // confirm token represents a `value` token, not `syntax` token
            if (!tok.expr_index())
                *ci++ = e_diag_t::error("Invalid value", tok);

            // if fixed, emit `bytes` into stream
            else if (auto p = tok.get_fixed_p())
                return proc_one(ci, *p);
             
            // if variable, emit expression & calculate size later
            else
                *ci++ = tok.expr();
            return { 1, leb_t::max_size() };
        }
       
        // for internal use by dwarf: no error checking
        template <typename CI>
        static op_size_t proc_one(CI& ci, expr_t e)
        {
            if (auto p = e.get_fixed_p())
                return proc_one(ci, *p);
            *ci++ = e;
            return { 1, leb_t::max_size() };
        }

        template <typename CI>
        static op_size_t proc_one(CI& ci, MAX_T value)
        {
            auto fn = [&ci](value_t n) { *ci++ = n; };
            return leb_t::write(fn, value);
        }
    
        
        static op_size_t size_one(expr_t const& v, core_fits const& fits)
        {
            std::cout << "\nopc_leb::size_one: " << v << std::endl;
            // calculate size min/max
    #if 0
            short min = 1;
            for (; min < leb_t::max_size(); ++min)
                if (fits.fits(v, leb_t::min_value(min), leb_t::max_value(min)) != fits.no)
                    break;

            short max = min;
            for (; max < leb_t::max_size(); ++max)
                if (fits.fits(v, leb_t::min_value(max), leb_t::max_value(max)) == fits.yes)
                    break;
#else
            typename op_size_t::value_t min = 1;
            for (; min < leb_t::max_size(); ++min)
            {
                auto result = fits.fits(v, leb_t::min_value(min), leb_t::max_value(min));
                std::cout << "result = " << result << std::endl;
                if (result != fits.no)
                    break;
            }
            auto max = min;
            for (; max < leb_t::max_size(); ++max)
            {
             //   if (fits.fits(v, leb_t::min_value(max), leb_t::max_value(max)) == fits.yes)
               //     break;

                auto result = fits.fits(v, leb_t::min_value(min), leb_t::max_value(max));
                std::cout << "result = " << result << std::endl;
                if (result == fits.yes)
                    break;
            }
            std::cout << "size = {" << min << ", " << max << "}" << std::endl;
#endif

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
            if (auto p = e.template get_p<core_expr_t>())
                if (p->num_relocs() == 0)
                    return (void)leb_t::write(fn, p->get_offset()());
            // not resolved to fixed -> error
            base << emit_filler(1); 
#endif
        }
    };
}

// actual pseudo-op types
using opc_uleb128 = detail::opc_leb<expression::uleb128, uint32_t>;
using opc_sleb128 = detail::opc_leb<expression::sleb128, int32_t>;

}

#endif
