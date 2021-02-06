#ifndef KAS_CORE_CORE_EXPR_H
#define KAS_CORE_CORE_EXPR_H

/******************************************************************************

Implementation notes:

1. `expr_term` can be constructed from a "core_symbol", or "core_addr".
    `symbol_ref` also contain a `kas_loc` used in error messages.

2. When `core_symbol` is added, use save symbol pointer.
    - save sym->value_p in `expr_term` to lookup EQUs by value.
    - save sym->addr_p  in `expr_term` to lookup LBLs by value.

3. `expr_term` contains three active pointers resolved as follows
    - if `addr_p` is set, it is the "resolved" value.
    - else if `value_p` is set, it is the "resolved" value
    - else `symbol_p` must be "external" (or forward reference)

4. When flattening `expr_term` lists: (for get<e_fixed_t> or `core_fits`)

    1. if `symbol_p` set, lookup value & addr_p (could have been referenced
        before defined -- ie a forward symbol reference)

    2. if coreaddr_p() set: address is value.

    3. if value_p set: recurse on value:
        switch(type):
            core_addr:  update `expr_term` coreaddr_p().
            e_fixed_t:  add to offset, delete sym_p, value_p
            core_expr:  1. execute core_expr->flatten()
                        2. use core_expr operator{+,-} to splice *copy*
                        3. delete sym_p, value_p

    4. if only `sym_p` set, symbol is external or undefined

5. When evaluating `expr_term` lists seeking pairs: (for `core_fits`)

    1. if coreaddr_p() set: exact match only on addr_p

    2. `value_p` should not be set unless `coreaddr_p()` set -- so ignore

    3. `sym_p` exact match on `sym_p` (should only be externals -- no EQUs)

6. Remove unneeded list elements (prune) [sym_p & addr_p null]

7. Pair plus/minus relocs on `addr_p->frag_p`.
    - can now calculate counts of "sym+/-", "frag +/-" relocs.
    (see `core_expr_fits.h` for details)

******************************************************************************/


#include "core_expr_type.h"
#include "core_symbol.h"
#include "core_fragment.h"
#include "core_reloc.h"

#include <numeric>

