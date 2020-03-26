#ifndef KAS_CORE_FITS_H
#define KAS_CORE_FITS_H

// core_fits: used in `relax` to test expressions
//
// `core_fits` extends `expr_fits` to support the `core_addr`, `core_expr`,
// and `core_symbol` types. In addition to testing for fixed-size fields,
// the `fits.disp()` [testing for offset from current `dot`] can also be
// implemented.
//
// The `core_fits` object is designed to be used in `relax`, so the constructor
// takes a reference to the `dot` object for the current `frag` & the current
// `fuzz`. The `fuzz` is the delta required to turn a `MIGHT_FIT` into a
// `NO_FIT`. For example, if an expression has a calculated value of [130,130],
// it will fit into a 7-bit field (max 127) if the `fuzz` is over 3 (MIGHT_FIT).
// If `fuzz` is under `3`, it's a NO_FIT.
//
// This allows `core_fits` & `relax` fully resolve expressions
// as `fuzz` is decreased.

#if 0

offset calculation:

when `dot` not in a frag with expr, just sum addr.offset();
when `dot` in frag is before `addr`, just sum addr.offset();
when `dot` is at or past `addr`, need to apply min_delta

how to tell is dot is past addr?
test: addr.offset.max <= dot.offset.max?


#endif

#include "expr/expr_fits.h"
#include "core_expr_dot.h"

namespace kas::core
{
    struct core_fits : expression::expr_fits
    {
        using expr_fits::operator();
        using expr_fits::zero;
        using expr_fits::fits;
        using expr_fits::disp;

        core_fits(core_expr_dot const *dot_p, int fuzz = {})
            : dot_p(dot_p), fuzz(fuzz) {}

        
        // XXX legacy
        core_fits(core_expr_dot const& dot, int fuzz = {}) : core_fits(&dot, fuzz) {}

        //
        // Must re-implement the virtual-function trampoline functions
        // from base type, so that derived type methods will be considered
        //

        virtual result_t fits(expr_t const& e, fits_min_t min, fits_max_t max) const override
        {
            // allow `expr_fits` base type to evaluate fixed value expressions
            if (auto p = e.get_fixed_p())
                return (*this)(*p, min, max);


            return e.apply_visitor(
                x3::make_lambda_visitor<result_t>
                    ([&](auto const& value)
                        { return (*this)(value, min, max); }
                    ));
        }

        virtual result_t disp(expr_t const& e, fits_min_t min, fits_max_t max, int delta) const override
        {
            std::cout << ": " << e;
            std::cout << " min/max = " << min << "/" << max;
            std::cout << ", delta: " << delta;
            std::cout << ", fuzz: "  << fuzz  << std::endl;

            if (auto p = e.get_fixed_p())
                return no;      // constants are not displacements

            return e.apply_visitor(
                x3::make_lambda_visitor<result_t>
                    ([&](auto const& value)
                        { return (*this)(value, min, max, delta); }
                    ));
        }


        // returns true if symbol is before `dot` or external
        // used to disallow branch deletion
        virtual bool seen_this_pass(expr_t const& e) const override
        {
            if (dot_p)
                if (auto sym_p = e.template get_p<core_symbol_t>())
                    if (auto addr_p = sym_p->addr_p())
                        if (&addr_p->section() == &dot_p->section())
                            return dot_p->seen_this_pass(*addr_p);

            // not a symbol, not a label, or not same section
            return true;
        }

        auto& get_dot() const
        {
            return *dot_p;
        }

    protected:
        template <typename T>
        std::enable_if_t<!std::is_integral_v<T>, result_t>
        operator()(T const&, fits_min_t, fits_max_t) const
        {
            print_type_name{"expr_fits: emits_value"}.name<T>();
            bool is_value_type = expression::emits_value<T>::value;
            return is_value_type ? maybe : no;
        }

