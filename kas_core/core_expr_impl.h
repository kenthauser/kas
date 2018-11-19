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
#include "core_emit.h"

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

    template<>
    expression::e_fixed_t const* core_expr::get_p<expression::e_fixed_t>() const
    {
        // NB: `expression::e_fixed_t` undefined when `core_expr` declared
        static_assert(sizeof(expr_fixed_t) >= sizeof(expression::e_fixed_t));
        static_assert(std::is_signed<expr_fixed_t>::value);
        static_assert(std::is_integral<expr_fixed_t>::value);

        flatten();          // know what we know

        if (plus.empty() && minus.empty())
            return &fixed;

        return nullptr;
    }

    template <>
    template <>
    expression::e_fixed_t const* symbol_ref::get_p<expression::e_fixed_t>() const
    {
        // if symbol is defined, evaluate its value
        if (auto p = get().value_p())
            return p->get_fixed_p();
        return nullptr;
    }

    template <>
    parser::kas_loc const* core_expr::get_p<parser::kas_loc>() const
    {
        if (plus.size())
            return &plus.front().loc;
        if (minus.size())
            return &minus.front().loc;
        return nullptr;
    }

    core_expr::expr_fixed_t const* core_expr::get_fixed_p() const
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
    core_expr& core_expr::operator+(expr_fixed_t v)
    {
        fixed += v;
        return *this;
    }
    core_expr& core_expr::operator+(core_symbol const& sym)
    {
        plus.emplace_back(sym);
        return *this;
    }
    core_expr& core_expr::operator+(core_addr const& addr)
    {
        plus.emplace_back(addr);
        return *this;
    }
    core_expr& core_expr::operator+(core_expr const& other)
    {
        plus.insert(plus.end(),   other.plus.begin(), other.plus.end());
        minus.insert(minus.end(), other.minus.begin(), other.minus.end());
        fixed  += other.fixed;
        return *this;
    }

    // operator-
    core_expr& core_expr::operator-(expr_fixed_t v)
    {
        fixed -= v;
        return *this;
    }
    core_expr& core_expr::operator-(core_symbol const& sym)
    {
        minus.emplace_back(sym);
        return *this;
    }
    core_expr& core_expr::operator-(core_addr const& addr)
    {
        minus.emplace_back(addr);
        return *this;
    }
    core_expr& core_expr::operator-(core_expr const& other)
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

    core_expr::core_expr(expr_t const& e) 
    {
        if (auto p = e.get_fixed_p())
            fixed = *p;
        else if (auto p = e.template get_p<core_symbol>())
            plus.emplace_back(*p);
        else if (auto p = e.template get_p<core_addr>())
            plus.emplace_back(*p);
        else if (auto p = e.template get_p<core_expr>())
            *this = *p;     // use copy operator
        else
            error = parser::kas_diag::error("invalid expression").ref();
    }

    core_expr& core_expr::operator= (core_expr const& other)
    {
        plus.clear();
        plus.insert(plus.end(), other.plus.begin(), other.plus.end());
        
        minus.clear();
        minus.insert(minus.end(), other.minus.begin(), other.minus.end());

        fixed = other.fixed;
        error = other.error;
        reloc_cnt = -1;
        return *this;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    // implement `core_expr` methods
    //
    ///////////////////////////////////////////////////////////////////////////

    expr_offset_t core_expr::get_offset(core_expr_dot const* dot_ptr)
    {
        //std::cout << "core_expr::get_offset: expr = " << expr_t(*this) << std::endl;

        if (reloc_cnt < 0)
            //throw std::logic_error("core_expr::get_offset w/o num_relocs");
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

        //std::cout << "core_expr::get_offset: " << offset << " reloc_cnt: " << reloc_cnt << std::endl;

        return offset;
    }

    void core_expr::flatten()
    {
        //std::cout << "core_expr::flatten: " << expr_t(*this) << std::endl;
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
            throw std::runtime_error("core_expr::flatten: infinite loop");

        //std::cout << "core_expr::flatten: reduce: " << expr_t(*this) << std::endl;
        // Reduce trees. Look for same plus/minus symbols/addresses
        for (auto& p : plus) {
            if (p.addr_p)
                for (auto& m : minus)
                    if (m.addr_p == p.addr_p) {
                        m.erase();
                        p.erase();
                        break;
                    }
            if (p.symbol_p)
                for (auto& m : minus)
                    if (m.symbol_p == p.symbol_p) {
                        m.erase();
                        p.erase();
                        break;
                    }
        }

        prune();
        //std::cout << "core_expr::flatten: done: " << expr_t(*this) << std::endl;
    }

    template <typename BASE_T>
    void core_expr::emit(BASE_T& base) const
    {
        auto n = calc_num_relocs();

        //std::cout << "core_expr::emit: " << expr_t(*this) << " fixed = " << fixed << std::endl;

        // emit fixed
        base(fixed);

        // do plus. assume all addresses relaxed. need to to resolve addresses
        for (auto&p : plus) {
            if (p.addr_p) {
                // if addr is paired, just add in offset delta
                if (p.p) {
                    //std::cout << " ==> data changed from " << base.data;
                    base( p.addr_p->offset()());
                    base(-p.p->addr_p->offset()());
                    //std::cout << " to " << base.data << std::endl;
                } else {
                    //std::cout << " ==> emit addr " << *p.addr_p << std::endl;
                    base(*p.addr_p);
                }
                continue;
            }

            if (p.symbol_p) {
                //std::cout << " ==> emit symbol " << expr_t(*p.symbol_p) << std::endl;
                base(*p.symbol_p);
                continue;
            }
        }

        // XXX don't know how to emit minus relocs. ignore for now

    }

    void core_expr::prune()
    {
        //std::cout << "core_expr::prune: " << expr_t(*this) << std::endl;
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

    core_expr::expr_term::expr_term(core_symbol const& sym)
            : symbol_p(&sym) //, loc(sym.get_loc())
    {
        // resolve symbol & copy symbol by "value"
        value_p  = symbol_p->value_p();
        addr_p   = symbol_p->addr_p();
    }

    core_expr::expr_term::expr_term(core_addr const& addr)
            : addr_p(&addr) //, loc(addr.get_loc())
            {}

    core_expr::expr_term::expr_term(expr_term const& other)
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
    bool core_expr::expr_term::flatten(core_expr& e, bool is_minus)
    {

        // if `symbol` was forward referenced, just update `addr_p`
        // NB:: this will never match an EQU
        if (symbol_p && !addr_p)
            addr_p   = symbol_p->addr_p();

        // if `addr_p`, node is flat.
        if (addr_p || !value_p)
            return true;

        // Here have `value_p`. Interpret expression & splice into `core_expr`
        // Could use `visitor`, but only a few types matter, so just
        // expand list if `core_expr` handles additional base types

        // //std::cout << "flatten node: " << /* expr_t(_sym) << */ std::endl;

        // handle `e_fixed_t` values
        if (auto p = value_p->get_fixed_p()) {
            e.fixed += is_minus ? -*p : *p;
            this->erase();
            return true;
        }

        // handle `core_symbol` values -- overwrite node
        if (auto p = value_p->get_p<symbol_ref>()) {
            *this = p->get();
            return false;   // not done. flatten again.
        }

        // handle `core_addr *` values -- overwrite node
        if (auto p = value_p->get_p<addr_ref>()) {
            *this = p->get();
            return true;    // addr_p set. done.
        }

        // `core_expr` is final supported type. All others are error.
        auto p = value_p->get_p<expr_ref>();
        if (!p) {
            // identify error node: `value_p && !symbol_p`
            symbol_p = {};
            return true;
        }

        // We have a `const` pointer.
        // copy elements to `expr`
        auto other = p->get();
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

    template <typename OS>
    void core_expr::print(OS& os) const
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
}

#endif
