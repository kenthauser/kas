#ifndef KAS_CORE_EXPR_TYPE_H
#define KAS_CORE_EXPR_TYPE_H

//             c o r e _ e x p r e s s i o n s

// Core expressions allow arbitrary addition/subraction of
// symbols, locations (`core_addr`), and fixed constants. The
// `core_expression` type can hold values which can not be
// handled by the backend (eg: sum of multiple external symbols,
// a negative segment offset, etc) -- these must be rejected as
// invalid by the backend (via `core_emit`). However, the `core_expr`
// type will properly pair and cancel out a "plus" and "minus" of the
// same symbol and handles "segment" relocations on a "net" basis.

// Special support is provided for two cases: `fixed` value expressions,
// and "net zero" segment expressions (eg branch offsets)

// Fixed value expressions
//  - can have unlimted fixed operands
//  - EQU symbols with fixed values
//  - paired (ie net zero) identical symbols (direct or via EQU)
//  - paired (ie net zero) identical locations (via dot, defined symbol, or EQU)
//
// The `core_expr` specializes the `get` template so `expr.get_fixed_p()`
// will find it.
//
// Support for branch offset ("net zero") includes hooking into the
// `dot delta` system & returning a min/max during address relax.
// The `core_fits` type fully supports branch offsets.

#include "kas_object.h"
#include "core_terminal_types.h"
#include "parser/kas_error.h"
#include <list>

namespace kas::core
{
    // forward declare types
    using expression::ast::expr_t;
    struct core_symbol;
    //struct emit_base;

    struct core_expr : kas_object<core_expr, expr_ref>
    {
        // this type defined before `e_fixed_t` is available
        // NB declared type must be able to hold `e_fixed_t`
        // enforced by static_assert in `core_expr::get<e_fixed_t>`
        using expr_fixed_t = int;

        struct expr_term
        {
            // if more types added -- expand `flatten()`
            expr_term(core_symbol const& sym);
            expr_term(core_addr   const& addr);

            // custom copy ctor -- clear mutables
            expr_term(expr_term const&);

            bool flatten(core_expr&, bool);

            // erase & test if erased
            bool empty () const   { return !symbol_p && !addr_p; }
            void erase()          { symbol_p = {}; addr_p = {}; value_p = {}; }

            // instance variables
            core_symbol   const *symbol_p {};
            expr_t        const *value_p  {};
            core_addr     const *addr_p   {};
            parser::kas_loc      loc {};

            // set by `core_expr::get_offset()` to tag offsets
            // NB: mutable values set in `expr_term::offset()`
            expr_offset_t offset(core_expr_dot const* = nullptr) const;
            enum cx_delta_t : uint16_t;
            mutable expr_term const *p {};
            mutable cx_delta_t cx {};
        };

        using emits_value = std::true_type;
        using sym_list_t = std::list<expr_term>;

        // ctors
        // NB: plain `int` ctor picks up `float`. Fix with MPL
        template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
        explicit core_expr(T value) : fixed{value} {}

        core_expr(core_symbol const& sym)  : plus{sym } {}
        core_expr(core_addr   const& addr) : plus{addr} {}

        // forward declare complicated ctor & copy assignment operator
        core_expr(expr_t const& e);
        core_expr& operator=(core_expr const&);
        
        // create expressions with "ref_loc_t" handles.
        // this is a little difficult because underlying types (eg `core_symbol`)
        // are incomplete types. The `sizeof` SFINAE trick defers evaluation of 
        // this ctor only after underlying type completely defined.
        template <typename Ref
                , auto N = sizeof(typename Ref::object_t)>
                //, typename = std::enable_if_t<std::is_base_of<ref_loc_tag, Ref>::value>>
        core_expr(Ref const& ref)
            : core_expr(ref.get())
        {
            // init loc
        }
        
        // copy ctor (copy & allocate)
        core_expr(core_expr const& other)
        {
            std::memcpy(this, &add(other.fixed), sizeof(*this));
            plus  = other.plus;
            minus = other.minus;
        }