namespace kas::core
{
///////////////////////////////////////////////////////////////////////////
//
// specialize templates to support `expression.get_fixed_t()`
//
// - define `get<e_fixed_t>` for `core_expr` && `core_symbol`
//
///////////////////////////////////////////////////////////////////////////

template <>
auto core_expr_t::get_p(e_fixed_t const&) const -> e_fixed_t const *
{
    flatten();          // know what we know

    if (plus.empty() && minus.empty())
        return &fixed;

    return nullptr;
}

#if 0
template <>
template <>
expression::e_fixed_t const* core_symbol_t::get_p<expression::e_fixed_t>() const
{
    // if symbol is defined, evaluate its value
    if (auto p = get().value_p())
        return p->get_fixed_p();
    return nullptr;
}

template <>
template <>
kas::parser::kas_loc const* core_expr_t::get_p<parser::kas_loc>() const
{
    if (plus.size())
        return &plus.front().loc;
    if (minus.size())
        return &minus.front().loc;
    return nullptr;
}
#endif

template <>
e_fixed_t const* core_expr_t::get_fixed_p() const
{
    if (get_p<expression::e_fixed_t>())
        return &fixed;
    return nullptr;
}
    



///////////////////////////////////////////////////////////////////////////
//
// implement parser operations declared in `core_expr_type`
//
///////////////////////////////////////////////////////////////////////////

// operator+
template <typename REF>
auto core_expr<REF>::operator+(e_fixed_t v) -> core_expr&
{
    fixed += v;
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator+(core_symbol_t const& sym) -> core_expr&
{
    plus.emplace_back(sym);
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator+(core_addr_t const& addr) -> core_expr&
{
    plus.emplace_back(addr);
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator+(core_expr const& other) -> core_expr&
{
    plus.insert(plus.end(),   other.plus.begin(), other.plus.end());
    minus.insert(minus.end(), other.minus.begin(), other.minus.end());
    fixed  += other.fixed;
    return *this;
}

// operator-
template <typename REF>
auto core_expr<REF>::operator-(e_fixed_t v) -> core_expr&
{
    fixed -= v;
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator-(core_symbol_t const& sym) -> core_expr&
{
    minus.emplace_back(sym);
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator-(core_addr_t const& addr) -> core_expr&
{
    minus.emplace_back(addr);
    return *this;
}
template <typename REF>
auto core_expr<REF>::operator-(core_expr const& other) -> core_expr&
{
    plus.insert(plus.end(),   other.minus.begin(), other.minus.end());
    minus.insert(minus.end(), other.plus.begin(), other.plus.end());
    fixed  -= other.fixed;
    return *this;
}


///////////////////////////////////////////////////////////////////////////
//
// implement `core_expr` non-trivial constructors
//
///////////////////////////////////////////////////////////////////////////

#if 0
// NB: allows `float`
template <typename REF>
core_expr<REF>::core_expr(expr_t const& e) 
{
    if (auto p = e.get_fixed_p())
        fixed = *p;
    else if (auto p = e.template get_p<core_symbol_t>())
        plus.emplace_back(*p);
    else if (auto p = e.template get_p<core_addr_t>())
        plus.emplace_back(*p);
    else if (auto p = e.template get_p<core_expr_t>())
        *this = *p;     // use copy operator
    else
        error = parser::kas_diag_t::error("invalid expression").ref();
}
#endif
    
// asignment constructor
template <typename REF>
auto core_expr<REF>::operator= (core_expr const& other) -> core_expr&
{
    // allocate new `core_expr` instance & populate with current values
    auto& n = this->add(other.fixed);
    n.plus.insert (n.plus.end() , other.plus.begin() , other.plus.end());
    n.minus.insert(n.minus.end(), other.minus.begin(), other.minus.end());

    n.reloc_cnt = -1;
    return n;
}

///////////////////////////////////////////////////////////////////////////
//
// implement `core_expr` methods
//
///////////////////////////////////////////////////////////////////////////

template <typename REF>
expr_offset_t core_expr<REF>::get_offset(core_expr_dot const* dot_ptr)
{
    //std::cout << "core_expr<REF>::get_offset: expr = " << expr_t(*this) << std::endl;

    if (reloc_cnt < 0)
        //throw std::logic_error("core_expr<REF>::get_offset w/o num_relocs");
        calc_num_relocs();

    expr_offset_t offset{fixed};
    bool do_prune = false;
#if 0
    for (auto& elem : plus) {
        auto delta = elem.offset(dot_ptr);
        if (delta.is_relaxed()) {
            do_prune = true;
            fixed += delta();
            elem.erase();
        }
        offset += delta;
    }
#endif
#if 1
    for (auto& elem : plus)
        offset += elem.offset(dot_ptr);
    for (auto& elem : minus)
        offset -= elem.offset(dot_ptr);
#else
    for (auto& elem : plus) {
        auto delta = elem.offset(dot_ptr);
        if (delta.is_relaxed()) {
            do_prune = true;
            fixed += delta();
            elem.erase();
        }
        offset += delta;
    }
    for (auto& elem : minus) {
        auto delta = elem.offset(dot_ptr);
        if (delta.is_relaxed()) {
            do_prune = true;
            fixed -= delta();
            elem.erase();
        }
        offset -= delta;
    }

#endif
    // remove resolved elements
    if (do_prune)
        prune();

    //std::cout << "core_expr<REF>::get_offset: " << offset << " reloc_cnt: " << reloc_cnt << std::endl;

    return offset;
}

template <typename REF>
void core_expr<REF>::flatten()
{
    //std::cout << "core_expr<REF>::flatten: " << expr_t(*this) << std::endl;
    int n = 100;    // max loops: big number before `throw`
    for (bool done = false; !done && n; --n) {
        done = true;

        // XXX
        auto do_plus  = [&](auto&& node) { return node.flatten(*this, false); };
        auto do_minus = [&](auto&& node) { return node.flatten(*this, true);  };
        done &= std::all_of(plus.begin(),  plus.end(),  do_plus);
        done &= std::all_of(minus.begin(), minus.end(), do_minus);
    }

    if (!n)
        throw std::runtime_error("core_expr<REF>::flatten: infinite loop");

    //std::cout << "core_expr<REF>::flatten: reduce: " << expr_t(*this) << std::endl;
    // Reduce trees. Look for same plus/minus symbols/addresses
    for (auto& p : plus)
    {
        if (p.addr_p)
        {
            for (auto& m : minus)
                if (m.addr_p == p.addr_p)
                {
                    m.erase();
                    p.erase();
                    break;
                }
        }
        else if (p.symbol_p)
            for (auto& m : minus)
                if (m.symbol_p == p.symbol_p)
                {
                    m.erase();
                    p.erase();
                    break;
                }
    }

    prune();
    //std::cout << "core_expr<REF>::flatten: done: " << expr_t(*this) << std::endl;
}

template <typename REF>
template <typename BASE_T, typename RELOC_T>
void core_expr<REF>::emit(BASE_T& base, RELOC_T& reloc, parser::kas_error_t& diag) const
{
    auto do_emit = [&base, &diag, loc_p=reloc.loc_p]
                    (auto& reloc, expr_term const *term_p = {}, bool pc_rel = false)
    {
        reloc.loc_p     = loc_p;
        reloc.sym_p     = {};
        reloc.section_p = {};
        
        // interpret expr_term as addr, symbol, or error
        if (term_p)
        {
            if (term_p->symbol_p)
            {
                reloc.sym_p = term_p->symbol_p;
            }
            else if (term_p->addr_p)
            {
                reloc.addend   +=  term_p->addr_p->offset()();
                reloc.section_p = &term_p->addr_p->section();
            }
            else if (term_p->value_p)
            {
                // not symbol nor addr -> error term
                // if value_p is not error, create one
                auto p = term_p->value_p->template get_p<parser::kas_diag_t>();
                if (!p)
                    p = &parser::kas_diag_t::error("Invalid expression", *loc_p);
                diag = *p;

            }
        }

        if (pc_rel)
            reloc.reloc.flags |=  kbfd::kbfd_reloc::RFLAGS_PC_REL;
        else
            reloc.reloc.flags &=~ kbfd::kbfd_reloc::RFLAGS_PC_REL;
    
        reloc.emit(base, diag);
    };

    calc_num_relocs();      // pair terms, look for error
    

    //std::cout << "core_expr<REF>::emit: " << expr_t(*this) << " fixed = " << fixed << std::endl;
    // build "new" reloc for expression
    reloc.addend      += fixed;     // acumulate `fixed`
    reloc.core_expr_p  = {};        // processing now

    // examine `minus` list to find `pc_rel` & subs
    auto section_p = &base.get_section();
    unsigned pc_rel_cnt = !!(reloc.reloc.flags & kbfd::kbfd_reloc::RFLAGS_PC_REL);
    unsigned minus_cnt  = {};

    // convert minus terms to pc-relative if possible
    for (auto& m : minus)
    {
        // if paired, ignore
        if (m.p)
            continue;
        // if minus term in current section, convert to `PC_REL` cnt
        if (m.addr_p && &m.addr_p->section() == section_p)
        {
            reloc.addend -= m.addr_p->offset()();
            ++pc_rel_cnt;
        }
        else
            ++minus_cnt;
    }

    // loop back thru finding compatable `minus` as needed
    auto m_iter = minus.begin();

    // handle "paired" `plus` expressions
    for (auto& p : plus)
    {
        // if `plus` matched, update `offset`
        if (p.addr_p && p.p)
        {
            reloc.addend += p.addr_p->offset()();
            reloc.addend -= p.p->addr_p->offset()();
            continue;
        }
        
        // if PC_REL pending, & `p` in correct section, consume a `pc_rel`
        if (pc_rel_cnt)
            if (p.addr_p && &p.addr_p->section() == &base.get_section())
            {
                reloc.reloc.flags  &=~ kbfd::kbfd_reloc::RFLAGS_PC_REL;
                reloc.addend       +=  p.addr_p->offset()();
                --pc_rel_cnt;
                continue;
            }

        // here generate a `relocation`. 
        // if `pc_rel` pending, perform first
        if (pc_rel_cnt)
        {
            --pc_rel_cnt;
            do_emit(reloc, &p, true);
            continue;
        }
        
        // emit pending `minus` next
        if (minus_cnt)
        {
            --minus_cnt;
            for(;;)
            {
                auto& m = *m_iter++;
                
                if (m.p)
                    continue;
                if (m.addr_p && &m.addr_p->section() == section_p)
                    continue;

                // here need to emit `minus` relocation
                // same width & offset
                RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits}, 0, reloc.offset };
                
                do_emit(m_reloc, &m);
                break;
            }
        }

        // emit `plus` reloc (w/o pc_rel)
        do_emit(reloc, &p);
    }

    // edge cases: some object formats support subtracting symbols.
    // test if there are any.

    while(pc_rel_cnt--)
    {
        RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits}, 0, reloc.offset };
        do_emit(m_reloc, nullptr, true);
    }
    
    if (minus_cnt)
    {
        for (auto& m : minus)
        {
            if (m.p)
                continue;
            if (m.addr_p && &m.addr_p->section() == section_p)
                continue;

            // here need to emit `minus` relocation
            // same width & offset
            RELOC_T m_reloc { {kbfd::K_REL_SUB(), reloc.reloc.bits}, 0, reloc.offset };
            
            do_emit(m_reloc, &m);
        }
    }

    // if addend, must emit at least once (fixed data)
    if (reloc.addend)
        do_emit(reloc);
}

template <typename REF>
void core_expr<REF>::prune()
{
    //std::cout << "core_expr<REF>::prune: " << expr_t(*this) << std::endl;
    // Prune trees. Remove erased elements
    auto empty = [](auto&& node) { return node.empty(); };
    plus. remove_if(empty);
    minus.remove_if(empty);
}

///////////////////////////////////////////////////////////////////////////
//
// `expr_term` non-trivial constructors
//
///////////////////////////////////////////////////////////////////////////

template <typename REF>
core_expr<REF>::expr_term::expr_term(core_symbol_t const& sym)
        : symbol_p(&sym) //, loc(sym.get_loc())
{
    // resolve symbol & copy symbol by "value"
    value_p  = symbol_p->value_p();
    addr_p   = symbol_p->addr_p();
}

template <typename REF>
core_expr<REF>::expr_term::expr_term(core_addr_t const& addr)
        : addr_p(&addr) //, loc(addr.get_loc())
        {}

template <typename REF>
core_expr<REF>::expr_term::expr_term(expr_term const& other)
{
    static_assert(std::is_standard_layout<expr_term>::value);

    // copy ctor clears mutable variables
    std::memcpy(this, &other, sizeof(expr_term));
    this->cx = {};
    this->p  = {};
}

///////////////////////////////////////////////////////////////////////////
//
// implement `expr_term` methods
//
///////////////////////////////////////////////////////////////////////////

// return true if this node is "flat"
template <typename REF>
bool core_expr<REF>::expr_term::flatten(core_expr& e, bool is_minus)
{

    // if `symbol` was forward referenced, just update `addr_p`
    // NB:: this will never match an EQU
    if (symbol_p && !addr_p)
        addr_p   = symbol_p->addr_p();
    
    // if `addr_p`, node is flat.
    if (addr_p)
        return true;

    if (symbol_p)
        value_p = symbol_p->value_p();

    if (!value_p)
        return true;        // forward declared symbol

    // Here have `value_p`. Interpret expression & splice into `core_expr`
    // Could use `visitor`, but only a few types matter, so just
    // expand list if `core_expr` handles additional base types

    // //std::cout << "flatten node: " << /* expr_t(_sym) << */ std::endl;

    // handle `e_fixed_t` values
    if (auto p = value_p->get_fixed_p())
    {
        e.fixed += is_minus ? -*p : *p;
        this->erase();
        return true;
    }

    // handle `core_symbol` values -- overwrite node
    if (auto p = value_p->get_p<symbol_ref>())
    {
        *this = p->get();
        return false;   // not done. flatten again.
    }

    // handle `core_addr *` values -- overwrite node
    if (auto p = value_p->get_p<addr_ref>())
    {
        *this = p->get();
        return true;    // addr_p set. done.
    }

    // `core_expr` is final supported type. All others are error.
    auto p = value_p->get_p<expr_ref>();
    if (!p)
    {
        // error node: `value_p && !symbol_p`
        return true;
    }

    // We have a `const` pointer.
    // copy elements to `expr`
    auto& other = p->get();
    if (!is_minus)
        e.operator+(std::move(other));
    else
        e.operator-(std::move(other));

    this->erase();      // this node consumed
    return false;       // not done, flatten again.
}


///////////////////////////////////////////////////////////////////////////
//
// print support routines
//
///////////////////////////////////////////////////////////////////////////

template <typename REF>
template <typename OS>
void core_expr<REF>::print(OS& os) const
{
    auto print_elem = [&](auto prefix, auto &e)
    {
        if (e.addr_p)
            os << prefix << *e.addr_p;
        else if (e.value_p)
            os << prefix << *e.value_p;
        else if (e.symbol_p)
            os << prefix << *e.symbol_p;
        // else if (!e._sym.empty())
        //     os << prefix << e._sym.ref;
        else
            os << prefix << "[???]";
        if (e.empty())
            os << " (deleted)";
    };
    
    if (reloc_cnt < 0) {
        os << "cx[" << index() << "](" << fixed;
        for (auto const& elem : plus)
            print_elem("+", elem);
        for (auto const& elem : minus)
            print_elem("-", elem);
        os << ")";
    } else {
        os << "CX[" << index() << "](" << fixed;
        for (auto const& elem : plus) {
            // if paired, print delta
            if (elem.p)
                os << "+" << (elem.offset() - elem.p->offset());
            else
                print_elem("+", elem);
        }
        for (auto const& elem : minus)
            if (!elem.p)
                print_elem("-", elem);
        os << ")";

    }
}



template auto core_expr<expr_ref>::operator+(e_fixed_t v) -> core_expr&;
template auto core_expr<expr_ref>::operator+(core_symbol_t const& sym) -> core_expr&;
template auto core_expr<expr_ref>::operator+(core_addr_t const& addr) -> core_expr&;
template auto core_expr<expr_ref>::operator+(core_expr const& other) -> core_expr&;
template auto core_expr<expr_ref>::operator-(e_fixed_t v) -> core_expr&;
template auto core_expr<expr_ref>::operator-(core_symbol_t const& sym) -> core_expr&;
template auto core_expr<expr_ref>::operator-(core_addr_t const& addr) -> core_expr&;
template auto core_expr<expr_ref>::operator-(core_expr const& other) -> core_expr&;

//template core_expr<expr_ref>::core_expr(core_expr const&);
template core_expr<expr_ref>& core_expr<expr_ref>::operator=(core_expr const&);
template expr_offset_t core_expr<expr_ref>::get_offset(core_expr_dot const* dot_ptr);
template short core_expr<expr_ref>::calc_num_relocs() const;

template core_expr<expr_ref>::expr_term::expr_term(core_symbol_t const&);
template core_expr<expr_ref>::expr_term::expr_term(core_addr_t const&);
template core_expr<expr_ref>::expr_term::expr_term(expr_term const&);

}

#endif
