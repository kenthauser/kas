#ifndef KAS_EXPR_EXPR_FITS
#define KAS_EXPR_EXPR_FITS

// `expr_fits` is used to determine if a `expr_t` value
// is within a min/max value pair (inclusive)
//
// `expr_fits` is designed as a base class which performs the following:
//
// 1. fits(expr_t, long min, long max) == evaluates if `expr` is within limits
//
// 2. fits<T>(expr_t)  == evaluates if `expr` with within min/max for type
//
// 3. ufits<T>(expr_t) == same above but uses `signed T`::min and `unsigned T`::max
//
// The general `fits` method is a visitor trampoline for `expr_t`.
//
// overloads for `e_fixed_t` and `kas_error_t` are provided. Other types
// are dispatched to derived class's overload if they exist.
//
// A `constexpr` overload is provided to allow integral types to
// be evaluated at compile time.


//#include "expr_variant.h"

#include "expr.h"

#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>
#include <type_traits>

#include "utility/print_type_name.h"

namespace kas::expression
{
    struct expr_fits
    {
        using result_t = fits_result;
        using result_type = result_t;   // XXX

        // general use constants: expose in type
        static auto constexpr yes   = DOES_FIT;
        static auto constexpr no    = NO_FIT;
        static auto constexpr maybe = MIGHT_FIT;

        // could declare actual types & use braced{}-init
        // however, unsigned values cause unintuitive results. correct for
        // using signed types in the `integral` overload.
        using fits_min_t   = int64_t;
        using fits_max_t   = int64_t;
        using fits_value_t = int64_t;

        virtual ~expr_fits() = default;

        // general entrypoint & variant trampoline
        virtual result_t fits(expr_t const& e, fits_min_t min, fits_max_t max) const
        {
            // allow `expr_fits` base type to identify fixed value expressions
            if (auto p = e.get_fixed_p())
                return (*this)(*p, min, max);
#if 0
            // XXX make min/max member variables so not passed around
            // XXX allows better variant support.
            return e.apply_visitor(
                x3::make_lambda_visitor<result_t>
                    ([&](auto const& value)
                        {
                            std::cout << "fits::fits: value = " << expr_t(value) << std::endl;
                            return (*this)(value, min, max);
                        }
                    ));
#else
            return MIGHT_FIT;
#endif
        }

        // displacement trampoline overridden where displacements are understood...
        virtual result_t disp(expr_t const& e, fits_min_t min, fits_max_t max, int delta) const
        {
            if (auto p = e.get_fixed_p())
                return (*this)(*p + delta, min, max);   // just signed constant
            return maybe;       // constants know nothing else of offsets...
        }

        virtual bool seen_this_pass(expr_t const& e) const
        {
            return false;
        }
        
        result_t fits(e_fixed_t e, fits_min_t min, fits_max_t max) const
        {
            return (*this)(e, min, max);
        }

        // entrypoint for `signed T`
        template <typename T, typename U>
        auto fits(U&& e) const
        {
            constexpr fits_min_t min { std::numeric_limits<T>::min() };
            constexpr fits_max_t max { std::numeric_limits<T>::max() };
            return fits(std::forward<U>(e), min, max);
        }

        // unsigned version:: allow signed min to unsigned max
        template <typename T, typename U>
        std::enable_if_t<!std::is_void<T>::value, result_t>
        constexpr ufits(U&& e) const
        {
            using ST = std::make_signed_t<T>;
            using UT = std::make_unsigned_t<T>;

            // don't used braced init. work around in `integral` overload
            constexpr fits_min_t min { std::numeric_limits<ST>::min() };
            constexpr fits_max_t max = std::numeric_limits<UT>::max();

#if 0
            print_type_name{"ufits"}.name<T>();
            std::cout << "value = " << e << "/" << std::hex << e;
            std::cout << " min/max = " << min << "/" << max;
            std::cout << std::dec << std::endl;
#endif
            return fits(std::forward<U>(e), min, max);
        }