        // unwrap reference wrappers (std:: & core::ref_loc)
        template <typename T>
        result_t operator()(std::reference_wrapper<T> const& ref, fits_min_t min, fits_max_t max) const
        {
            return (*this)(ref.get(), min, max);
        }

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<core::ref_loc_tag, T>>>
        auto operator()(T const& ref, fits_min_t min, fits_max_t max) const
                -> std::enable_if_t<sizeof(typename T::object_t) != 0, result_t>
        {
            print_type_name{"core_fits: unwrap"}.name<T>();
            return (*this)(ref.get(), min, max);
        }

        // relocated addresses never fit.
        result_t operator()(core_addr_t const& e, fits_min_t min, fits_max_t max) const
        {
            return no;
        }


        result_t operator()(core_symbol_t const& e, fits_min_t min, fits_max_t max) const
        {
            //std::cout << "core_fits: core_symbol: " << expr_t(e);
            //std::cout << " max = " << std::hex << max << std::endl;
            // symbol is expression or relocatable symbol of some sort
            if (auto p = e.value_p())
                return fits(*p, min, max);
            //std::cout << "core_fits: core_symbol: " << expr_t(e) << " -> no" << std::endl;

            return no;
        }

        // catch non-core-expr deltas
        template <typename T>
        result_t operator()(T const&, fits_min_t, fits_max_t, int) const
        {
            print_type_name{"core_fits: (delta) unsupported"}.name<T>();
            return no;
        }

        result_t operator()(core_symbol_t const& sym, fits_min_t min, fits_max_t max, int delta) const
        {
            //std::cout << "core_fits: (disp) core_symbol: " << expr_t(sym);
            //std::cout << " max = " << std::hex << max;
            //std::cout << " delta = " << delta << std::endl;

            if (auto p = sym.addr_p())
                return (*this)(*p, min, max, delta);
            if (auto e = sym.value_p())
                return (*this)(*e, min, max, delta);

            // here common or undefined -- and you can't get there from here.
            return no;
        }
        
        // the non-trivial implementations are in core_expr_fits.h
        result_t operator()(core_expr_t const&, fits_min_t, fits_max_t) const;
        result_t operator()(core_expr_t const&, fits_min_t, fits_max_t, int delta) const;
        result_t operator()(core_addr_t const&, fits_min_t, fits_max_t, int delta) const;

        // support routine to evaluate offset_t<> & fuzz
        //
        // XXX probably need to rethink algorithm for non-zero based min/max.
        // algorithm: assume offset min <= max. Thus if min fits, so does max.
        //         1. if (fits(offset.min) == no
        //              -> result = no.
        //         2. if (fits(ofset.max, within fuzz) == yes
        //              -> result = yes.
        //         3. otherwise maybe...
        //

        template <typename T>
        result_t operator()(offset_t<T> const& offset, fits_min_t min, fits_max_t max, int disp) const
        {
            std::cout << "core_fits (offset) (" << offset << ", " << min << ", " << max << ", " << disp << ")" << std::endl;
            // require `min` to be in range: NB: min can only increase

            // require special test for `min == 0` (ie insn deletion)
            // ...test for offset == { 0, disp }, ie this insn (delete if so)
            if (min == 0)
            {
                if (offset.min != 0) 
                    return no;
                if (offset.max == disp)     // just this insn
                    return yes;
                return maybe;
            }
            else if ((offset.min + disp) < min)
                return no;

            // check max w/o fuzz
            auto max_result = (*this)(offset.max - disp, min, max);
            std::cout << "max_result -> " << +max_result << std::endl;
            if (max_result == yes)
                return yes;

            // check max with fuzz
            max_result = (*this)(offset.max - disp, min, max+fuzz);
            std::cout << "max_result (fuzz) -> " << +max_result << std::endl;

            if (max_result == yes)
                return maybe;
            return no;
        }

    private:
        // pointer to real "dot"
        core_expr_dot const *dot_p;
        int fuzz;
    };
}


#endif