        // `core_expr` can only operate on specific types
        // operator+
        core_expr& operator+(core_expr const&);
        core_expr& operator+(core_symbol const&);
        core_expr& operator+(core_addr const&);
        core_expr& operator+(expr_fixed_t);

        // operator-
        core_expr& operator-(core_expr const&);
        core_expr& operator-(core_symbol const&);
        core_expr& operator-(core_addr const&);
        core_expr& operator-(expr_fixed_t);

        // specialize for allowed types (eg e_fixed_t, kas_loc)
        template <typename T>
        T const* get_p() const
        {
            return nullptr;
        }

        // backend interface: expr can have multiple relocs
        template <typename BASE_T> void emit(BASE_T&) const;

        // interface for `core_fits`: implemented in `core_expr_fits.h`
        short calc_num_relocs() const;
        short num_relocs() const { return reloc_cnt; }
        expr_offset_t get_offset(core_expr_dot const *dot = nullptr);

        // support "displacement" calculation
        bool disp_ok(core_expr_dot const& dot) const;
        expr_offset_t get_disp(core_expr_dot const& dot) const;

        template <typename OS> void print(OS&) const;

        expr_offset_t get_offset(core_expr_dot const *dot = nullptr) const
        {
            return const_cast<core_expr*>(this)->get_offset(dot);
        }

        // just declare because don't know "expression::e_fixed_t" yet.
        expr_fixed_t const* get_fixed_p() const;

    private:
        // evaluate symbols, find duplicates & combine constants
        void flatten();

        // remove evaluated expressions
        void prune();

        // flatten modifies everything, but in a "mutable" way.
        // Just `const_cast` the non-const version.
        void flatten() const
        {
            const_cast<core_expr*>(this)->flatten();
        }

        // find same-fragment operands & calculate relax_deltas
        void pair_nodes() const;     // calculate mutable variables

        // elements: operands and the constant
        sym_list_t plus;
        sym_list_t minus;
        expr_fixed_t fixed {};
        parser::kas_error_t  error {};

        // side effect of `num_relocs`
        mutable short reloc_cnt{-1};
        static inline core::kas_clear _c{base_t::obj_clear};
    };

    static_assert (!std::is_constructible<core_expr, float>::value);
    static_assert ( std::is_constructible<core_expr, int>::value);
    static_assert ( std::is_constructible<core_expr, core_symbol&&>::value);


    // hook `expr_t` expressions into type system...
    // ...test if possible to implicitly construct `core_expr` from first type
    // ...and if second type can be added/subtracted from `core_expr`

    // expr operator{+,-} expr ==> core_expr
    // XXX not sure why compiler doesn't resolve operator+(core_expr const&)
    // XXX without is_same<> clause, but there you have it. 2017/6/10
    template <typename T, typename U, typename =
                std::enable_if_t<!std::is_same<core_expr, std::decay_t<T>>::value
//                                && std::is_constructible<core_expr, T>::value
                                && std::is_convertible<T&&, core_expr>::value
                                >>
    inline auto operator+(T const& t, U&& u)
        -> decltype(std::declval<core_expr&>().operator+(std::forward<U>(u)))
    {
        auto& expr = core_expr::add(t);
        return expr.operator+(std::forward<U>(u));
    }
    template <typename T, typename U, typename =
                std::enable_if_t<!std::is_same<core_expr, std::decay_t<T>>::value
  //                              && std::is_constructible<core_expr, T>::value
                                && std::is_convertible<T&&, core_expr>::value
                                >>
    inline auto operator-(T const& t, U&& u)
        -> decltype(std::declval<core_expr&>().operator+(std::forward<U>(u)))
    {
        auto& expr = core_expr::add(t);
        return expr.operator-(std::forward<U>(u));
    }

    // operator-()
    template <typename T>
    inline auto operator-(T&& t)
        -> decltype(std::declval<core_expr&>().operator-(std::forward<T>(t)))
    {
        return core_expr::add(0).operator-(std::forward<T>(t));
    }

    // declare specialization of get_p<>
    template <> parser::kas_loc const* core_expr::get_p<parser::kas_loc>() const;
}


#endif
