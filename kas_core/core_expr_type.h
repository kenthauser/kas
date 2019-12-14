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
#include "parser/kas_error.h"
#include <list>

namespace kas::core
{
// forward declare types
using expression::ast::expr_t;
using expression::e_fixed_t;
struct core_fits;

template <typename Ref>
struct core_expr : kas_object<core_expr<Ref>, Ref>
{
    // this type defined before `e_fixed_t` is available
    // NB declared type must be able to hold `e_fixed_t`
    // enforced by static_assert in `core_expr::get<e_fixed_t>`
    using base_t       = kas_object<core_expr<Ref>, Ref>;
    using base_t::index;
private:
    struct expr_term
    {
        // if more types added -- expand `flatten()`
        expr_term(core_symbol_t const& sym);
        expr_term(core_addr_t   const& addr);

        // custom copy ctor -- clear mutables
        expr_term(expr_term const&);

        bool flatten(core_expr&, bool);

        // erase & test if erased
        bool empty () const   { return !symbol_p && !addr_p; }
        void erase()          { symbol_p = {}; addr_p = {}; value_p = {}; }

        // instance variables
        core_symbol_t        const *symbol_p {};
        expr_t          const *value_p  {};
        core_addr_t          const *addr_p   {};
        parser::kas_loc loc {};

        // set by `core_expr::get_offset()` to tag offsets
        // NB: mutable values set in `expr_term::offset()`
        expr_offset_t offset(core_expr_dot const* = nullptr) const;
        enum cx_delta_t : uint16_t;
        mutable expr_term const *p {};
        mutable cx_delta_t cx {};
    };
public:
    using emits_value = std::true_type;
    using sym_list_t = std::list<expr_term>;
    
    // ctors
    // NB: plain `int` ctor picks up `float`. Fix with MPL
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    explicit core_expr(T value) : fixed{value} {}
   
    // ctors for supported types
    core_expr(core_symbol_t const& sym)  : plus{sym } {}
    core_expr(core_addr_t   const& addr) : plus{addr} {}

    // copy ctor (copy & allocate)
    //core_expr(core_expr const& other);
    
    // forward declare copy assignment operator
    core_expr& operator=(core_expr const&);
    
    // `core_expr` can only operate on specific types
    // operator+
    core_expr& operator+(core_expr     const&);
    core_expr& operator+(core_symbol_t const&);
    core_expr& operator+(core_addr_t   const&);
    core_expr& operator+(e_fixed_t);

    // operator-
    core_expr& operator-(core_expr      const&);
    core_expr& operator-(core_symbol_t  const&);
    core_expr& operator-(core_addr_t    const&);
    core_expr& operator-(e_fixed_t);

    // add hooks for `ref_loc_t` wrapped objects
    core_expr& operator+(symbol_ref ref) { return *this + ref.get(); }; 
    core_expr& operator+(addr_ref   ref) { return *this + ref.get(); }; 
    core_expr& operator+(expr_ref   ref) { return *this + ref.get(); }; 
    core_expr& operator-(symbol_ref ref) { return *this - ref.get(); }; 
    core_expr& operator-(addr_ref   ref) { return *this - ref.get(); }; 
    core_expr& operator-(expr_ref   ref) { return *this - ref.get(); }; 

    // specialize for allowed types (eg e_fixed_t, kas_loc)
    // XXX modify to `get_p(T)` to simplify specialization
    template <typename T>
    T const* get_p() const
    {
        return this->get_p(T{});
    }
   
    template <typename T>
    auto get_p(T const&) const
    {
        return nullptr;
    }
    
    e_fixed_t const *get_p(e_fixed_t const&) const;

    // backend interface: expr can have multiple relocs
    template <typename BASE_T, typename RELOC_T> void emit(BASE_T&, RELOC_T&) const;

    // interface for `core_fits`: implemented in `core_expr_fits.h`
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
    e_fixed_t const* get_fixed_p() const;

private:
    friend core_fits;

    // evaluate symbols, find duplicates & combine constants
    void flatten();

    // flatten modifies everything, but in a "mutable" way.
    // Just `const_cast` the non-const version.
    void flatten() const
    {
        const_cast<core_expr*>(this)->flatten();
    }

    // remove evaluated expressions
    void prune();

    // find same-fragment operands & calculate relax_deltas
    void pair_nodes() const;     // calculate mutable variables
    
    // helper for `core_fits`
    short calc_num_relocs() const;

    // elements: operands and the constant
    sym_list_t plus;
    sym_list_t minus;
    e_fixed_t fixed {};
    parser::kas_error_t  error {};

    // side effect of `num_relocs`
    mutable short reloc_cnt{-1};
    static inline core::kas_clear _c{base_t::obj_clear};
};

#if 1
// XXX move to impl

static_assert (!std::is_constructible_v<core_expr_t, float>);
static_assert ( std::is_constructible_v<core_expr_t, int>);
static_assert ( std::is_constructible_v<core_expr_t, core_symbol_t const&>);
static_assert ( std::is_constructible_v<core_expr_t, core_addr_t   const&>);
#endif

// hook `core_expr_t` expressions into type system...
// ...test if possible to implicitly construct `core_expr` from first type
// ...and if second type can then be added/subtracted from `core_expr_t`
#if 1
// expr operator+ expr ==> core_expr_t&
template <typename T, typename U,
          typename = std::enable_if_t<std::is_constructible_v<core_expr_t, T>>>
auto operator+(T&& t, U&& u)
        -> decltype(std::declval<core_expr_t&>().operator+(std::forward<U>(u)))
{ return core_expr_t::add(std::forward<T>(t)).operator+(std::forward<U>(u)); }

// expr operator- expr ==> core_expr_t&
template <typename T, typename U,
          typename = std::enable_if_t<std::is_constructible_v<core_expr_t, T>>>
auto operator-(T&& t, U&& u)
        -> decltype(std::declval<core_expr_t&>().operator-(std::forward<U>(u)))
{ return core_expr_t::add(std::forward<T>(t)).operator-(std::forward<U>(u)); }

// operator-()
template <typename U>
auto operator-(U&& u)
        -> decltype(std::declval<core_expr_t&>().operator-(std::forward<U>(u)))
{ return core_expr_t::add(0).operator-(std::forward<U>(u)); }

#endif
}
#endif