        // ufits<void> specialization for chunk::value_type too big.
        template <typename T, typename U>
        std::enable_if_t<std::is_void<T>::value, result_t>
        constexpr ufits(U&& e) const
        {
            return no;
        }

        // ufits with value sz argument
        template <typename T>
        auto ufits_sz(T&& e, unsigned sz) const
        {
            switch (sz)
            {
                case 1: return ufits<uint8_t> (std::forward<T>(e));
                case 2: return ufits<uint16_t>(std::forward<T>(e));
                case 4: return ufits<uint32_t>(std::forward<T>(e));
                case 8: return ufits<uint64_t>(std::forward<T>(e));
                
                default:
                    throw std::logic_error(__FUNCTION__);
            }
        }

        template <typename U>
        constexpr auto zero(U&& e) const
        {
            return fits(std::forward<U>(e), 0, 0);
        }

        // displacement offsets are always signed
        template <typename T>
        constexpr auto disp(expr_t const& e, int delta) const
        {
            static_assert(std::is_signed<T>::value, "displacements are signed");
            constexpr fits_min_t min { std::numeric_limits<T>::min() };
            constexpr fits_max_t max { std::numeric_limits<T>::max() };
            return disp(e, min, max, delta);
        }

        constexpr auto disp_sz(unsigned sz, expr_t const& e, int delta) const
        {
            switch (sz)
            {
                case 0:
                    return disp(e, 0, 0, delta);
                case 1:
                    return disp<int8_t>(e, delta);
                case 2:
                    return disp<int16_t>(e, delta);
                case 4:
                    return disp<int32_t>(e, delta);
                default:
                    throw std::logic_error(__FUNCTION__);
            }
        }

        template <typename U>
        constexpr auto zero(U&& e, int delta) const
        {
            return disp(std::forward<U>(e), 0, 0, delta);
        }

    protected:
        // use non-reference argument as imperfect match
        // require integers to promote to tested type
        template <typename T>
        std::enable_if_t<!std::is_integral_v<T>, result_t>
        operator()(T const&, fits_min_t, fits_max_t) const
        {
            print_type_name{"expr_fits: emits_value"}.name<T>();
            bool is_value_type = emits_value<T>::value;
            return is_value_type ? MIGHT_FIT : NO_FIT;
        }

        result_t operator()(core::core_expr_t const& e, fits_min_t, fits_max_t) const
        {
            std::cout << "expr_fits::fits: expr = " << expr_t(e) << std::endl;
            return MIGHT_FIT;
        }

        // implement `integral` and `error` types
        // constexpr
        result_t operator()(fits_value_t value, fits_min_t min, fits_max_t max) const
        {
            // work around passing `max` in signed variable
            static_assert(sizeof(fits_max_t) >= sizeof(e_fixed_t));

            // get limits as unsigned -- if `max` exceeds value::max, then test as unsigned
            using u_fixed_t = std::make_unsigned_t<e_fixed_t>;
            std::make_unsigned_t<fits_max_t> u_max = max;
            constexpr decltype(u_max) value_max = std::numeric_limits<u_fixed_t>::max();
#if 0
            print_type_name{"operator()"}.name<decltype(value)>();
            std::cout << "value = " << value << "/" << std::hex << value;
            std::cout << " min/max = " << min << "/" << max;
            std::cout << " value_max = " << value_max;
#endif
            bool ok{};
            if (max > value_max) {
                // test as unsigned if max out of range for `value`
      //          std::cout << " u_max = " << u_max;
                ok = static_cast<decltype(u_max)>(value) <= u_max;
            } else {
                ok = min <= value && value <= max;
            }

      //      std::cout << std::dec << " OK = " << ok << std::endl;
            return ok ? DOES_FIT : NO_FIT;
        }

        auto operator()(kas::parser::kas_error_t const&, fits_min_t min, fits_max_t max) const
        {
            // errors always fit non-zero field, so as to propogate original error.
            return max ? DOES_FIT : NO_FIT;
        }
    };
}

#endif
